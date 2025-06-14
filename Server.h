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

class HttpClient : public QObject, public ISocket
{
	Q_OBJECT
public:
	explicit HttpClient(tcp::socket *socket, QObject *parent = nullptr) : QObject(parent), socket{socket} { }
	~HttpClient() { }

	std::function<void(const QString &str)> logFoo;
	void Log(const QString &str) { if(logFoo) logFoo(str); else qdbg << str; }
	void Error(const QString &str) { if(logFoo) logFoo(str); /*else*/ qdbg << str; }

	tcp::socket *socket = nullptr;
	http::request<http::string_body> request;
	QString ReadTarget() { return QString::fromStdString(request.target().to_string()); }
	QString ReadBody() { return QString::fromStdString(request.body()); }

	bool readyWrite = false;
	QString bodyToWrite;

signals:
	void SignalDisconnected();
	void SignalReadyRead();
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
	explicit HttpServer(QObject *parent = nullptr) : QObject(parent) { }
	~HttpServer() { stop(); }

	std::function<void(const QString &str)> logFoo;
	void Log(const QString &str) { if(logFoo) logFoo(str); else qdbg << str; }
	void Error(const QString &str) { if(logFoo) logFoo(str); /*else*/ qdbg << str; }

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
