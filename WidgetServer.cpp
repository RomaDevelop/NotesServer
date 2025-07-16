#include "WidgetServer.h"

#include <QDebug>
#include <QCloseEvent>
#include <QSettings>
#include <QDir>
#include <QLabel>
#include <QTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCryptographicHash>
#include <QGuiApplication>

#include "MyQDifferent.h"
#include "MyQFileDir.h"
#include "MyQExecute.h"
#include "MyQTextEdit.h"
#include "MyCppDifferent.h"
#include "CodeMarkers.h"
#include "TextEditCleaner.h"

#include "Note.h"
#include "DataBase.h"

WidgetServer::WidgetServer(QWidget *parent)
	: QWidget(parent)
{
	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	QHBoxLayout *hlo3 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);
	vlo_main->addLayout(hlo3);

	QPushButton *btn1 = new QPushButton("clear");
	hlo1->addWidget(btn1);
	connect(btn1,&QPushButton::clicked,[this](){ textEdit->clear(); });

	QPushButton *btnSqlTest = new QPushButton(" notes and groups ");
	hlo1->addWidget(btnSqlTest);
	connect(btnSqlTest,&QPushButton::clicked,[this](){
		textEdit->append("notes:");
		auto table = DataBase::DoSqlQueryGetTable("select * from " + Fields::Notes());
		for(auto &row:table) textEdit->append(row.join(' '));

		textEdit->append("\ngroups:");
		table = DataBase::DoSqlQueryGetTable("select * from " + Fields::Groups());
		for(auto &row:table) textEdit->append(row.join(' '));
	});

	QPushButton *btnSqlClearNotes = new QPushButton(" clear notes ");
	hlo1->addWidget(btnSqlClearNotes);
	connect(btnSqlClearNotes,&QPushButton::clicked,[](){
		QMbError("disabled");
		//DataBase::DoSqlQuery("delete from " + Fields::Notes());
	});

	QPushButton *btnDB = new QPushButton(" DB ");
	hlo1->addWidget(btnDB);
	connect(btnDB,&QPushButton::clicked,[](){
		MyQExecute::Execute(DataBase::baseDataCurrent->baseFilePathName);
	});

	hlo1->addWidget(new QLabel("answ delay:"));
	leAnswDelay = new QLineEdit;
	hlo1->addWidget(leAnswDelay);
	connect(leAnswDelay, &QLineEdit::textChanged, [this](const QString &text){ answDelay = text.toUInt(); });

	hlo1->addStretch();

	hlo2->addWidget(new QLabel("session:"));
	auto leSesssion = new QLineEdit;
	hlo2->addWidget(leSesssion);
	hlo2->addWidget(new QLabel("message:"));
	auto leMessage = new QLineEdit;
	hlo2->addWidget(leMessage);
	QPushButton *btnWrite = new QPushButton(" Write ");
	hlo2->addWidget(btnWrite);
	connect(btnWrite, &QPushButton::clicked, [this, leSesssion, leMessage](){
		if(!SessionData::mapIdAndSession.count(leSesssion->text().toUInt()))
		{
			Error("session not found");
			return;
		}
		auto session = SessionData::mapIdAndSession[leSesssion->text().toUInt()];
		if(!session->activeSockets.size())
		{
			Error("session has no active sockets");
			return;
		}

		HttpClient *cl = *session->activeSockets.begin();
		SendInSock(cl, leMessage->text().isEmpty() ? "empty text" : leMessage->text(), true);
	});

	hlo2->addStretch();

	textEdit = new QTextEdit;
	textEdit->setTabStopDistance(40);
	hlo3->addWidget(textEdit);

	new TextEditCleaner(textEdit, 10000, textEdit);

	ConnectDB();
	StartServer();

	resize(650,600);
	QTimer::singleShot(0,[this](){
		move(-1850, 10);
		if(qApp->screens().size() == 1) move(10, 10);
	});
}

