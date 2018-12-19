#ifndef LMNGR_HPP_
#define LMNGR_HPP_

// #include "Signal.hpp"
#include <libs/SimpleSignal.hpp>
#include <libs/log.hpp>

#include <vector>
#include <cstdarg>

class LMngr;

static size_t const MAX_BUF_SIZE = 0x1000;

// enum class value_t : uint8_t

class Export
{
public:

	Export()
	{
	}

	Export(Export const &aOther)
	{
	}

	Export(Export &&aOther)
	{
	}

	virtual int onHandle(std::string const &)=0;
	virtual int onInit(void *)=0;
	virtual void onDeinit(void *)=0;
};

class LMngr
{
public:

	LMngr() {}

	virtual ~LMngr() 
	{
		for(auto lp : _exportContainer)
		{
			delete lp;
		}
	}

	virtual void init(void *aPtr=nullptr)
	{
		// _onFormat.connect_member(&_format, &Format::onHandle);
		// _onExport.connect_member(&_export, &Export::onHandle);
		_onInit.emit(aPtr);
	}


	virtual void deInit(void *aPtr=nullptr)
	{
		// _onFormat.connect_member(&_format, &Format::onHandle);
		// _onExport.connect_member(&_export, &Export::onHandle);
		_onDeinit.emit(aPtr);
	}

	virtual void disconnect()
	{
		// _onFormat.disconnect();
		// _onExport.disconnect();
	}

	void add(Export *aExport)
	{
		_exportContainer.push_back(aExport);
		Export *lIns = _exportContainer.back();
		size_t lId = _onExport.connect(Simple::slot(lIns, &Export::onHandle));
		lId = _onInit.connect(Simple::slot(lIns, &Export::onInit));
		lId = _onDeinit.connect(Simple::slot(lIns, &Export::onDeinit));
	}

	void log(int aLvl, const char *fmt, ...);

private:
	// Simple::Signal<std::string (char const *, ...)> _onFormat;
	Simple::Signal<int (std::string const &)> _onExport;
	Simple::Signal<int (void *)> _onInit;
	Simple::Signal<void (void *)> _onDeinit;

	std::vector<Export *> _exportContainer;
};

void LMngr::log(int aLvl, const char *fmt, ...)
{
	std::string lBuff;
	char str[MAX_BUF_SIZE];
	va_list args;
	va_start (args, fmt);

	if (vsnprintf (str, sizeof str, fmt, args) >= 0) lBuff = str;
	va_end (args);

	_onExport.emit(lBuff);
}

#endif // LMNGR_HPP_