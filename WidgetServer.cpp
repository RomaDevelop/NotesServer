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

#include "Note.h"
#include "DataBase.h"

WidgetServer::WidgetServer(QWidget *parent)
	: QWidget(parent)
{
	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

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
		DataBase::DoSqlQuery("delete from " + Fields::Notes());
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

	textEdit = new QTextEdit;
	textEdit->setTabStopDistance(40);
	hlo2->addWidget(textEdit);

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
		QMetaObject::invokeMethod(textEdit, [this, str]() {
			textEdit->append(str);
		});
	};

	int port = 25002;
	server.start(port);
}

void WidgetServer::Log(const QString &str, bool appendInLastRow)
{
	if(appendInLastRow) MyQTextEdit::AppendInLastRow(textEdit, str);
	else textEdit->append(str);
	MyQTextEdit::ColorizeLastCount(textEdit, Qt::black, str.size());
}

void WidgetServer::Error(const QString &str)
{
	textEdit->append(str);
	MyQTextEdit::ColorizeLastCount(textEdit, Qt::red, str.size());
}

void WidgetServer::Warning(const QString &str)
{
	textEdit->append(str);
	MyQTextEdit::ColorizeLastCount(textEdit, Qt::blue, str.size());
}

void WidgetServer::SlotNewConnection(HttpClient *sock)
{
	connect(sock, &HttpClient::SignalDisconnected, [this, sock]()
	{
		Log("disconnected: "/* + MyQString::AsDebug(sock)*/);
		clientsDatas.erase(sock);
	});
	connect(sock, &HttpClient::SignalReadyRead, this, &WidgetServer::SlotReadClient);
	clientsDatas[sock] = {};

	Log("new connection processed: " + MyQString::AsDebug(sock));
}

void WidgetServer::SlotReadClient()
{
	HttpClient *sock = (HttpClient*)sender();

	//Log("received:\n" + HttpCommon::GetFullText(sock->request));
	//Log("received target: " + sock->ReadTarget());
	//Log("received body: " + sock->ReadBody());

	QString target = sock->ReadTarget();
	QString readed = sock->ReadBody();

	bool authRes = NetClient::ChekAuth(target);
	Log("auth res: "+QSn(authRes));
	if(!authRes) { Log(", closing", true); sock->socket->close(); return; }

	if(readed.endsWith(';')) readed.chop(1);
	else
	{
		Error("get unfinished data ["+readed+"]");
		return;
	}
	auto commands = readed.split(';');

	for(auto &command:commands)
	{
		command.replace(NetConstants::end_marker_replace(), NetConstants::end_marker());
		Log("received command: " + command);

		if(command.startsWith(NetConstants::msg()))
		{
			MsgsWorker(sock, std::move(command));
		}
		else if(command.startsWith(NetConstants::request()))
		{
			RequestsWorker(sock, std::move(command));
		}
		else if(command.startsWith(NetConstants::request_answ()))
		{
			RequestsAnswersWorker(std::move(command));
		}
		else
		{
			Warning("received unexpacted data from client {" + command + "}");
			SendInSock(sock, NetConstants::unexpacted(), true);
		}
	}
}

void WidgetServer::Write(ISocket *sock, const QString &str)
{
	if(auto castedSock = dynamic_cast<HttpClient*>(sock))
	{
		castedSock->bodyToWrite = str;
		Log(castedSock->bodyToWrite);
		castedSock->readyWrite = true;
	}
	else { Error("WidgetServer::Write executed with sock not HttpClient"); }
}

void WidgetServer::MsgsWorker(ISocket * sock, QString text)
{
	auto msgData = NetClient::DecodeMsg(text);
	if(!msgData.errors.isEmpty())
	{
		Error("error decoding msg: "+msgData.errors + "; full text:\n"+text);
		return;
	}

	Log("get msg "+msgData.type+", start work");

	if(auto it = msgWorkersMap.find(msgData.type); it != msgWorkersMap.end())
	{
		std::function<void(ISocket *sock, QString &&)> &requestWorker = it->second;
		requestWorker(sock, std::move(msgData.content));
	}
	else Error("Unrealesed requestData.type " + msgData.type);
}

void WidgetServer::msg_error_worker(ISocket * /*sock*/, QString && msgContent)
{
	msgContent.prepend("Geted msg about error from client: ");
	Error(msgContent);
}

void WidgetServer::msg_all_client_notes_synch_sended_worker(ISocket * /*sock*/, QString && /*msgContent*/)
{
	Error("//auto &clientData = clientsDatas[sock];");
	//auto &clientData = clientsDatas[sock];
	//clientData.lastSynchedNotesIdsOnServer.clear();
	Log("msg_all_client_notes_synch_sended get, lastSynchedNotesIdsOnServer cleared");
}

