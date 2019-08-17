#include <LogMngr.hpp>
#include <ExporterUDP.hpp>

#include <string>
#include <chrono>
#include <fstream>

#include <cstring>
#include <Base.hpp>
// void do_smt(int aLoop, int aThreadNums, int aDelay);

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

llt::LogMngr *gpLog;


void create_thread(int aLoop, int aThreadNums, int aDelay)
{
    std::thread ts[aThreadNums];
    {
        TRACE(gpLog, THREADING);
        for (auto &t : ts)
            t = std::thread([aLoop,aDelay]()
        {
            // gpLog->reg_ctx("sub do_smt");
            TRACE(gpLog, Thread);
            Base1 b1;
            Base2 b2;
            for(int i=0; i<aLoop; i++)
            {
                LOGI(gpLog, "");
                b1.do_smt();
                b2.do_smt();
                std::this_thread::sleep_for(std::chrono::microseconds(aDelay));
            }
        });
    }
        
    for(auto &t : ts) t.join();
}


int main(int argc, char **argv)
{
    llt::Util::SEPARATOR = ";";

    std::string host = getCmdOption(argc, argv, "-h=");
    std::string port = getCmdOption(argc, argv, "-p=");
    std::string sport = getCmdOption(argc, argv, "-s=");
    std::string delay = getCmdOption(argc, argv, "-d=");
    std::string threads = getCmdOption(argc, argv, "-t=");
    std::string loop = getCmdOption(argc, argv, "-l=");

    // std::ofstream ofs("prof.log", std::ios::out);

    // LOGD("%s", host.data());
    // LOGD("%s", port.data());
    // LOGD("%s", threads.data());
    // LOGD("%s", loop.data());

    int lLoop = std::stoi(loop);
    // int lPort = std::stoi(port);
    // int lSPort = std::stoi(sport);
    int lThreads = std::stoi(threads);
    int lDelay = std::stoi(delay);

    std::srand(std::time(nullptr));
    llt::LogMngr logger({ 
            // new llt::EConsole(),
            new llt::EFile(std::string(argv[0])+".log"), 
            // new llt::EUDPClt(host, lPort),
            // new llt::EUDPSvr(lSPort),
            // new llt::ENSClt("/tmp/llt.dgram"),
        }, 1);

    logger.set_async(1);
    logger.reg_app(argv[0]);
    // logger.reg_ctx("main");
    gpLog = &logger;

    TRACE(gpLog, MAIN);

    {
        TRACE(gpLog, main_self_call);
    }

    {
        ::create_thread(lLoop, lThreads, lDelay);
    }

    return 0;
}