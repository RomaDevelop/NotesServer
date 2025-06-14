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

	virtual void Write(ISocket *sock, const QString &str) override;

private:
	QTextEdit *textEdit;
	QLineEdit *leAnswDelay;

	QDateTime lastUpdate;

	std::map<HttpClient*, ClientData> clientsDatas;

	void MsgsWorker(ISocket *sock, QString text);

	void msg_error_worker(ISocket *sock, QString && msgContent);
	void msg_all_client_notes_synch_sended_worker(ISocket *sock, QString && msgContent);
	void msg_highest_idOnServer_worker(ISocket *sock, QString && msgContent);

	std::map<QStringRefWr_const, std::function<void(ISocket *sock, QString && msgContent)>> msgWorkersMap {
		{ std::cref(NetConstants::msg_error()), [this](ISocket *sock, QString && msgContent){ msg_error_worker(sock, std::move(msgContent)); } },
		{ std::cref(NetConstants::msg_all_client_notes_synch_sended()), [this](ISocket *sock, QString && msgContent){ msg_all_client_notes_synch_sended_worker(sock, std::move(msgContent)); } },
		{ std::cref(NetConstants::msg_highest_idOnServer()), [this](ISocket *sock, QString && msgContent){ msg_highest_idOnServer_worker(sock, std::move(msgContent)); } },
	};

	void RequestGetNote(ISocket * sock, QString idOnServer);

	void request_try_create_group_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_create_note_on_server_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_move_note_to_group_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_remove_note_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_note_saved_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_synch_note_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_polly_worker(ISocket *sock, NetClient::RequestData && requestData);

	std::map<QStringRefWr_const, std::function<void(ISocket *sock, NetClient::RequestData &&requestData)>> requestWorkersMap {
		{ std::cref(NetConstants::request_try_create_group()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_try_create_group_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_create_note_on_server()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_create_note_on_server_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_move_note_to_group()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_move_note_to_group_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_remove_note()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_remove_note_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_note_saved()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_note_saved_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_synch_note()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_synch_note_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_polly()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_polly_worker(sock, std::move(requestData)); } },

	};
	virtual std::map<QStringRefWr_const, std::function<void(ISocket *sock, RequestData &&requestData)>> & RequestWorkersMap() override { return requestWorkersMap; }

	void Command_to_client_remove_note(ISocket *sock, const QString &idOnClient);
	void Command_to_client_update_note(ISocket *sock, const Note &note);
};
#endif // WIDGET_H