void WidgetServer::msg_highest_idOnServer_worker(ISocket * sock, QString && msgContent)
{
	auto table = DataBase::NotesWithHigherIdOnServer(msgContent);
	Log("msg_highest_idOnServer get, count notes higher than on client: " + QSn(table.size()));
	for(auto &row:table)
	{
		Command_to_client_update_note(sock, Note::CreateFromRecord(row));
	}
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

void WidgetServer::request_try_create_group_worker(ISocket * sock, NetClient::RequestData && requestData)
{
	QString newGroupName = requestData.content;
	int id = DataBase::TryCreateNewGlobalGroup(newGroupName, {});
	if(id < 0)
	{
		Log("Group "+newGroupName+" not created");
		AnswerForRequestSending(sock, requestData, NetConstants::not_did());
	}
	else
	{
		Log("Group "+newGroupName+" created");
		AnswerForRequestSending(sock, requestData, NetConstants::MakeAnsw_try_create_group(QSn(id)));
	}
}

void WidgetServer::request_create_note_on_server_worker(ISocket * sock, NetClient::RequestData && requestData)
{
	auto tmpNoteUptr = Note::LoadNote(requestData.content);
	if(!tmpNoteUptr) {
		Error("cant load note from data:\n" + requestData.content);
		AnswerForRequestSending(sock, requestData, NetConstants::not_did());
		return;
	}

	auto idOnServer = DataBase::InsertNoteInServerDB(tmpNoteUptr.get());
	if(idOnServer < 0)
	{
		Error("Note "+tmpNoteUptr->Name()+" not created");
		AnswerForRequestSending(sock, requestData, NetConstants::not_did());
	}
	else
	{
		Log("Note "+tmpNoteUptr->Name()+" created");
		AnswerForRequestSending(sock, requestData, NetConstants::MakeAnsw_create_note_on_server(QSn(idOnServer)));
	}
}

void WidgetServer::request_move_note_to_group_worker(ISocket *sock, NetClient::RequestData && requestData)
{
	auto decoded = NetConstants::GetDataFromRequest_move_note_to_group(requestData.content);
	if(decoded.idNoteOnServer.isEmpty() || decoded.idNewGroup.isEmpty() || decoded.dtLastUpdatedStr.isEmpty())
	{
		Error("GetIdNoteOnServerAndIdNewGroupFromRequest_move_note_to_group empty res");
		AnswerForRequestSending(sock, requestData, NetConstants::not_did());
		return;
	}
	if(!DataBase::CheckNoteIdOnServer(decoded.idNoteOnServer))
	{
		Error("CheckNoteId "+decoded.idNoteOnServer+" false");
		AnswerForRequestSending(sock, requestData, NetConstants::not_did());
		return;
	}

	if(decoded.idNewGroup == DataBase::DefaultGroupId2()) // в дефолтную группу, удаляем
	{
		if(DataBase::RemoveNoteOnServer(decoded.idNoteOnServer, true))
		{
			Log("note "+decoded.idNoteOnServer+" moved to default group, server removed it");
			AnswerForRequestSending(sock, requestData, NetConstants::success());
		}
		else
		{
			Error("note tryed "+decoded.idNoteOnServer+"move to default group, but server remove it returned false");
			AnswerForRequestSending(sock, requestData, NetConstants::not_did());
		}
	}
	else // не в дефолтную, перемещаем
	{
		if(DataBase::MoveNoteToGroupOnServer(decoded.idNoteOnServer, decoded.idNewGroup, decoded.dtLastUpdatedStr))
		{
			Log("note "+decoded.idNoteOnServer+" moved to group "+decoded.idNewGroup);
			AnswerForRequestSending(sock, requestData, NetConstants::success());
		}
		else
		{
			Error("note tryed "+decoded.idNoteOnServer+"move to group "+decoded.idNewGroup+", but server can't");
			AnswerForRequestSending(sock, requestData, NetConstants::not_did());
		}
	}
}

void WidgetServer::request_remove_note_worker(ISocket * sock, NetClient::RequestData && requestData)
{
	if(DataBase::RemoveNoteOnServer(requestData.content, true))
	{
		Log("note "+requestData.content+" removed");
		AnswerForRequestSending(sock, requestData, NetConstants::success());
	}
	else
	{
		Error("note tryed "+requestData.content+" to remove, but can't");
		AnswerForRequestSending(sock, requestData, NetConstants::not_did());
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
			AnswerForRequestSending(sock, requestData, NetConstants::success());
		}
		else
		{
			Error("note "+note_uptr->Name()+" tryed to save, but can't");
			AnswerForRequestSending(sock, requestData, NetConstants::not_did());
		}
	}
	else
	{
		Error("can't load note from reseived data:\n" + requestData.content);
		AnswerForRequestSending(sock, requestData, NetConstants::not_did());
	}
}

void WidgetServer::request_synch_note_worker(ISocket * sock, NetClient::RequestData && requestData)
{
	Log("request_synch_note_worker: "+requestData.content);

	auto datas = NetConstants::GetDataFromRequest_synch_note(requestData.content);
	if(datas.empty()) {
		Error("request_synch_note_worker get empty datas from content: " + requestData.content);
		AnswerForRequestSending(sock, requestData, NetConstants::not_did());
		return;
	}

	QString answ;
	for(uint i=0; i<datas.size(); i++) answ.append(NetConstants::request_synch_res_success()).append(',');
	answ.chop(1);
	AnswerForRequestSending(sock, requestData, answ);

	for(auto &data:datas)
	{
		auto noteRec = DataBase::NoteByIdOnServer(data.idOnSever);
		if(noteRec.isEmpty()) // заметка на сервере отсутсвует
		{
			/// значит она была удалена другим клиентом
			/// отправляем клинету комунду удалить заметку
			Log("note "+data.idOnSever+" not found on server, give client command to remove it");
			Command_to_client_remove_note(sock, data.idOnSever);
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

	/// Если на сервере имеются заметки которых нет у клиента -
	///		значит они были созданы другим клиентом создаем их на клиенте.
	///
	///		idOnServer в возрастающем порядке
	///		значит можно сверять максимальный idOnServer,
	///		ага, хуй там, заметка может быть в группе на которую не подписан клиент
	///		значит нужно смотреть макс айди по группам
	///		все равно какая-то дрочь, ведь заметки могут перемещаться по группам и изза этого хуйпойми
	///		поговорю с чатом как такое делают

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

