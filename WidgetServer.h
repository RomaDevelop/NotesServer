#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>

#include "Constants.h"

class WidgetServer : public QWidget
{
	Q_OBJECT

public:
	WidgetServer(QWidget *parent = nullptr);
	void CreateServer();
	~WidgetServer() = default;

	void Log(const QString &str) { textEdit->append(str); }

private:
	QTcpServer *server;
private slots:
	void SlotNewConnection();
	void SlotReadClient();
private:
	void SendToClient(QTcpSocket *sock, const QString &str);

private:
	QTextEdit *textEdit;

	QDateTime lastUpdate;

};
#endif // WIDGET_H
