#include <tests/third-party/cxxopts/include/cxxopts.hpp>

int __attribute__((weak)) benchmark()
{
    printf("benchmark is disabled!\n");
    return 0;
}

int __attribute__((weak)) perfTunnel()
{
    printf("perfTunnel is disabled!\n");
    return 0;
}
int __attribute__((weak)) unittests(int argc, char **argv)
{
    printf("unittests is disabled!\n");
    return 0;
}

int main(int argc, char *argv[])
{
    cxxopts::Options options(argv[0]);

    options.add_options()
        ("b,benchmark", "Run benchmark", cxxopts::value<bool>()->default_value("false"))
        ("p,perftunnel", "Run performance tunnel", cxxopts::value<bool>()->default_value("false"))
        ("u,unittests", "Run unit tests", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Print help")
    ;
    options.allow_unrecognised_options();
    auto result = options.parse(argc, argv);

    // std::cout << result.arguments().size() << std::endl;

    if (result.arguments().size() == 0 || result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    // bool debug = result["debug"].as<bool>();
    // std::string bar;
    if (result.count("benchmark"))
    {
        benchmark();
    }

    if (result.count("perftunnel"))
    {
        perfTunnel();
    }

    if (result.count("unittests"))
    {
        unittests(argc, argv);
    }

    // int foo = result["foo"].as<int>();


    return 0;
}