void WidgetServer::ConnectDB()
{
	BaseData baseData = DataBase::defineBase(DataBase::server);
	QString fileWithDB = MyQDifferent::PathToExe() + "/files/db.txt";
	if(QFileInfo(fileWithDB).isFile())
	{
		auto readRes = MyQFileDir::ReadFile2(fileWithDB);
		if(!readRes.success) { Error("cant read file fileWithDB"); return; }

		readRes.content.remove('\r');
		auto rows = readRes.content.split("\n");
		if(rows.size() != 3) { Error("bad rows size " + QSn(rows.size())); return; }
		baseData = BaseData(rows[0], rows[1], rows[2]);
	}
	else Log("File with base info not found, default will be try to load.\nFile should be\n"
				 + fileWithDB + "\nand should contain 3 rows:\n\tbaseName(just name)\n\tbaseFilePathName\n\tstoragePath");

	DataBase::Init(baseData, {}, [this](const QString &str){ Log(str); }, [this](const QString &str){ Error(str); } );
	DataBase::InitChildDataBase(DataBase::server);
	DataBase::BackupBase();
}

void WidgetServer::StartServer()
{
	connect(&server, &HttpServer::SignalNewConnection, this, &WidgetServer::SlotNewConnection);

	server.logFoo = [this](const QString &str){
		QMetaObject::invokeMethod(textEdit, [this, str]() { Log(str); });
	};
	server.errorFoo = [this](const QString &str){
		QMetaObject::invokeMethod(textEdit, [this, str]() { Error(str); });
	};

	int port = 25002;
	server.start(port);
}

void WidgetServer::Log(const QString &str, bool appendInLastRow)
{
	if(appendInLastRow) MyQTextEdit::AppendInLastRow(textEdit, str);
	else textEdit->append(str);
	MyQTextEdit::ColorizeLastCount(textEdit, Qt::black, str.size());
	//qdbg << "log" << str;
}

void WidgetServer::Error(const QString &str)
{
	textEdit->append(str);
	MyQTextEdit::ColorizeLastCount(textEdit, Qt::red, str.size());
	//qdbg << "Error" << str;
}

void WidgetServer::Warning(const QString &str)
{
	textEdit->append(str);
	MyQTextEdit::ColorizeLastCount(textEdit, Qt::blue, str.size());
	//qdbg << "Warning" << str;
}

void WidgetServer::SlotNewConnection(HttpClient *sock)
{
	connect(sock, &HttpClient::SignalDisconnected, [this, sock]()
	{
		Log("disconnected: " + MyQString::AsDebug(sock));
	});
	connect(sock, &HttpClient::SignalReadyRead, this, &WidgetServer::SlotReadClient);

	sock->logFoo = [this](const QString &str){ QMetaObject::invokeMethod(textEdit, [this, str]() { Log(str); }); };
	sock->errorFoo = [this](const QString &str){ QMetaObject::invokeMethod(textEdit, [this, str]() { Error(str); }); };

	Log("new connection processed: " + MyQString::AsDebug(sock));
}

void WidgetServer::SlotReadClient()
{
	HttpClient *sock = (HttpClient*)sender();

	if(!sock->addedInSocksWitoutSession) {
		socketsWithotSession.insert(sock);
		sock->addedInSocksWitoutSession = true;
		connect(sock, &HttpClient::SignalDisconnected, this, [this, sock](){
			if(!sock->removedFromSocksWitoutSession)
				socketsWithotSession.erase(sock);
		});
	}

	//Log("SlotReadClient");
	//Log("received:\n" + HttpCommon::GetFullText(sock->request));
	//Log("received target: " + sock->ReadTarget());
	//Log("received body: " + sock->ReadBody());`

	QString target = sock->ReadTarget();
	QString body = sock->ReadBody();

	auto targetContent = NetClient::DecodeTarget(target);
	if(!NetClient::CheckTargetContent(targetContent))
	{
		Warning("get bad target " + target);
		SendInSock(sock, NetConstants::msg_error() + "bad target", true);
		return;
	}

	// AuthCheck
	if(!AuthCheck(sock, targetContent)) return;
	// SessionWork
	if(!SessionWork(sock, targetContent)) return;

	if(body.endsWith(';')) body.chop(1);
	else
	{
		Error("get unfinished data ["+body+"]");
		return;
	}
	auto msgs = body.split(';');

	for(auto &msg:msgs)
	{
		msg.replace(NetConstants::end_marker_replace(), NetConstants::end_marker());
		Log("received message: " + msg);

		if(msg.startsWith(NetConstants::msg()))
		{
			MsgsWorker(sock, std::move(msg));
		}
		else if(msg.startsWith(NetConstants::request()))
		{
			RequestsWorker(sock, std::move(msg));
		}
		else if(msg.startsWith(NetConstants::request_answ()))
		{
			RequestsAnswersWorker(std::move(msg));
		}
		else
		{
			Warning("received unexpacted data from client {" + msg + "}");
//			static int a = 0;
//			if(a++ % 2 != 0) return;
			SendInSock(sock, NetConstants::unexpected(), true);
//			MyCppDifferent::sleep_ms(100);
//			SendInSock(sock, "1fghfghfgh11", true);
//			MyCppDifferent::sleep_ms(100);
//			SendInSock(sock, "2222", true);
//			MyCppDifferent::sleep_ms(100);
//			SendInSock(sock, "33333", true);
		}
	}
}

