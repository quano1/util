#include <libs/LMngr.hpp>
#include <libs/log.hpp>

#include <iostream>
#include <fstream>

class EConsole : public Export
{
public:
    int onInit(void *aPtr) { return 0; }
    void onDeinit(void *aPtr) { }
    int onHandle(std::string const &aBuff) { std::cout<<aBuff; }
};

class EFile : public Export
{
public:
    EFile(std::string const& aFile)
    {
        _f = aFile;
    }

    int onInit(void *aPtr)
    {
        ofs.open(_f, std::ios::out | std::ios::app );
        if(!ofs.is_open()) return 1;
        return 0; 
    }

    void onDeinit(void *aPtr)
    {
        if(ofs.is_open())
        {
            ofs.flush();
            ofs.close();
        }
    }

    int onHandle(std::string const &aBuff) 
    { 
        ofs << aBuff;
    }

private:
    std::ofstream ofs;
    std::string _f;
};


int main()
{
    LMngr log;
    log.add(new EConsole());
    log.add(new EFile("log.txt"));

    log.init(nullptr);

    log.log(0, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    log.deInit();
    LOGD("");
    return 0;
}