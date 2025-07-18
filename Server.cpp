#include "Server.h"

#include <iostream>

#include <QCoreApplication>

#include "MyCppDifferent.h"
#include "MyQString.h"

HttpServer::HttpServer(QObject *parent) : QObject(parent)
{
	HttpClient::InitPollyCloser(this);
}

void HttpServer::start(unsigned short port) {
	if (running) return;
	Log("HttpServer::start");
	running = true;
	serverThread = std::thread([this, port] { run(port); });
}

void HttpServer::stop() {
	Log("HttpServer::stop");

	running = false;

	mtxActiveSockets.lock();
	for(auto &sock:activeSockets) sock->close();
	activeSockets.clear();
	activeSocketsCount = 0;
	mtxActiveSockets.unlock();

	if(acceptor) acceptor->close();

	if (serverThread.joinable()) {
		serverThread.join();
	}
	Log("HttpServer::stop end");
}

bool HttpServer::isRunning() const {
	return running;
}

void HttpServer::run(unsigned short port) {
	Log("HttpServer::run");
	try {
		ioc = std::make_unique<net::io_context>();
		acceptor = std::make_unique<tcp::acceptor>(*ioc, tcp::endpoint(tcp::v4(), port));
		while (running) {

			tcp::socket socket(*ioc);
			acceptor->accept(socket);
			std::thread(&HttpServer::handle_session, this, std::move(socket)).detach();
		}
	} catch (std::exception& e) {
		Error(QString("HttpServer::run catch exception: ") + QString::fromLocal8Bit(e.what()));
	}

	Log("HttpServer::run end");
}

void HttpServer::handle_session(tcp::socket socket) {
	mtxActiveSockets.lock();
	activeSockets.insert(&socket);
	activeSocketsCount++;
	mtxActiveSockets.unlock();

	try {
		HttpClientGuard clientGuard;
		if(0) CodeMarkers::to_do("нужно как-то сделать так, чтобы HttpClient не умирал, а еще какое-то время жил"
								 "а то можно где-то как-то всраться с использованием метвого объекта"
								 "но тогда нужно в нем сделать соответсвующую пометку, что он своё отработал, а его дергают");
		QMetaObject::invokeMethod(this, [this, &clientGuard, &socket](){
			clientGuard.client = new HttpClient(&socket);
			emit SignalNewConnection(clientGuard.client);
		}, Qt::BlockingQueuedConnection);

		HttpClient *client = clientGuard.client;
		Log("HttpServer::handle_session HttpClient created: " + MyQString::AsDebug(client) + "; activeSocketsCount: " + QSn(activeSocketsCount));

		while (true){
			beast::flat_buffer buffer;
			http::request<http::string_body> request;
			http::read(socket, buffer, request);

			clientGuard.client->request = request;
			clientGuard.client->EmitSignalReadyRead();

			while (!client->ReadyWrite() && !stopFlag) {
				MyCppDifferent::sleep_ms(10);
			}

			if(stopFlag) break;

			clientGuard.client->Write_in_HttpServerHandler();

			if (!request.keep_alive()) break;  // клиент просит закрыть
		}

		socket.shutdown(tcp::socket::shutdown_send);
	}
	catch (std::exception& e) {
		Error(QString("HttpServer::handle_session catch exception: ") + QString::fromLocal8Bit(e.what()));
	}

	mtxActiveSockets.lock();
	activeSockets.erase(&socket);
	activeSocketsCount--;
	mtxActiveSockets.unlock();

	Log("HttpServer::handle_session end");
}

HttpClient::~HttpClient()
{
	if(pollyWriter == this)
	{
		ClearPolly();
	}
}

void HttpClient::InitPollyCloser(HttpServer *server)
{
	HttpClient::pollyCloserTimer = new QTimer(server);
	connect(HttpClient::pollyCloserTimer, &QTimer::timeout, [server](){
		if(!HttpClient::pollyRequestData.id.isEmpty())
		{
			if(HttpClient::pollyRequestGetAt.addMSecs(NetConstants::pollyMaxWaitServerMs) <= QDateTime::currentDateTime())
			{
//				server->Log(">>> write polly answ: pollyRequestGetAt: " + HttpClient::pollyRequestGetAt.toString(DateTimeFormat_ms)
//							+ "\n\t" + HttpClient::pollyRequestGetAt.addMSecs(HttpClient::pollyMaxWaitMs).toString(DateTimeFormat_ms) + " <= "
//							+ QDateTime::currentDateTime().toString(DateTimeFormat_ms));

				if(HttpClient::pollyWriter)
				{
					QString answ = NetConstants::nothing();
					Requester::PrepareStringToSend(answ, true);
					HttpClient::pollyWriter->Write(std::move(answ), true);
				}
				else
				{
					server->Error("PollyCloserWorker: HttpClient::pollyWriter invalid (pollyRequestData.id = "
									+HttpClient::pollyRequestData.id+")");
				}
			}
		}
	});
	HttpClient::pollyCloserTimer->start(HttpClient::pollyCloserTimeoutMs);
}

void HttpClient::Write(QString &&text, bool forcePolly)
{
	// если сейчас идет отправка, ждем 3 секунды чтобы завершилась
	if(hasPreparedDataToWrite) {
		for(int i=0; i<300 && hasPreparedDataToWrite; i++)
		{
			if(i%10 == 0) QCoreApplication::processEvents();
			MyCppDifferent::sleep_ms(10);
		}
		if(hasPreparedDataToWrite) { Error("HttpClient::Write: called when hasPreparedDataToWrite"); return; }
		// если не завершилась - выдаем ошибку
	}

	if(canSendNow && !forcePolly && this != pollyWriter)
	{
		Log("HttpClient::Write: regular answering now: " + text);
		bodyToWrite = std::move(text);
		hasPreparedDataToWrite = true;
	}
	else
	{
		if(!pollyRequestData.id.isEmpty())
		{
			if(!pollyWriter) { Error("HttpClient::Write: try write polly to invalid pollyWriter: " + text); return; }
			Log("HttpClient::Write: sending by polly now: " + text);

			pollyWriter->bodyToWrite.clear();
			pollyWriter->bodyToWrite.append(NetConstants::request_answ()).append(pollyRequestData.id).append(" ").append(text);
			pollyWriter->hasPreparedDataToWrite = true;

			ClearPolly();
		}
		else
		{
			if(waitsPollyToWrite.size() > 100)
			{
				Error("HttpClient::Write:  waitsPollyToWrite is full, remove 10");
				for (int i = 0; i < 10; ++i) waitsPollyToWrite.pop();
			}

			Log("HttpClient::Write: message will wait polly: "+text);
			waitsPollyToWrite.emplace(std::move(text));
		}
	}
}

void HttpClient::Write_in_HttpServerHandler()
{
	http::response<http::string_body> res{http::status::ok, request.version()};
	res.set(http::field::server, "Boost.Beast");
	res.set(http::field::content_type, "text/plain");
	res.set(http::field::connection, "keep-alive");
	res.body() = bodyToWrite.toStdString();

	bodyToWrite.clear();

	res.prepare_payload();

	http::write(*socket, res);
	hasPreparedDataToWrite = false;
	canSendNow = false;

	if(pollyWriter && pollyWriter->socket == socket) ClearPolly();
}