bool WidgetServer::AuthCheck(HttpClient *sock, const NetClient::TargetContent &targetContent)
{
	bool authRes = NetClient::ChekAuth(targetContent);
	Log(QString("auth res: ").append(authRes ? "success" : "fail"));
	if(authRes) sock->authFailCount = 0;
	else
	{
		sock->authFailCount++;
		if(sock->authFailCount > 5)
		{
			sock->socket->close();
			return false;
		}
		SendInSock(sock, NetConstants::auth_failed(), true);
		return false;
	}

	return true;
}

bool WidgetServer::SessionWork(HttpClient *sock, const NetClient::TargetContent &targetContent)
{
	// клиент прислал undefinedSessionIdStr
	if(targetContent.sessionId == NetClient::undefinedSessionIdStr())
	{
		if(0) CodeMarkers::to_do("нужно проверять имеет ли право клиент получить сессию");

		if(sock->sessionPtr) // если это известный клиент
		{
			Command_to_client_your_session_id(sock);
			if(0) CodeMarkers::to_do("а какого хера он это присылает???");
			Warning("for existing client " + MyQString::AsDebug(sock) + " send your_session_id " + QSn(sock->sessionPtr->id));
		}
		else // если это новый клиент
		{
			BindSocketToSession(sock, nullptr, true);
			Log("for new client " + MyQString::AsDebug(sock) + " created session " + QSn(sock->sessionPtr->id));
		}
	}
	// клиент прислал конкретный sessionId
	else
	{
		// этот сокет уже известен серверу
		if(sock->sessionPtr)
		{
			if(sock->sessionPtr->CmpWithTarget(targetContent))
				Log("existing client " + MyQString::AsDebug(sock) + " using existing session id " + targetContent.sessionId);
			else {
				Error("existing client " + MyQString::AsDebug(sock) + " not passed session check ("
					  + QSn(sock->sessionPtr->id)+" != "+targetContent.sessionId
					  + " or " + sock->sessionPtr->dt+" ? "+targetContent.sessionDt + ")");
				SendInSock(sock, NetConstants::msg_error(), true);
				return false;
			}
		}
		// это неизвестный для сервера сокет
		else
		{
			auto idFromTarget = targetContent.sessionId.toLongLong();
			SessionData *sessionPtr = nullptr;
			if(auto it = SessionData::mapIdAndSession.find(idFromTarget); it != SessionData::mapIdAndSession.end())
				sessionPtr = it->second;

			// sessionId от клиента известен и параметры сессии совпадают
			if(sessionPtr && sessionPtr->CmpWithTarget(targetContent))
			{
				// присоединение сокета к существующей сессии
				if(0) CodeMarkers::to_do("нужно проверять имеет ли право клиент стать частью этой сессии");

				BindSocketToSession(sock, sessionPtr, false);
				Log("new client " + MyQString::AsDebug(sock) + " using existing session id " + QSn(idFromTarget));
			}
			else
			{
				BindSocketToSession(sock, nullptr, true);
				Log("new client " + MyQString::AsDebug(sock) + " not passed session check for existing session id "
					+ QSn(idFromTarget) + "; created new session " + QSn(sock->sessionPtr->id));
			}
		}

		if(0) CodeMarkers::to_do("нужно удалять не актуальные sock из мапы");
		if(0) CodeMarkers::can_be_optimized("можно убрать предобразование строки в число");
	}

	return true;
}

