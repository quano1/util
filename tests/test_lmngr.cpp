#include <libs/LMngr.hpp>
#include <libs/log.hpp>

#include <string>
#include <chrono>
#include <fstream>

#include <cstring>

std::string getCmdOption(int argc, char* argv[], const std::string& option)
{
    std::string cmd;
     for( int i = 0; i < argc; ++i)
     {
          std::string arg = argv[i];
          if(0 == arg.find(option))
          {
               std::size_t found = arg.find_last_of(option);
               cmd =arg.substr(found + 1);
               return cmd;
          }
     }
     return cmd;
}

int main(int argc, char **argv)
{
    std::string host = getCmdOption(argc, argv, "-h=");
    std::string port = getCmdOption(argc, argv, "-p=");
    std::string threads = getCmdOption(argc, argv, "-t=");
    std::string loop = getCmdOption(argc, argv, "-l=");

    std::ofstream ofs("prof.log", std::ios::out);

    LOGD("%s", host.data());
    LOGD("%s", port.data());
    LOGD("%s", threads.data());
    LOGD("%s", loop.data());

    int lLoop = std::stoi(loop);
    int lPort = std::stoi(port);
    int lThreads = std::stoi(threads);

    {
        LogMngr log;
        log.add(new EConsole());
        log.add(new EFile("log_async.txt"));
        log.add(new ENetUDP(host, lPort));

        log.init(lThreads);

        std::chrono::high_resolution_clock::time_point _tbeg = std::chrono::high_resolution_clock::now();
        for(int i=0; i<lLoop; i++)
        {
            log.log_async(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log_async(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log_async(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log_async(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log_async(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log_async(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log_async(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log_async(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log_async(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
        }
        std::chrono::high_resolution_clock::time_point lNow = std::chrono::high_resolution_clock::now();
        double diff = std::chrono::duration <double, std::milli> (lNow - _tbeg).count();
        ofs << "Async: " << diff << std::endl;

        LOGD("");
        log.async_wait();
        log.deinit();
    }

    {
        LogMngr log;
        log.add(new EConsole());
        log.add(new EFile("log_sync.txt"));
        log.add(new ENetUDP("localhost", 65530));

        log.init();

        std::chrono::high_resolution_clock::time_point _tbeg = std::chrono::high_resolution_clock::now();
        for(int i=0; i<lLoop; i++)
        {
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
        }
        std::chrono::high_resolution_clock::time_point lNow = std::chrono::high_resolution_clock::now();
        double diff = std::chrono::duration <double, std::milli> (lNow - _tbeg).count();
        ofs << "Sync: " << diff << std::endl;

        LOGD("");
        log.deinit();
    }

    LOGD("");

    return 0;
}