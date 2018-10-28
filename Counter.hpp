#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>


#define MAX_LOG_LENGTH 1024

namespace llt::utils::log
{
    template <typename T>
    class Reporter
    {
    public:
        Reporter();
        Reporter(T &&aT);
        virtual ~Reporter();

        void write(char const *aBuf, size_t aSize);
        void write(std::vector<char> const &aBuf);
        Reporter<T> &operator << (std::vector<char> const &aBuf);
        Reporter<T> &operator << (std::string const &aBuf);

        void writef(char const *format, va_list args)
        {
            int bufferSize = MAX_LOG_LENGTH;
            char* buf = nullptr;

            do
            {
                buf = new (std::nothrow) char[bufferSize];

                if (buf == nullptr)
                {
                    return;    // not enough memory
                }

                int ret = vsnprintf(buf, bufferSize - 3, format, args);

                if (ret < 0)
                {
                    bufferSize *= 2;

                    delete [] buf;
                }
                else
                {
                    break;
                }

            }
            while (true);

            strcat(buf, "\n");

        #if defined(WIN32)

            int pos = 0;
            int len = strlen(buf);
            char tempBuf[MAX_LOG_LENGTH + 1] = { 0 };
            WCHAR wszBuf[MAX_LOG_LENGTH + 1] = { 0 };

            do
            {
                std::copy(buf + pos, buf + pos + MAX_LOG_LENGTH, tempBuf);

                tempBuf[MAX_LOG_LENGTH] = 0;

                MultiByteToWideChar(CP_UTF8, 0, tempBuf, -1, wszBuf, sizeof(wszBuf));
                OutputDebugStringW(wszBuf);
                WideCharToMultiByte(CP_ACP, 0, wszBuf, -1, tempBuf, sizeof(tempBuf), nullptr, FALSE);
                printf("%s", tempBuf);

                pos += MAX_LOG_LENGTH;

            }
            while (pos < len);

            // SendLogToWindow(buf);
            fflush(_outObj);
        #else
            // Linux, Mac, iOS, etc
            fprintf(_outObj, "%s", buf);
            fflush(_outObj);
        #endif

            delete [] buf;
        }

    // protected:
        T &_outObj;
    };

    template <typename T>
    Reporter<T>::Reporter()
    {

    }

    template <typename T>
    Reporter<T>::Reporter(T &&aT) : _outObj(aT)
    {
        // _outObj = aT;
    }

    template <typename T>
    Reporter<T>::~Reporter()
    {

    }

    template <typename T>
    void Reporter<T>::write(char const *aBuf, size_t aSize)
    {
        _outObj.write(aBuf, aSize);
    }

    template <typename T>
    void Reporter<T>::write(std::vector<char> const &aBuf)
    {
        _outObj.write(aBuf.data(), aBuf.size());
    }

    template <typename T>
    Reporter<T> &Reporter<T>::operator << (std::vector<char> const &aBuf)
    {
        // this->write(aBuf.data(), aBuf.size());
        _outObj << aBuf;
        return *this;
    }

    template <typename T>
    Reporter<T> &Reporter<T>::operator << (std::string const &aBuf)
    {
        // this->write(aBuf.data(), aBuf.size());
        _outObj << aBuf;
        return *this;
    }
}



// namespace llt::utils::perf
// {
//     class Timer
//     {
//     public:
//         Timer() 
//         {
//             using namespace std::chrono;
//             _tbeg = std::chrono::high_resolution_clock::now();
//         }

//         virtual ~Timer() 
//         {
//             using namespace std::chrono;
//             // _indents[std::this_thread::get_id()]-=_INDENT_NUM;
//             high_resolution_clock::time_point lNow = high_resolution_clock::now();
//             // PRINTF("%lld [Prf %lx-%d] %*s%s: %.2f ms\n", custom::Log::timestamp(), (size_t)std::hash<std::thread::id>()(std::this_thread::get_id()), (int)getpid(), _indents[std::this_thread::get_id()], "", _name.data(), duration <double, std::milli> (lNow - _tbeg).count());
//         }

//     protected:
//         std::string _name;
//         std::chrono::high_resolution_clock::time_point _tbeg;
//     };
// }

// Log::Log()
// {
//     setLog("default");
// }

// Log::~Log()
// {
// }

// Log &Log::instance()
// {
//     static Log _sLog;
//     return _sLog;
// }