void WidgetServer::Write(ISocket *sock, const QString &str)
{
	auto castedSock = static_cast<HttpClient*>(sock);
	if(auto it = mapClientSession.find(castedSock); it != mapClientSession.end())
	{
		SessionData *sessionPtr = it->second;
		if(sessionPtr->activeSockets.count(castedSock))
			castedSock->Write(QString(str));
		else if(!sessionPtr->activeSockets.empty())
			(*sessionPtr->activeSockets.begin())->Write(QString(str));
		else Warning("Write in dead session; id session = " + QSn(sessionPtr->id));
	}
	else if(socketsWithotSession.count(castedSock) != 0)
	{
		castedSock->Write(QString(str));
	}
	else Error("WidgetServer::Write executed with sock not found in mapClientSession");

	if(0) CodeMarkers::to_do("optimisation QString(str)");
}

void WidgetServer::BindSocketToSession(HttpClient *sock, SessionData *session, bool sendYourSessionId)
{
	if(!session)
	{
		session = SessionData::MakeNewSession();
		sendYourSessionId = true;
	}

	if(!sock->removedFromSocksWitoutSession)
	{
		socketsWithotSession.erase(sock);
		sock->removedFromSocksWitoutSession = true;
	}

	session->activeSockets.insert(sock);
	mapClientSession[sock] = session;
	sock->sessionPtr = session;

	if(!sock->connectDid_ErasingActiveSock)
	{
		connect(sock, &HttpClient::SignalDisconnected, this, [this, sock](){
			//mapClientSession.erase(sock); - не нужно удалять, чтобы можно было по умершему клиенту найти живую сессию
			if(!sock->sessionPtr) { Error("HttpClient::SignalDisconnected null sock->sessionPtr"); return; }
			sock->sessionPtr->activeSockets.erase(sock);
		});
		sock->connectDid_ErasingActiveSock = true;
	}

	if(sendYourSessionId)
	{
		Command_to_client_your_session_id(sock);
	}
}

void WidgetServer::MsgsWorker(ISocket * sock, QString text)
{
	auto msgData = NetClient::DecodeMsg(text);
	if(!msgData.errors.isEmpty())
	{
		Error("error decoding msg: "+msgData.errors + "; full text:\n"+text);
		SendInSock(sock, NetConstants::msg_error() + " error decoding msg", true);
		return;
	}

	Log("get msg "+msgData.type+", start work");

	if(auto it = msgWorkersMap.find(msgData.type); it != msgWorkersMap.end())
	{
		std::function<void(ISocket *sock, QString &&)> &requestWorker = it->second;
		requestWorker(sock, std::move(msgData.content));
	}
	else
	{
		Error("Unrealesed msgData.type " + msgData.type);
		SendInSock(sock, NetConstants::msg_error() + " Unrealesed msgData.type " + msgData.type, true);
	}
}

void WidgetServer::msg_error_worker(ISocket * sock, QString && msgContent)
{
	msgContent.prepend("Geted msg about error from client: ");
	Error(msgContent);
	SendInSock(sock, NetConstants::nothing(), true);
}

void WidgetServer::msg_all_client_notes_synch_sended_worker(ISocket * sock, QString && /*msgContent*/)
{
	Error("//auto &clientData = clientsDatas[sock];");
	//auto &clientData = clientsDatas[sock];
	//clientData.lastSynchedNotesIdsOnServer.clear();
	Log("msg_all_client_notes_synch_sended get, lastSynchedNotesIdsOnServer cleared");
	SendInSock(sock, NetConstants::nothing(), true);
}

