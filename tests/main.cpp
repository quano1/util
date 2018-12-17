#include <iostream>
#include <fstream>

#include <Counter.hpp>

int main(int argc, char **argv)
{
    {
        std::ofstream lofs("log_ofstream.txt");
        llt::utils::log::Reporter<std::ofstream> myRpt(std::move(lofs));
        myRpt << "test ofstream" << "\n";

        std::string buffer = "test write\n";
        myRpt.write(buffer.data(), buffer.size());
    }

    {
        llt::utils::log::Reporter<std::ostream> myRpt(std::move(std::cout));
        myRpt << "test cout" << "\n";

        std::string buffer = "test write\n";
        myRpt.write(buffer.data(), buffer.size());
        float test = 10.f;
        myRpt.writef("oi troi oi %.2f\n");
    }

    return 0;
}