// void Log::setLog(std::string const &aFile) const
// {
//     using namespace std::chrono;
//     freopen( std::string("/tmp/out." + Log::date() + "." + aFile + ".log.txt").data(), "a", stdout );
//     freopen( std::string("/tmp/err." + Log::date() + "." + aFile + ".log.txt").data(), "a", stderr );
//     std::cout << Log::now() << " - " << duration_cast<microseconds>(system_clock::now().time_since_epoch()).count() << " - " << Log::timestamp() << " Start logging process: " << std::to_string((int)getpid()) << std::endl;
//     std::cerr << Log::now() << " - " << duration_cast<microseconds>(system_clock::now().time_since_epoch()).count() << " - " << Log::timestamp() << " Start logging process: " << std::to_string((int)getpid()) << std::endl;
// }

// void Log::_log(const char *format, va_list args) const
// {
//     int bufferSize = MAX_LOG_LENGTH;
//     char* buf = nullptr;

//     do
//     {
//         buf = new (std::nothrow) char[bufferSize];

//         if (buf == nullptr)
//         {
//             return;    // not enough memory
//         }

//         int ret = vsnprintf(buf, bufferSize - 3, format, args);

//         if (ret < 0)
//         {
//             bufferSize *= 2;

//             delete [] buf;
//         }
//         else
//         {
//             break;
//         }

//     }
//     while (true);

//     strcat(buf, "\n");

// #if defined(WIN32)

//     int pos = 0;
//     int len = strlen(buf);
//     char tempBuf[MAX_LOG_LENGTH + 1] = { 0 };
//     WCHAR wszBuf[MAX_LOG_LENGTH + 1] = { 0 };

//     do
//     {
//         std::copy(buf + pos, buf + pos + MAX_LOG_LENGTH, tempBuf);

//         tempBuf[MAX_LOG_LENGTH] = 0;

//         MultiByteToWideChar(CP_UTF8, 0, tempBuf, -1, wszBuf, sizeof(wszBuf));
//         OutputDebugStringW(wszBuf);
//         WideCharToMultiByte(CP_ACP, 0, wszBuf, -1, tempBuf, sizeof(tempBuf), nullptr, FALSE);
//         printf("%s", tempBuf);

//         pos += MAX_LOG_LENGTH;

//     }
//     while (pos < len);

//     // SendLogToWindow(buf);
//     fflush(stdout);
// #else
//     // Linux, Mac, iOS, etc
//     fprintf(stdout, "%s", buf);
//     fflush(stdout);
// #endif

//     delete [] buf;
// }

// // int _Counter::_indent = 0;
// static std::unordered_map<std::thread::id, int> _indents;

// _Counter::_Counter() : _name("")
// {
//     using namespace std::chrono;
//     _tbeg = std::chrono::high_resolution_clock::now();

//     PRINTF("%lld [Prf %lx-%d] %*s%s\n", custom::Log::timestamp(), (size_t)std::hash<std::thread::id>()(std::this_thread::get_id()), (int)getpid(), _indents[std::this_thread::get_id()], "", _name.data());
//     _indents[std::this_thread::get_id()]+=_INDENT_NUM;
// }

// _Counter::_Counter(std::string const &a_str) : _name(a_str)
// {
//     using namespace std::chrono;
//     _tbeg = std::chrono::high_resolution_clock::now();
//     PRINTF("%lld [Prf %lx-%d] %*s%s\n", custom::Log::timestamp(), (size_t)std::hash<std::thread::id>()(std::this_thread::get_id()), (int)getpid(), _indents[std::this_thread::get_id()], "", _name.data());
//     _indents[std::this_thread::get_id()]+=_INDENT_NUM;
// }

// _Counter::~_Counter()
// {
//     using namespace std::chrono;
//     _indents[std::this_thread::get_id()]-=_INDENT_NUM;
//     high_resolution_clock::time_point lNow = high_resolution_clock::now();
//     PRINTF("%lld [Prf %lx-%d] %*s%s: %.2f ms\n", custom::Log::timestamp(), (size_t)std::hash<std::thread::id>()(std::this_thread::get_id()), (int)getpid(), _indents[std::this_thread::get_id()], "", _name.data(), duration <double, std::milli> (lNow - _tbeg).count());
// }

// #endif // LOG_EN
