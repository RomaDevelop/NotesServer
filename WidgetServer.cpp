#include "WidgetServer.h"

#include <QDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCryptographicHash>

#include "MyQDifferent.h"
#include "MyQFileDir.h"

WidgetServer::WidgetServer(QWidget *parent)
	: QWidget(parent)
{
	QVBoxLayout *vlo_main = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vlo_main->addLayout(hlo1);
	vlo_main->addLayout(hlo2);

	QPushButton *btn1 = new QPushButton("button1");
	hlo1->addWidget(btn1);
	connect(btn1,&QPushButton::clicked,[](){ qDebug() << "btn1"; });

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
		Log("get unfinished data ["+readed+"]");
		return;
	}
	auto commands = readed.split(';');

	for(auto &command:commands)
	{
		if(command.startsWith(Constants::auth()))
		{
			Log("auth data get, cheking passwd");

			QStringRef hashedPasswdFromClient(&command, Constants::auth().size(), command.size() - Constants::auth().size());

			QCryptographicHash hashCorretPasswd(QCryptographicHash::Md5);
			hashCorretPasswd.addData(Constants::test_passwd().toUtf8());

			if(hashCorretPasswd.result().toHex() == hashedPasswdFromClient)
			{
				Log(Constants::auth_success());
				SendToClient(sock, Constants::auth_success());

				if(lastUpdate.isValid())
					SendToClient(sock, Constants::last_update() + lastUpdate.toString(DateTimeFormat_ms));
				else SendToClient(sock, Constants::last_update() + Constants::invalid());
			}
			else
			{
				Log(Constants::auth_failed());
				SendToClient(sock, Constants::auth_failed());
			}
		}
		else
		{
			Log("received unexpacted data from client {" + command + "}");
			SendToClient(sock, Constants::unexpacted());
		}
	}
}

void WidgetServer::SendToClient(QTcpSocket *sock, const QString &str)
{
	sock->write(str.toUtf8());
	sock->write(";");
}

