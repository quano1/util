#include <libs/LMngr.hpp>
#include <libs/log.hpp>

#include <iostream>
#include <fstream>

class EConsole : public Export
{
public:
	EConsole operator=(EConsole const &aOther)
	{
		// aOther.ofs = std::move(ofs);
	}

	EConsole operator=(EConsole &&aOther)
	{
		// aOther.ofs = std::move(ofs);
	}

	int onInit() { return 0; }
	int onDeinit() { return 0; }
	int onHandle(std::string const &aBuff) { std::cout<<aBuff; }
};

class EFile : public Export
{
public:
	EFile(std::string const& aFile)
	{
		LOGD("");
		_f = aFile;
	}

	// EFile(EFile const &aOther)
	// {
	// 	ofs = std::move(aOther.ofs);
	// }

	EFile(EFile &&aOther)
	{
		LOGD("");
		ofs = std::move(aOther.ofs);
	}

	// EFile operator=(EFile const &aOther)
	// {
	// 	ofs = std::move(aOther.ofs);
	// }

	EFile &operator=(EFile &&aOther)
	{
		LOGD("");
		ofs = std::move(aOther.ofs);
		if(!ofs.is_open())
			LOGD("");
		return *this;
	}

	int onInit()
	{
		LOGD("");
		ofs.open(_f, std::ios::out | std::ios::app );
		if(!ofs.is_open())
			LOGD("");
		return 0; 
	}

	int onDeinit() 
	{
		LOGD("");
		ofs.close();
		return 0;
	}

	int onHandle(std::string const &aBuff) 
	{ 
		LOGD("");
		ofs << aBuff;
	}

	std::ofstream ofs;
	std::string _f;
};


int main()
{
	LMngr log;
	// EConsole lECons;
	// log.connectExport(std::move(lECons));

	// EFile lEfile("log.txt");
	// log.connectExport(lEfile);
	log.connectExport(EFile("log2.txt"));

	log.init();

	log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

	// std::string hello = string_printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

	// printf("%s", hello.data());

	log.deInit();
		LOGD("");
	return 0;
}