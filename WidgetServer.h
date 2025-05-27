#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>

#include "NetConstants.h"
#include "NetClient.h"

class WidgetServer : public QWidget
{
	Q_OBJECT

public:
	WidgetServer(QWidget *parent = nullptr);
	void CreateServer();
	~WidgetServer() = default;

	void Log(const QString &str);
	void Error(const QString &str);
	void Warning(const QString &str);

private:
	QTcpServer *server;
private slots:
	void SlotNewConnection();
	void SlotReadClient();
private:
	void SendToClient(QTcpSocket *sock, QString str, bool sendEndMarker);

private:
	QTextEdit *textEdit;

	QDateTime lastUpdate;
	
	void RequestsWorker(QTcpSocket *sock, QString text);
	void AnswerForRequest(QTcpSocket *sock, NetClient::RequestData &requestData, QString str);

	void NoteSavedWork(QString text);
};
#endif // WIDGET_H
