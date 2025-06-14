#include "Server.h"

#include <iostream>

#include <MyCppDifferent.h>



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
	Log("HttpServer::handle_session");

	mtxActiveSockets.lock();
	activeSockets.insert(&socket);
	mtxActiveSockets.unlock();

	try {
		HttpClientGuard clientGuard;
		QMetaObject::invokeMethod(this, [this, &clientGuard, &socket](){
			clientGuard.client = new HttpClient(&socket);
			emit SignalNewConnection(clientGuard.client);
		}, Qt::BlockingQueuedConnection);

		HttpClient *client = clientGuard.client;
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

			clientGuard.client->Write_for_HttpServer();

			if (!request.keep_alive()) break;  // клиент просит закрыть
		}

		socket.shutdown(tcp::socket::shutdown_send);
	}
	catch (std::exception& e) {
		Error(QString("HttpServer::handle_session catch exception: ") + QString::fromLocal8Bit(e.what()));
	}

	mtxActiveSockets.lock();
	activeSockets.erase(&socket);
	mtxActiveSockets.unlock();

	Log("HttpServer::handle_session end");
}