void WidgetServer::msg_highest_idOnServer_worker(ISocket * sock, QString && msgContent)
{
	auto table = DataBase::NotesWithHigherIdOnServer(msgContent);
	Log("msg_highest_idOnServer get, count notes higher than on client: " + QSn(table.size()) + "; sending updates");
	for(auto &row:table)
	{
		Command_to_client_update_note(sock, Note::CreateFromRecord(row));
	}

	if(0) CodeMarkers::to_do("тут еще нужно анализировать на какие группы подписан клиент чтобы не передавать ему лишнего");
	SendInSock(sock, NetConstants::nothing(), true);
	if(0) CodeMarkers::to_do("приходится вручную следить чтобы сервер отвечал на каждое сообщение, "
							 "это должно быть как-то автоматизированно");
}

void WidgetServer::RequestGetNote(ISocket * sock, QString idOnServer)
{
	AnswerWorkerFunction answFoo = [this, idOnServer](QString &&answContent) {
		if(answContent == NetConstants::invalid())
		{
			Error("answ for RequestGetNote came invalid");
			return;
		}

		auto note_uptr = Note::LoadNote(answContent);
		if(note_uptr)
		{
			Log("Note loaded from reseived data:\n" + note_uptr->ToStrForLog());
			if(DataBase::SaveNoteOnServer(note_uptr.get()))
			{
				Log("note "+note_uptr->Name()+" saved");
			}
			else
			{
				Error("note "+note_uptr->Name()+" tryed to save, but can't");
			}
		}
		else
		{
			Error("can't load note from reseived data:\n" + answContent);
		}
	};

	RequestInSock(sock, NetConstants::request_get_note(), std::move(idOnServer), std::move(answFoo));
}

void WidgetServer::request_get_session_worker(ISocket *sock, Requester::RequestData &&requestData)
{
	if(auto castedSock = dynamic_cast<HttpClient*>(sock))
	{
		if(!castedSock->sessionPtr)
		{
			Error("request_get_session_id_worker : !castedSock->sessionPtr");
			AnswerForRequestSending(sock, std::move(requestData), NetConstants::msg_error());
		}
		else if(!NetClient::IsSessionIdValid(castedSock->sessionPtr->id))
		{
			Error("request_get_session_id_worker : invalid session");
			AnswerForRequestSending(sock, std::move(requestData), NetConstants::msg_error());
		}
		else
		{
			AnswerForRequestSending(sock, std::move(requestData),
				NetConstants::MakeAnsw_request_get_session(QSn(castedSock->sessionPtr->id), castedSock->sessionPtr->dt));
		}
	}
	else { Error("request_get_session_id_worker executed with sock not HttpClient"); }
}

void WidgetServer::request_try_create_group_worker(ISocket * sock, NetClient::RequestData && requestData)
{
	QString newGroupName = requestData.content;
	int id = DataBase::TryCreateNewGlobalGroup(newGroupName, {});
	if(id < 0)
	{
		Log("Group "+newGroupName+" not created");
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
	}
	else
	{
		Log("Group "+newGroupName+" created");
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::MakeAnsw_try_create_group(QSn(id)));
	}
}

void WidgetServer::request_create_note_on_server_worker(ISocket * sock, NetClient::RequestData && requestData)
{
	auto tmpNoteUptr = Note::LoadNote(requestData.content);
	if(!tmpNoteUptr) {
		Error("cant load note from data:\n" + requestData.content);
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
		return;
	}

	auto idOnServer = DataBase::InsertNoteInServerDB(tmpNoteUptr.get());
	if(idOnServer < 0)
	{
		Error("Note "+tmpNoteUptr->Name()+" not created");
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
	}
	else
	{
		Log("Note "+tmpNoteUptr->Name()+" created");
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::MakeAnsw_create_note_on_server(QSn(idOnServer)));
	}
}

