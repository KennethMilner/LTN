#include "init_server.h"
#include "server.h"

int init_server(int argc, char* argv[])
{
    int32_t ret = THE_SERVER.init(argc, argv);
    if (ERR_SUCCESS != ret)
    {
        LOG_ERROR << "server init error and ready to exit";
        return ERR_FAILED;
    }
    return ERR_SUCCESS;
}

int exit_server()
{
    return THE_SERVER.exit();
}
