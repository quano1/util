#ifndef LMNGR_HPP_
#define LMNGR_HPP_

#include "Signal.hpp"

#include <vector>
#include <cstdarg>

class LMngr;


class Format
{
public:
	virtual void onHandle(std::vector<uint8_t> &aBuff, const char *aFmt, va_list aVars)
	{

	}
};

class Export
{
public:
	virtual void onHandle(std::vector<uint8_t> const &aBuff)
	{
		// return std::vector<uint8_t>();
	}
};

class LMngr
{
public:

	LMngr()
	{
		// _onFormat.connect_member(&_format, &Format::onHandle);
		// _onExport.connect_member(&_export, &Export::onHandle);
	}

	virtual void init()
	{
		// _onFormat.connect_member(&_format, &Format::onHandle);
		// _onExport.connect_member(&_export, &Export::onHandle);
	}

	virtual void disconnect()
	{
		// _onFormat.disconnect();
		// _onExport.disconnect();
	}

	void connectFormat(Format &aFormat)
	{
		_onFormat.connect_member(&aFormat, &Format::onHandle);
	}

	void connectExport(Export &aExport)
	{
		_onExport.connect_member(&aExport, &Export::onHandle);
	}

	void doFormat(std::vector<uint8_t> &aBuff, const char *aFmt, va_list aVars)
	{
		_onFormat.emit(aBuff, aFmt, aVars);
	}

	void doExport(std::vector<uint8_t> const &aBuff)
	{
		_onExport.emit(aBuff);
	}

	void log(const char *fmt, ...)
	{
		std::vector<uint8_t> lBuff;
		// va_list ap;
		// va_start(ap, fmt);
		// doFormat(lBuff, fmt, ap);
		// va_end(ap);
		doExport(lBuff);
	}

	Signal<std::vector<uint8_t> &, const char *, va_list> _onFormat;
	Signal<std::vector<uint8_t> const &> _onExport;

	// Format _format;
	// Export _export;
};

#endif // LMNGR_HPP_