void WidgetServer::request_move_note_to_group_worker(ISocket *sock, NetClient::RequestData && requestData)
{
	auto decoded = NetConstants::GetDataFromRequest_move_note_to_group(requestData.content);
	if(decoded.idNoteOnServer.isEmpty() || decoded.idNewGroup.isEmpty() || decoded.dtLastUpdatedStr.isEmpty())
	{
		Error("GetIdNoteOnServerAndIdNewGroupFromRequest_move_note_to_group empty res");
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
		return;
	}
	if(!DataBase::CheckNoteIdOnServer(decoded.idNoteOnServer))
	{
		Error("CheckNoteId "+decoded.idNoteOnServer+" false");
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
		return;
	}

	// в локальную группу, удаляем
	if(decoded.idNewGroup == DataBase::IsGroupLocalById(decoded.idNewGroup))
	{
		if(DataBase::RemoveNoteOnServer(decoded.idNoteOnServer, true))
		{
			Log("note "+decoded.idNoteOnServer+" moved to default group, server removed it");
			AnswerForRequestSending(sock, std::move(requestData), NetConstants::success());
		}
		else
		{
			Error("note tryed "+decoded.idNoteOnServer+"move to default group, but server remove it returned false");
			AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
		}
	}
	else // не в локальную, перемещаем
	{
		if(DataBase::MoveNoteToGroupOnServer(decoded.idNoteOnServer, decoded.idNewGroup, decoded.dtLastUpdatedStr))
		{
			Log("note "+decoded.idNoteOnServer+" moved to group "+decoded.idNewGroup);
			AnswerForRequestSending(sock, std::move(requestData), NetConstants::success());
		}
		else
		{
			Error("note tryed "+decoded.idNoteOnServer+"move to group "+decoded.idNewGroup+", but server can't");
			AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
		}
	}
}

void WidgetServer::request_remove_note_worker(ISocket * sock, NetClient::RequestData && requestData)
{
	if(DataBase::RemoveNoteOnServer(requestData.content, true))
	{
		Log("note "+requestData.content+" removed");
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::success());
	}
	else
	{
		Error("note tryed "+requestData.content+" to remove, but can't");
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
	}
}

void WidgetServer::request_note_saved_worker(ISocket * sock, NetClient::RequestData && requestData)
{
	auto note_uptr = Note::LoadNote(requestData.content);
	if(note_uptr)
	{
		Log("Note loaded from reseived data:\n" + note_uptr->ToStrForLog());
		if(DataBase::SaveNoteOnServer(note_uptr.get()))
		{
			Log("note "+note_uptr->Name()+" saved");
			AnswerForRequestSending(sock, std::move(requestData), NetConstants::success());
		}
		else
		{
			Error("note "+note_uptr->Name()+" tryed to save, but can't");
			AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
		}
	}
	else
	{
		Error("can't load note from reseived data:\n" + requestData.content);
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
	}
}

void WidgetServer::request_synch_note_worker(ISocket * sock, NetClient::RequestData && requestData)
{
	Log("request_synch_note_worker: "+requestData.content);

	auto datas = NetConstants::GetDataFromRequest_synch_note(requestData.content);
	if(datas.empty()) {
		Error("request_synch_note_worker get empty datas from content: " + requestData.content);
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
		return;
	}

	QString answ;
	for(uint i=0; i<datas.size(); i++) answ.append(NetConstants::request_synch_res_success()).append(',');
	answ.chop(1);
	AnswerForRequestSending(sock, std::move(requestData), answ);

	for(auto &data:datas)
	{
		auto noteRec = DataBase::NoteByIdOnServer(data.idOnServer);
		if(noteRec.isEmpty()) // заметка на сервере отсутсвует
		{
			/// значит она была удалена другим клиентом
			/// отправляем клинету комунду удалить заметку
			Log("note "+data.idOnServer+" not found on server, give client command to remove it");
			Command_to_client_remove_note(sock, data.idOnServer);
		}
		else
		{
			Note note;
			note.InitFromRecord(noteRec);

			Error("//auto &clientData = clientsDatas[sock];");
			//auto &clientData = clientsDatas[sock];
			//clientData.lastSynchedNotesIdsOnServer.emplace_back(note.idOnServer);

			if(0) CodeMarkers::to_do_with_cpp20("replace two compare < > to one <=>");
			else if(note.DtLastUpdatedStr() < data.dtUpdatedStr)
			{
				/// заметка на сервере отстаёт, запрашиваем у клиента полную информацию о заметке
				Log("note "+note.Name()+" is old on server, request to client for update");
				RequestGetNote(sock, QSn(note.idOnServer));

			}
			else if(note.DtLastUpdatedStr() > data.dtUpdatedStr)
			{
				/// заметка на клиенте отстаёт, отправляем клиенту команду обновить заметку
				Log("note "+note.Name()+" is old on client, commad to client for update");
				Command_to_client_update_note(sock, note);
			}
			else if(note.DtLastUpdatedStr() == data.dtUpdatedStr)
			{
				// если даты совпадают ничего делать не надо
				Log("note "+note.Name()+" has same update on server and client");
			}
		}
	}
}

