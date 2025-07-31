#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QDateTime>
#include <QLineEdit>

#include "NetConstants.h"
#include "NetClient.h"

#include "Server.h"

struct SessionData
{
	qint64 id;
	QString dt;
	std::set<HttpClient*> activeSockets;

	static SessionData* MakeNewSession()
	{
		auto newSessionPtr = sessionsDatas.emplace(new SessionData(sessionIdCounter++)).first->get();
		mapIdAndSession[newSessionPtr->id] = newSessionPtr;
		return newSessionPtr;
	}
	inline static std::map<qint64, SessionData*> mapIdAndSession;

	bool CmpWithTarget(const NetClient::TargetContent &targetContent)
	{
		return id == targetContent.sessionId.toLongLong() && dt == targetContent.sessionDt;
	}

private:

	SessionData(qint64 id_): id{id_} { dt = QDateTime::currentDateTime().toString(NetConstants::auth_date_time_format()); }

	inline static qint64 sessionIdCounter = 1;
	inline static std::set<std::unique_ptr<SessionData>> sessionsDatas;
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
	bool AuthCheck(HttpClient *sock, const NetClient::TargetContent &targetContent);
	bool SessionWork(HttpClient *sock, const NetClient::TargetContent &targetContent);
private:

	virtual void Write(ISocket *sock, const QString &str) override;

private:
	QTextEdit *textEdit;
	QLineEdit *leAnswDelay;

	QDateTime lastUpdate;

	std::set<HttpClient*> socketsWithotSession;

	// nullptr for creation new session
	void BindSocketToSession(HttpClient* sock, SessionData *session, bool sendYourSessionId);
	std::map<HttpClient*, SessionData*> mapClientSession;

	void MsgsWorker(ISocket *sock, QString text);

	void msg_error_worker(ISocket *sock, QString && msgContent);
	void msg_all_client_notes_synch_sended_worker(ISocket *sock, QString && msgContent);
	//void msg_highest_idOnServer_worker(ISocket *sock, QString && msgContent);

	std::map<QStringRefWr_const, std::function<void(ISocket *sock, QString && msgContent)>> msgWorkersMap {
		{ std::cref(NetConstants::msg_error()), [this](ISocket *sock, QString && msgContent){ msg_error_worker(sock, std::move(msgContent)); } },
		{ std::cref(NetConstants::msg_all_client_notes_synch_sended()), [this](ISocket *sock, QString && msgContent){ msg_all_client_notes_synch_sended_worker(sock, std::move(msgContent)); } },
		//{ std::cref(NetConstants::msg_highest_idOnServer()), [this](ISocket *sock, QString && msgContent){ msg_highest_idOnServer_worker(sock, std::move(msgContent)); } },
	};

	void RequestGetNote(ISocket * sock, QString idOnServer);

	void request_get_session_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_try_create_group_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_create_note_on_server_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_move_note_to_group_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_remove_note_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_note_saved_worker(ISocket *sock, NetClient::RequestData && requestData);
	//void request_synch_note_worker(ISocket *sock, NetClient::RequestData && requestData);
	//void request_get_new_notes_worker(ISocket *sock, NetClient::RequestData && requestData) {}
	void request_get_note_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_group_check_notes_worker(ISocket *sock, NetClient::RequestData && requestData);
	void request_all_notes_worker(ISocket *sock, NetClient::RequestData && requestData);

	void request_polly_worker(ISocket *sock, NetClient::RequestData && requestData);

	std::map<QStringRefWr_const, std::function<void(ISocket *sock, NetClient::RequestData &&requestData)>> requestWorkersMap {
		{ std::cref(NetConstants::request_get_session()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_get_session_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_try_create_group()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_try_create_group_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_create_note_on_server()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_create_note_on_server_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_move_note_to_group()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_move_note_to_group_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_remove_note()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_remove_note_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_note_saved()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_note_saved_worker(sock, std::move(requestData)); } },
		//{ std::cref(NetConstants::request_synch_note()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_synch_note_worker(sock, std::move(requestData)); } },
		//{ std::cref(NetConstants::request_get_new_notes()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_get_new_notes_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_get_note()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_get_note_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_group_check_notes()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_group_check_notes_worker(sock, std::move(requestData)); } },
		{ std::cref(NetConstants::request_all_notes()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_all_notes_worker(sock, std::move(requestData)); } },

		{ std::cref(NetConstants::request_polly()), [this](ISocket *sock, NetClient::RequestData &&requestData){ request_polly_worker(sock, std::move(requestData)); } },

	};
	virtual std::map<QStringRefWr_const, std::function<void(ISocket *sock, RequestData &&requestData)>> & RequestWorkersMap() override { return requestWorkersMap; }

	void Command_to_client_your_session_id(HttpClient *sock);
	void Command_to_client_remove_note(ISocket *sock, const QString &idOnClient);
	void Command_to_client_update_note(ISocket *sock, const Note &note);
};
#endif // WIDGET_H
