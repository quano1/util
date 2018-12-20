#include <libs/LMngr.hpp>
#include <libs/log.hpp>

#include <chrono>
#include <fstream>

int main()
{
    std::ofstream ofs("prof.log", std::ios::out);

    {
        LogSync log;
        log.add(new ENetUDP("localhost", 65530));

        log.init(nullptr);

        std::chrono::high_resolution_clock::time_point _tbeg = std::chrono::high_resolution_clock::now();
        for(int i=0; i<10000; i++)
        {
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
        }
        std::chrono::high_resolution_clock::time_point lNow = std::chrono::high_resolution_clock::now();
        double diff = std::chrono::duration <double, std::milli> (lNow - _tbeg).count();
        ofs << "Sync ENetUDP: " << diff << std::endl;

        LOGD("");
        log.deInit();
    }

    {
        LogAsync log;
        log.add(new ENetUDP("localhost", 65530));

        log.init(nullptr);

        std::chrono::high_resolution_clock::time_point _tbeg = std::chrono::high_resolution_clock::now();
        for(int i=0; i<10000; i++)
        {
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
        }
        std::chrono::high_resolution_clock::time_point lNow = std::chrono::high_resolution_clock::now();
        double diff = std::chrono::duration <double, std::milli> (lNow - _tbeg).count();
        ofs << "ENetUDP: " << diff << std::endl;

        LOGD("");
        log.deInit();
    }

    {
        LogSync log;
        log.add(new EFile("log.txt"));

        log.init(nullptr);

        std::chrono::high_resolution_clock::time_point _tbeg = std::chrono::high_resolution_clock::now();
        for(int i=0; i<10000; i++)
        {
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
        }
        std::chrono::high_resolution_clock::time_point lNow = std::chrono::high_resolution_clock::now();
        double diff = std::chrono::duration <double, std::milli> (lNow - _tbeg).count();
        ofs << "Sync EFile: " << diff << std::endl;

        LOGD("");
        log.deInit();
    }

    {
        LogAsync log;
        log.add(new EFile("log.txt"));

        log.init(nullptr);

        std::chrono::high_resolution_clock::time_point _tbeg = std::chrono::high_resolution_clock::now();
        for(int i=0; i<10000; i++)
        {
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
        }
        std::chrono::high_resolution_clock::time_point lNow = std::chrono::high_resolution_clock::now();
        double diff = std::chrono::duration <double, std::milli> (lNow - _tbeg).count();
        ofs << "EFile: " << diff << std::endl;

        LOGD("");
        log.deInit();
    }

    {
        LogSync log;
        log.add(new EConsole());

        log.init(nullptr);

        std::chrono::high_resolution_clock::time_point _tbeg = std::chrono::high_resolution_clock::now();
        for(int i=0; i<10000; i++)
        {
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
        }
        std::chrono::high_resolution_clock::time_point lNow = std::chrono::high_resolution_clock::now();
        double diff = std::chrono::duration <double, std::milli> (lNow - _tbeg).count();
        ofs << "Sync EConsole: " << diff << std::endl;

        LOGD("");
        log.deInit();
    }

    {
        LogAsync log;
        log.add(new EConsole());

        log.init(nullptr);

        std::chrono::high_resolution_clock::time_point _tbeg = std::chrono::high_resolution_clock::now();
        for(int i=0; i<10000; i++)
        {
            log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
        }
        std::chrono::high_resolution_clock::time_point lNow = std::chrono::high_resolution_clock::now();
        double diff = std::chrono::duration <double, std::milli> (lNow - _tbeg).count();
        ofs << "EConsole: " << diff << std::endl;

        LOGD("");
        log.deInit();
    }

    LOGD("");

    return 0;
}