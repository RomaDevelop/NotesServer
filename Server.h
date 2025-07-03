#ifndef SERVER_H
#define SERVER_H

#include <thread>
#include <set>
#include <atomic>

#include <QDebug>
#include <QString>

#include <boost/beast.hpp>
#include <boost/asio.hpp>

#include "MyQShortings.h"

#include <QObject>

#include "NetClient.h"

using tcp = boost::asio::ip::tcp;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

struct HttpCommon
{
	template<class T>
	static QString GetFullText(T &container)
	{
		std::stringstream streamForOutput;
		streamForOutput << container;
		return "["+QString::fromStdString(streamForOutput.str())+"]";
	}
};

class HttpServer;

class HttpClient : public QObject, public ISocket
{
	Q_OBJECT
public:
	explicit HttpClient(tcp::socket *socket, QObject *parent = nullptr) : QObject(parent), socket{socket} { }
	~HttpClient();

	std::function<void(const QString &str)> logFoo;
	std::function<void(const QString &str)> errorFoo;
	void Log(const QString &str) { if(logFoo) logFoo(str); else qdbg << str; }
	void Error(const QString &str) { if(errorFoo) errorFoo(str); /*else*/ qdbg << str; }

	tcp::socket *socket = nullptr;
	http::request<http::string_body> request;
	QString ReadTarget() { return QString::fromStdString(request.target().to_string()); }
	QString ReadBody() { return QString::fromStdString(request.body()); }

	uint8_t authFailCount;

private:
	volatile bool hasPreparedDataToWrite = false;
	QString bodyToWrite;

	bool canSendNow = false;
signals: void SignalReadyRead();


public:
	void EmitSignalReadyRead()
	{
		canSendNow = true;
		emit SignalReadyRead();
	}

	bool ReadyWrite() { return hasPreparedDataToWrite; }

	inline static Requester::RequestData pollyRequestData;
	inline static HttpClient *pollyWriter {};
	inline static QDateTime pollyRequestGetAt;
	inline static QTimer *pollyCloserTimer {};
	inline static void InitPollyCloser(HttpServer *server);
	inline static const int pollyCloserTimeoutMs = 100;
	inline static std::queue<QString> waitsPollyToWrite;

	void Write(QString &&text, bool forcePolly = false);
	void Write_in_HttpServerHandler();

signals:
	void SignalDisconnected();

};

struct HttpClientGuard
{
	HttpClient* client {};
	~HttpClientGuard()
	{
		if(client)
		{
			QMetaObject::invokeMethod(client, [this](){
				emit client->SignalDisconnected();
				client->deleteLater();
			}, Qt::BlockingQueuedConnection);
		}
		else qdbg << "~HttpClientGuard called, but client invalid";
	}
};

class HttpServer : public QObject
{
	Q_OBJECT
public:
	explicit HttpServer(QObject *parent = nullptr);
	~HttpServer() { stop(); }

	std::function<void(const QString &str)> logFoo;
	std::function<void(const QString &str)> errorFoo;
	void Log(const QString &str) { if(logFoo) logFoo(str); else qdbg << str; }
	void Error(const QString &str) { if(errorFoo) errorFoo(str); /*else*/ qdbg << str; }

	void start(unsigned short port);
	void stop();
	bool isRunning() const;

private:
	void run(unsigned short port);
	void handle_session(boost::asio::ip::tcp::socket socket);

	std::thread serverThread;
	std::atomic<bool> running{false};
	std::atomic<bool> stopFlag{false};
	std::set<boost::asio::ip::tcp::socket *> activeSockets;
	std::mutex mtxActiveSockets;
	std::unique_ptr<net::io_context> ioc;
	std::unique_ptr<tcp::acceptor> acceptor;

signals:
	void SignalNewConnection(HttpClient *socket);
};

#endif // SERVER_H
