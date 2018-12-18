#ifndef LMNGR_HPP_
#define LMNGR_HPP_

// #include "Signal.hpp"
#include <libs/SimpleSignal.hpp>
#include <libs/log.hpp>

#include <vector>
#include <cstdarg>

class LMngr;

static std::string string_printf (const char *format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));

static size_t const MAX_BUF_SIZE = 0x1000;

static inline std::string string_printf (const char *format, ...)
{
  std::string result;
  char str[MAX_BUF_SIZE];
  va_list args;
  va_start (args, format);
  if (vsnprintf (str, sizeof str, format, args) >= 0) result = str;
  va_end (args);
  return std::move(result);
}

// enum class value_t : uint8_t

// class Format
// {
// public:
// 	virtual void onHandle(std::string &aBuff, const char *aFmt, va_list aVars)
// 	{

// 	}
// };

class Export
{
public:

	Export()
	{
	}

	Export(Export const &aOther)
	{
		// aOther.ofs = std::move(ofs);
	}

	Export(Export &&aOther)
	{
		// aOther.ofs = std::move(ofs);
	}

	virtual Export &operator=(Export &&aOther)
	{
		LOGD("");
		return *this;
	}

	virtual int onHandle(std::string const &aBuff)
	{
		// return std::string();
		return 0;
	}

	virtual int onInit()
	{
		return 0;
	}

	virtual int onDeinit()
	{
		return 0;
	}
};

class LMngr
{
public:

	LMngr() {}

	~LMngr() {}

	virtual void init()
	{
		// _onFormat.connect_member(&_format, &Format::onHandle);
		// _onExport.connect_member(&_export, &Export::onHandle);
		_onInit.emit();
	}


	virtual void deInit()
	{
		// _onFormat.connect_member(&_format, &Format::onHandle);
		// _onExport.connect_member(&_export, &Export::onHandle);
		_onDeinit.emit();
	}

	virtual void disconnect()
	{
		// _onFormat.disconnect();
		// _onExport.disconnect();
	}

	// void connectFormat(Format &aFormat)
	// {
	// 	_onFormat.connect_member(&aFormat, &Format::onHandle);
	// }

	void connectExport(Export &aExport)
	{
		LOGD("");
		size_t lId = _onExport.connect(Simple::slot(aExport, &Export::onHandle));
		lId = _onInit.connect(Simple::slot(aExport, &Export::onInit));
		lId = _onDeinit.connect(Simple::slot(aExport, &Export::onDeinit));
	}

	void connectExport(Export &&aExport)
	{
		LOGD("");
		_v.push_back(aExport);
		Export &lIns = _v[_v.size()-1];
		size_t lId = _onExport.connect(Simple::slot(&lIns, &Export::onHandle));
		lId = _onInit.connect(Simple::slot(&lIns, &Export::onInit));
		lId = _onDeinit.connect(Simple::slot(&lIns, &Export::onDeinit));
	}

	// std::string doFormat(const char *aFmt, va_list aVars)
	// {
	// 	return _onFormat.emit(aBuff, aFmt, aVars);
	// }

	// void doExport(std::string const &aBuff)
	// {
	// 	_onExport.emit(aBuff);
	// }

	void log(int aLvl, const char *fmt, ...);

	// Simple::Signal<std::string (char const *, ...)> _onFormat;
	Simple::Signal<int (std::string const &)> _onExport;
	Simple::Signal<int ()> _onInit;
	Simple::Signal<int ()> _onDeinit;
	// Format _format;
	// Export _export;

	std::vector<Export> _v;
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