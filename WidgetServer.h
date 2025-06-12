#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QDateTime>
#include <QLineEdit>

#include "NetConstants.h"
#include "NetClient.h"

#include "Server.h"

struct ClientData
{
	std::vector<int> lastSynchedNotesIdsOnServer;
	QString msg;
};

class WidgetServer : public QWidget, public Requester
{
	Q_OBJECT

public:
	WidgetServer(QWidget *parent = nullptr);
	void ConnectDB();
	void StartServer();
	~WidgetServer() = default;

	virtual void Log(const QString &str, bool appendInLastRow = false) override;
	virtual void Error(const QString &str) override;
	virtual void Warning(const QString &str) override;

private:
	HttpServer server;
private slots:
	void SlotNewConnection(HttpClient *sock);
	void SlotReadClient();
private:
	void SendToClient(QTcpSocket *sock, QString str, bool sendEndMarker);

private:
	QTextEdit *textEdit;
	QLineEdit *leAnswDelay;

	QDateTime lastUpdate;

	std::map<HttpClient*, ClientData> clientsDatas;

	void MsgsWorker(QTcpSocket *sock, QString text);

	void msg_error_worker(QTcpSocket *sock, QString && msgContent);
	void msg_all_client_notes_synch_sended_worker(QTcpSocket *sock, QString && msgContent);
	void msg_highest_idOnServer_worker(QTcpSocket *sock, QString && msgContent);

	std::map<QStringRefWr_const, std::function<void(QTcpSocket *sock, QString && msgContent)>> msgWorkersMap {
		{ std::cref(NetConstants::msg_error()), [this](QTcpSocket *sock, QString && msgContent){ msg_error_worker(sock, std::move(msgContent)); } },
		{ std::cref(NetConstants::msg_all_client_notes_synch_sended()), [this](QTcpSocket *sock, QString && msgContent){ msg_all_client_notes_synch_sended_worker(sock, std::move(msgContent)); } },
		{ std::cref(NetConstants::msg_highest_idOnServer()), [this](QTcpSocket *sock, QString && msgContent){ msg_highest_idOnServer_worker(sock, std::move(msgContent)); } },
	};

	void RequestGetNote(QTcpSocket * sock, QString idOnServer);

	void request_try_create_group_worker(QTcpSocket *sock, NetClient::RequestData && requestData);
	void request_create_note_on_server_worker(QTcpSocket *sock, NetClient::RequestData && requestData);
	void request_move_note_to_group_worker(QTcpSocket *sock, NetClient::RequestData && requestData);
	void request_remove_note_worker(QTcpSocket *sock, NetClient::RequestData && requestData);
	void request_note_saved_worker(QTcpSocket *sock, NetClient::RequestData && requestData);
	void request_synch_note_worker(QTcpSocket *sock, NetClient::RequestData && requestData);

	std::map<QStringRefWr_const, std::function<void(QTcpSocket *sock, NetClient::RequestData &&requestData)>> requestWorkersMap {
		{ std::cref(NetConstants::request_try_create_group()), [this](QTcpSocket *sock, NetClient::RequestData &&requestData){ request_try_create_group_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_create_note_on_server()), [this](QTcpSocket *sock, NetClient::RequestData &&requestData){ request_create_note_on_server_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_move_note_to_group()), [this](QTcpSocket *sock, NetClient::RequestData &&requestData){ request_move_note_to_group_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_remove_note()), [this](QTcpSocket *sock, NetClient::RequestData &&requestData){ request_remove_note_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_note_saved()), [this](QTcpSocket *sock, NetClient::RequestData &&requestData){ request_note_saved_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_synch_note()), [this](QTcpSocket *sock, NetClient::RequestData &&requestData){ request_synch_note_worker(sock, std::move(requestData)); } },

	};
	virtual std::map<QStringRefWr_const, std::function<void(QTcpSocket *sock, RequestData &&requestData)>> & RequestWorkersMap() override { return requestWorkersMap; }

	void Command_to_client_remove_note(QTcpSocket *sock, const QString &idOnClient);
	void Command_to_client_update_note(QTcpSocket *sock, const Note &note);
};
#endif // WIDGET_H
