#include <LogMngr.hpp>

#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <unistd.h>

using namespace llt;

template <typename T>
LogMngr::LogMngr(std::vector<Exporter<T> *> const &aExpList, size_t aWorkerNum)
{
    _appName = Util::to_string_hex((size_t)::getpid());
    for(auto _export : aExpList)
    {
        add(_export);
    }

    if(aWorkerNum)
    {
        _isAsync = true;
        _pool.add_worker(aWorkerNum);
    }
    else
    {
        _isAsync = false;
    }

    init();
}

LogMngr::~LogMngr() 
{
    deinit();
    for(auto lp : _exporters)
    {
        delete lp;
    }
}

void LogMngr::init()
{
    _sigInit.emit();
}

void LogMngr::deinit()
{
    _pool.stop(_forceStop);
    _sigDeinit.emit();
}

template <typename T>
void LogMngr::add(Exporter<T> *exporter)
{
    _exporters.push_back(exporter);
    Exporter<T> *lIns = _exporters.back();
    size_t lId = _sigExport.connect(Simple::slot(lIns, &Exporter<T>::onExport));
    lId = _sigInit.connect(Simple::slot(lIns, &Exporter<T>::onInit));
    lId = _sigDeinit.connect(Simple::slot(lIns, &Exporter<T>::onDeinit));
}

void LogMngr::log(LogType aLogType, const void *aThis, const std::string &aFile, const std::string &aFunction, int aLine, const char *fmt, ...)
{
    if(_isAsync) LLT_ASSERT(_pool.worker_size(), "EMPTY WORKER");
    va_list args;
    va_start (args, fmt);
    std::string lBuff = Util::format(fmt, args);
    va_end (args);

    std::thread::id lKey = std::this_thread::get_id();
    if(_contexts[lKey].empty()) _contexts[lKey] = Util::to_string_hex(lKey);
    LogInfo lInfo = {aLogType, _levels[lKey], std::chrono::system_clock::now(), aFile, aFunction, aLine, _contexts[lKey], std::move(lBuff), _appName, aThis};

    if(_isAsync) _sigExport.emit_async(_pool, lInfo);
    else _sigExport.emit(lInfo);
}

void LogMngr::log(LogType aLogType, const void *aThis, const char *fmt, ...)
{
    if(_isAsync) LLT_ASSERT(_pool.worker_size(), "EMPTY WORKER");
    va_list args;
    va_start (args, fmt);
    std::string lBuff = Util::format(fmt, args);
    va_end (args);

    std::thread::id lKey = std::this_thread::get_id();
    if(_contexts[lKey].empty()) _contexts[lKey] = Util::to_string_hex(lKey);
    LogInfo lInfo = {aLogType, _levels[lKey], std::chrono::system_clock::now(), std::string(), std::string(), -1, _contexts[lKey], std::move(lBuff), _appName, aThis};

    if(_isAsync) _sigExport.emit_async(_pool, lInfo);
    else _sigExport.emit(lInfo);
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

void ThreadPool::add_worker(size_t threads)
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
    add_worker(threads);
}


// the destructor joins all threads
ThreadPool::~ThreadPool()
{
    stop(true);
}

extern "C" void llt_log_init(LogMngr **aLogger, size_t aWorkerNum, const char *aFileName)
{
    *aLogger = new LogMngr({
            new EConsole(),
            new EFile(aFileName),
            // new EUDPClt(host, lPort),
            // new EUDPSvr(lSPort),
        }, aWorkerNum);
}

extern "C" void llt_log_deinit(LogMngr *aLogger)
{
    if(aLogger) delete aLogger;
}

extern "C" void llt_log(LogMngr *aLogger, int aLogType, const void *aThis, const char *aFile, const char *aFunction, int aLine, const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    std::string lBuff = Util::format(fmt, args);
    va_end (args);
    aLogger->log(static_cast<LogType>(aLogType), aThis, aFile, aFunction, aLine, lBuff.data());
}

extern "C" void llt_start_trace(LogMngr *aLogger, Tracer **aTracer, char const *aBuf)
{
    *aTracer = new Tracer(aLogger, aBuf);
}

extern "C" void llt_stop_trace(Tracer *aTracer)
{
    delete aTracer;
}

extern "C" void llt_set_async(LogMngr *aLogger, bool aAsync)
{
    aLogger->set_async(aAsync);
}