void WidgetServer::request_get_note_worker(ISocket *sock, Requester::RequestData &&requestData)
{
	QString &idOnServer = requestData.content;
	auto [check, rec] = DataBase::NoteByIdOnServerWithCheck(idOnServer);
	if(!check) {
		Error("request_get_note_worker not found note by id: " + requestData.content);
		AnswerForRequestSending(sock, std::move(requestData), NetConstants::not_did());
		return;
	}

	AnswerForRequestSending(sock, std::move(requestData), Note::InitFromRecordAndSaveToStr(rec));
}

void WidgetServer::request_group_check_notes_worker(ISocket *sock, Requester::RequestData &&requestData)
{
	Error("request_group_check_notes_worker deprecated");
//	auto [ok, idGroup, idsNotes_dtsLastUpdated] = NetConstants::request_group_check_notes_decode(requestData.content);
//	for(auto &[idNote,dtUpdated]:idsNotes_dtsLastUpdated)
//	{

//	}
	AnswerForRequestSending(sock, std::move(requestData), NetConstants::success());
}

void WidgetServer::request_all_notes_worker(ISocket *sock, Requester::RequestData &&requestData)
{
	auto notesTable = DataBase::DoSqlQueryGetTable("select * from " + Fields::Notes());
	AnswerForRequestSending(sock, std::move(requestData), NetConstants::request_all_notes_prepare(notesTable));
}

void WidgetServer::request_polly_worker(ISocket *sock, Requester::RequestData &&requestData)
{
	if(auto castedSock = dynamic_cast<HttpClient*>(sock))
	{
		castedSock->pollyRequestGetAt = QDateTime::currentDateTime();
		//Log(">>> HttpClient::pollyRequestGetA: " + HttpClient::pollyRequestGetAt.toString(DateTimeFormat_ms));
		castedSock->pollyWriter = castedSock;
		castedSock->pollyRequestData = std::move(requestData);

		// если есть что отправить - отправляем
		if(!castedSock->waitsPollyToWrite.empty())
		{
			QString toWrite = std::move(castedSock->waitsPollyToWrite.front());
			castedSock->waitsPollyToWrite.pop();
			Log("request_polly_worker: sending by polly: " + (toWrite == " " ? "_space_" : toWrite));
			castedSock->Write(std::move(toWrite), true);
		}
		// если нет ничего на отправку - ничего не делаем, запрос будет в ожидании
		else
		{
			//Log("request_polly_worker waitsPollyToWrite is empty");
		}
	}
	else { Error("WidgetServer::request_polly_worker executed with sock not HttpClient"); }
}

void WidgetServer::Command_to_client_your_session_id(HttpClient *sock)
{
	if(!sock->sessionPtr) Error("Command_to_client_new_session_id, but null sock->sessionPtr");
	else SendInSock(sock,
					NetClient::PrepareCommandToClient(
						NetConstants::command_your_session_id(),
						NetConstants::MakeAnsw_request_get_session(QSn(sock->sessionPtr->id), sock->sessionPtr->dt)),
					true);
}

void WidgetServer::Command_to_client_remove_note(ISocket * sock, const QString & idOnClient)
{
	SendInSock(sock, NetClient::PrepareCommandToClient(NetConstants::command_remove_note(), idOnClient), true);
}

void WidgetServer::Command_to_client_update_note(ISocket * sock, const Note & note)
{
	QString command = NetClient::PrepareCommandToClient(NetConstants::command_update_note(), note.ToStr_v1());
	SendInSock(sock, command, true);
}

