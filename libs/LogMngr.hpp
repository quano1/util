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
    LogMngr(std::vector<Export *> const &, size_t=1);
    virtual ~LogMngr();

    virtual void log(LogType aLogType, const char *fmt, ...);
    virtual void log_async(LogType aLogType, const char *fmt, ...);

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
        _ctx[std::this_thread::get_id()] = _appName + Util::SEPARATOR + aThreadName;
    }

    inline void set_force_stop(bool aForceStop) 
    {
        _forceStop = aForceStop;
    }

// protected:
    
    virtual void init(size_t=0);
    virtual void deinit();
    virtual void add(Export *aExport);

    ThreadPool _pool;
    bool _forceStop=false;

    Simple::Signal<void (LogInfo const &)> _sigExport;
    Simple::Signal<int ()> _sigInit;
    Simple::Signal<void ()> _sigDeinit;

    std::vector<Export *> _exportContainer;

    std::string _appName;
    std::unordered_map<std::thread::id, std::string> _ctx;
    std::unordered_map<std::thread::id, int> _indents;

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
        _pLogger->log_async(LogType::TRACE, "~%s", _name.data());
    }

    LogMngr *_pLogger;
    std::string _name;
};

} // llt

#define LOGD(format, ...) printf("[Dbg] %s %s %d " format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define TRACE(logger, FUNC) (logger)->log_async(llt::LogType::TRACE, #FUNC " %s %d", __FUNCTION__, __LINE__); llt::Tracer __##FUNC(logger, #FUNC)

#define LOGI(logger, fmt, ...) (logger)->log_async(llt::LogType::INFO, "%s %s %d " fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(logger, fmt, ...) (logger)->log_async(llt::LogType::WARN, "%s %s %d " fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(logger, fmt, ...) (logger)->log_async(llt::LogType::ERROR, "%s %s %d " fmt "", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)