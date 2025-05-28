#include "WidgetServer.h"

#include <QDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QLabel>
#include <QTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCryptographicHash>

#include "MyQDifferent.h"
#include "MyQFileDir.h"
#include "MyQTextEdit.h"
#include "MyCppDifferent.h"
#include "CodeMarkers.h"

#include "Note.h"
#include "DataBase.h"

WidgetServer::WidgetServer(QWidget *parent)
	: QWidget(parent)
{
	DataBase::Init(serverBase, {}, [this](const QString &str){ Error(str); }, [this](const QString &str){ Log(str); } );
	DataBase::InitChildDataBase(DataBase::server);

	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

	QPushButton *btn1 = new QPushButton("clear");
	hlo1->addWidget(btn1);
	connect(btn1,&QPushButton::clicked,[this](){ textEdit->clear(); });

	QPushButton *btnSqlTest = new QPushButton("sql test");
	hlo1->addWidget(btnSqlTest);
	connect(btnSqlTest,&QPushButton::clicked,[this](){
		auto table = DataBase::DoSqlQueryGetTable("select * from " + Fields::Notes());
		for(auto &row:table) textEdit->append(row.join(' '));
		table = DataBase::DoSqlQueryGetTable("select * from " + Fields::Groups());
		for(auto &row:table) textEdit->append(row.join(' '));
	});

	hlo1->addWidget(new QLabel("answ delay:"));
	answDelay = new QLineEdit;
	hlo1->addWidget(answDelay);

	hlo1->addStretch();

	textEdit = new QTextEdit;
	hlo2->addWidget(textEdit);

	CreateServer();

	resize(800,600);
	QTimer::singleShot(0,[this](){ move(-1500, 100); });
}

void WidgetServer::CreateServer()
{
	int port = 25001;
	server = new QTcpServer(this);
	connect(server, &QTcpServer::newConnection, this, &WidgetServer::SlotNewConnection);

	if(!server->listen(QHostAddress::Any,port))
	{
		QMessageBox::critical(this,"","error listen " + server->errorString());
		server->close();
	}
}

void WidgetServer::Log(const QString &str)
{
	textEdit->append(str);
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

void WidgetServer::SlotNewConnection()
{
	QTcpSocket *sock = server->nextPendingConnection();
	connect(sock, &QTcpSocket::disconnected, sock, &QTcpSocket::deleteLater);
	connect(sock, &QTcpSocket::readyRead, this, &WidgetServer::SlotReadClient);
	textEdit->append("new connection processed");
}

void WidgetServer::SlotReadClient()
{
	QTcpSocket *sock = (QTcpSocket*)sender();
	QString readed = sock->readAll();
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

		if(command.startsWith(NetConstants::auth()))
		{
			Log("auth data get, cheking passwd");

			QStringRef hashedPasswdFromClient(&command, NetConstants::auth().size(), command.size() - NetConstants::auth().size());

			QCryptographicHash hashCorretPasswd(QCryptographicHash::Md5);
			hashCorretPasswd.addData(NetConstants::test_passwd().toUtf8());

			if(hashCorretPasswd.result().toHex() == hashedPasswdFromClient)
			{
				Log(NetConstants::auth_success());
				SendToClient(sock, NetConstants::auth_success(), true);

				if(lastUpdate.isValid())
					SendToClient(sock, NetConstants::last_update() + lastUpdate.toString(DateTimeFormat_ms), true);
				else SendToClient(sock, NetConstants::last_update() + NetConstants::invalid(), true);
			}
			else
			{
				Warning(NetConstants::auth_failed());
				SendToClient(sock, NetConstants::auth_failed(), true);
			}
		}
		else if(command.startsWith(NetConstants::note_saved()))
		{
			NoteSavedWork(std::move(command));
		}
		else if(command.startsWith(NetConstants::request()))
		{
			RequestsWorker(sock, std::move(command));
		}
		else
		{
			Warning("received unexpacted data from client {" + command + "}");
			SendToClient(sock, NetConstants::unexpacted(), true);
		}
	}
}

void WidgetServer::SendToClient(QTcpSocket *sock, QString str, bool sendEndMarker)
{
	CodeMarkers::can_be_optimized("takes copy, than make other copy");

	str.replace(NetConstants::end_marker(), NetConstants::end_marker_replace());

	sock->write(str.toUtf8());
	if(sendEndMarker) sock->write(";");
}

void WidgetServer::RequestsWorker(QTcpSocket *sock, QString text)
{
	CodeMarkers::can_be_optimized("give copy text to DecodeRequestCommand, but can move, but it used in error");
	auto requestData = NetClient::DecodeRequestCommand(text);
	if(!requestData.errors.isEmpty())
	{
		Error("error decoding request: "+requestData.errors + "; full text:\n"+text);
		SendToClient(sock, "error decoding request: "+requestData.errors, true);
		return;
	}

	Log("get reuqest "+requestData.type+" "+requestData.id+", start work");
	if(requestData.type == NetConstants::request_group_names())
	{
		AnswerForRequest(sock, requestData, "unrealesed");
		Error("request "+requestData.type+" "+requestData.id+" unrealesed");
	}
	else if(requestData.type == NetConstants::request_try_create_group())
	{
		QString newGroupName = requestData.content;
		int id = DataBase::TryCreateNewGroup(newGroupName, {});
		if(id < 0)
		{
			Log("Group "+newGroupName+" not created");
			AnswerForRequest(sock, requestData, NetConstants::not_did());
		}
		else
		{
			Log("Group "+newGroupName+" created");
			AnswerForRequest(sock, requestData, NetConstants::Answ_try_create_group(QSn(id)));
		}
	}
	else Error("Unrealesed requestData.type " + requestData.type);
}

void WidgetServer::AnswerForRequest(QTcpSocket *sock, NetClient::RequestData &requestData, QString str)
{
	MyCppDifferent::sleep_ms(answDelay->text().toUInt());
	SendToClient(sock, NetConstants::request_answ(), false);
	SendToClient(sock, std::move(requestData.id.append(" ")), false);
	SendToClient(sock, std::move(str), true);
}

void WidgetServer::NoteSavedWork(QString text)
{
	QStringRef noteText(&text, NetConstants::note_saved().size(), text.size()-NetConstants::note_saved().size());
	auto note_uptr = Note::LoadNote(noteText.toString(), "");
	if(!note_uptr) Log("cant load note from data:\n" + noteText.toString());
	else Log("Note loaded from data:\n" + note_uptr->ToStrForLog());
}

