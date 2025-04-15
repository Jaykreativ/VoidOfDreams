#include "Log.h"
#include "Layers/Game.h"

#include "Zap/Zap.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>

void startWSA() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed\n");
		exit(1);
	}

	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2)
	{
		fprintf(stderr, "Version 2.2 of Winsock not available\n");
		WSACleanup();
		exit(1);
	}
}

void closeWSA() {
	WSACleanup();
}
#endif // _WIN32

void main() {
	logger::initLog();
	logger::beginRegion("main");

#ifdef _WIN32
	startWSA();
#endif // _WIN32

	auto base = Zap::Base::createBase("Void of Dreams", "VOD.zal"); // give the engine access to the asset library
	base->init(); // initializing zap engine, this loads the asset library and other things
	
	runGame();

	base->terminate();
	Zap::Base::releaseBase();

#ifdef _WIN32
	closeWSA();
#endif // _WIN32

	logger::endRegion();
	logger::terminateLog();

	system("pause");
}