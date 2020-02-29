#include "../libs/exporter.h"
#include "../libs/exporterudp.h"

int main(int argc, char const *argv[])
{
	using namespace llt;
	EConsole console;
	console.onInit();
	console.onExport(LogInfo{});
	return 0;
}