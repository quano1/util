#include <LogMngr.hpp>

#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <unistd.h>

using namespace llt;

LogMngr::LogMngr(std::vector<Export *> const &aExpList, size_t aWorkers)
{
    _appName = std::to_string((size_t)::getpid());
    for(auto _export : aExpList)
    {
        add(_export);
    }

    init(aWorkers);
}

LogMngr::~LogMngr() 
{
    deinit();
    for(auto lp : _exportContainer)
    {
        delete lp;
    }
}

void LogMngr::init(size_t aWorkers)
{    
    if(aWorkers)
    {
        _pool.add_workers(aWorkers);
    }
    
    _sigInit.emit();
}

void LogMngr::deinit()
{
    _pool.stop(_forceStop);
    _sigDeinit.emit();
}

void LogMngr::add(Export *aExport)
{
    _exportContainer.push_back(aExport);
    Export *lIns = _exportContainer.back();
    size_t lId = _sigExport.connect(Simple::slot(lIns, &Export::on_export));
    lId = _sigInit.connect(Simple::slot(lIns, &Export::on_init));
    lId = _sigDeinit.connect(Simple::slot(lIns, &Export::on_deinit));
}

void LogMngr::log(LogType aLogType, const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    std::string lBuff = Util::format(fmt, args);
    va_end (args);

    std::thread::id lKey = std::this_thread::get_id();
    if(_ctx[lKey].empty()) _ctx[lKey] = _appName + Util::SEPARATOR + Util::to_string(lKey);
    LogInfo lInfo = {aLogType, _indents[lKey], std::chrono::system_clock::now(), _ctx[lKey], std::move(lBuff) };
    _sigExport.emit(lInfo);
}


void LogMngr::log_async(LogType aLogType, const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    std::string lBuff = Util::format(fmt, args);
    va_end (args);
    std::thread::id lKey = std::this_thread::get_id();

    if(_ctx[lKey].empty()) _ctx[lKey] = _appName + Util::SEPARATOR + Util::to_string(lKey);
    LogInfo lInfo = {aLogType, _indents[lKey], std::chrono::system_clock::now(), _ctx[lKey], std::move(lBuff) };
    _sigExport.emit_async(_pool, lInfo);
}

void ThreadPool::stop(bool aForce)
{
    if(_stop) return;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        _stop = true;
        if(aForce) tasks = std::queue< std::function<void()> >();
    }
    condition.notify_all();

    for(std::thread &worker: workers)
    {
        worker.join();
    }
}

void ThreadPool::add_workers(size_t threads)
{
    for(size_t i = 0;i<threads;++i)
    {
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->_stop || !this->tasks.empty(); });
                        if(this->_stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            }
        );
    }
}

// the constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads)
    :   _stop(false)
{
    add_workers(threads);
}


// the destructor joins all threads
ThreadPool::~ThreadPool()
{
    stop(true);
}

extern "C" void log_init(LogMngr **aLogger)
{
    *aLogger = new LogMngr({
            new EConsole(),
            new EFile("run.log"),
            // new EUDPClt(host, lPort),
            // new EUDPSvr(lSPort),
        });
}

extern "C" void log_deinit(LogMngr *aLogger)
{
    if(aLogger) delete aLogger;
}

extern "C" void log_async(LogMngr *aLogger, int aLogType, const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    std::string lBuff = Util::format(fmt, args);
    va_end (args);
    aLogger->log_async(static_cast<LogType>(aLogType), lBuff.data());
}

extern "C" void start_trace(LogMngr *aLogger, Tracer **aTracer, char const *aBuf)
{
    *aTracer = new Tracer(aLogger, aBuf);
}

extern "C" void stop_trace(Tracer *aTracer)
{
    delete aTracer;
}
