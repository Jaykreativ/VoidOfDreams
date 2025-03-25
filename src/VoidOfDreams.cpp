#include "Log.h"
#include "Layers/Game.h"

#include "Zap/Zap.h"

void main() {
	logger::initLog();
	logger::beginRegion("main");

	auto base = Zap::Base::createBase("Void of Dreams", "VOD.zal"); // give the engine access to the asset library
	base->init(); // initializing zap engine, this loads the asset library and other things
	
	runGame();

	base->terminate();
	Zap::Base::releaseBase();

	logger::endRegion();
	logger::terminateLog();

	system("pause");
}