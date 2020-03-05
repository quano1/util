#pragma once

#include <fstream>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <sstream>
#include <string>
#include <unordered_set>

#include <cstdarg>
#include <iomanip>

#include <netinet/in.h>
#include <unistd.h> // close
#include <sys/socket.h>
#include <sys/un.h>

#include "utils.h"

#ifndef STATIC_LIB
#define UTIL_INLINE
#else
#define UTIL_INLINE inline
#endif


namespace llt {

enum class LogType : uint8_t
{
    kTrace=0,
    kDebug,
    kInfo,
    kFatal,
};

struct LogInfo
{
    LogType type;
    // size_t timestamp;
    // std::string context;
    std::string message;

    friend std::string to_string(LogInfo const &log)
    {
        return utils::Format("[%d]%s", log.type, log.message);
    }
};

template <unsigned int delay>
class Logger
{
public:
    Logger() : lf_queue_(0x1000), is_running_(true)
    {
        grabber = std::thread([this]()
        {
            while(is_running_)
            {
                using namespace std;
                uint32_t cons_head;
                while(!lf_queue_.tryPop(cons_head) && is_running_)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(delay));
                    continue;
                }
                if(!is_running_) return;
                std::string log_message = to_string(lf_queue_.buff(cons_head));
                lf_queue_.completePop(cons_head);
                for(auto fd : fds_)
                    write(fd, log_message.data(), log_message.size());
            }
        });
    }
    ~Logger() 
    {
        is_running_ = false;
        grabber.join();
    }

    inline void join()
    {
        while(lf_queue_.available())
            std::this_thread::sleep_for(std::chrono::microseconds(delay));
    }

    template <LogType type, typename... Args>
    void log(const char *format, Args &&... args)
    {
        lf_queue_.push({
            .type = type,
            // .timestamp = utils::timestamp(),
            // .context = ::utils::tid(),
            .message = utils::Format(format, std::forward<Args>(args)...)
        });
    }

    inline void add(int fd)
    {
        join();
        fds_.insert(fd);
    }

    inline void rem(int fd)
    {
        join();
        fds_.erase(fd);
    }

    utils::BSDLFQ<LogInfo> lf_queue_;
    bool is_running_;
    std::thread grabber;
    // std::vector<int> fds_;
    std::unordered_set<int> fds_;
};

} // llt


#ifndef STATIC_LIB
#include "exporter.cc"
#endif