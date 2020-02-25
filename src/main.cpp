#include <common/common.hpp>
#include "service/server.h"
#include <thread>
#include "service/init_server.h"

using namespace std::chrono_literals;

bool g_exited = false;

int main(int argc, char* argv[])
{
    if (ERR_SUCCESS != init_server(argc, argv))
    {
        return ERR_FAILED;
    }

    while (!g_exited)
    {
        std::this_thread::sleep_for(1s);
    }    

    return exit_server();
}
