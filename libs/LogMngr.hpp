#pragma once

#include <SimpleSignal.hpp>
#include <ThreadPool.hpp>

#include <Exporter.hpp>

#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>

namespace llt {
class LogMngr
{

public:

    LogMngr() = default;
    LogMngr(std::vector<Exporter *> const &, size_t);
    virtual ~LogMngr();

    virtual void log(LogType aLogType, const std::string &aFile, const std::string &aFunction, int aLine, const char *fmt, ...);
    virtual void log(LogType aLogType, const char *fmt, ...);

    inline void inc_indent()
    {
        _indents[std::this_thread::get_id()]++;
    }

    inline void dec_indent()
    {
        _indents[std::this_thread::get_id()]--;
    }

    inline void reg_app(std::string const &aProgName)
    {
        _appName = aProgName;
    }

    inline void reg_ctx(std::string aThreadName)
    {
        _contexts[std::this_thread::get_id()] = _appName + Util::SEPARATOR + aThreadName;
    }

    inline void set_force_stop(bool aForceStop) 
    {
        _forceStop = aForceStop;
    }

    inline void set_async(bool aAsync)
    {
        _isAsync = aAsync;
    }

protected:
    
    virtual void init();
    virtual void deinit();
    virtual void add(Exporter *);

    ThreadPool _pool;
    bool _forceStop=false;

    Simple::Signal<void (LogInfo const &)> _sigExport;
    Simple::Signal<int ()> _sigInit;
    Simple::Signal<void ()> _sigDeinit;

    std::vector<Exporter *> _exporters;

    std::string _appName;
    std::unordered_map<std::thread::id, std::string> _contexts;
    std::unordered_map<std::thread::id, int> _indents;

    bool _isAsync=false;

};

struct Tracer
{
public:
    Tracer(LogMngr *aLogger, std::string const &aName) : _pLogger(aLogger), _name(aName)
    {
        _pLogger->inc_indent();
    }

    ~Tracer()
    {
        _pLogger->dec_indent();
        _pLogger->log(LogType::TRACE, "~%s", _name.data());
    }

    LogMngr *_pLogger;
    std::string _name;
};

} // llt

// #if (defined __LLT_DEBUG__)
// #define LLT_ASSERT(cond, msg) std::assert(cond, msg)
// #else
#define LLT_ASSERT(cond, msg) if(!(cond)) throw std::runtime_error(msg)
// #endif

#define LOGD(format, ...) printf("[Dbg] %s %s %d " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define TRACE(logger, FUNC) (logger)->log(llt::LogType::TRACE, __FILE__, __FUNCTION__, __LINE__, #FUNC); llt::Tracer __##FUNC(logger, #FUNC)

#define LOGI(logger, fmt, ...) (logger)->log(llt::LogType::INFO, __FILE__, __FUNCTION__, __LINE__, ";" fmt "", ##__VA_ARGS__)
#define LOGW(logger, fmt, ...) (logger)->log(llt::LogType::WARN, __FILE__, __FUNCTION__, __LINE__, ";" fmt "", ##__VA_ARGS__)
#define LOGE(logger, fmt, ...) (logger)->log(llt::LogType::ERROR, __FILE__, __FUNCTION__, __LINE__, ";" fmt "", ##__VA_ARGS__)
