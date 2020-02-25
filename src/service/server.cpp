#include "server.h"
#include "util.h"
#include <locale>
#include <common/any_map.hpp>
#include <logger/logger.hpp>
#include "timer_generator_init.h"
#include "io_manager.h"
#include "config_manager.h"
#include "task_manager.h"
#include "trans_cli_config_manager.h"
#include "trans_cli_io_manager.h"
#include "file_reader_manager.h"
#include "trans_cli_upload_manager.h"
#include "trans_cli_download_manager.h"
#include "trans_cli_sync_manager.h"
#include "trans_svr_config_manager.h"
#include "trans_svr_io_manager.h"
#include "file_writer_manager.h"
#include "trans_svr_upload_manager.h"
#include "trans_svr_download_manager.h"
#include "trans_svr_sync_manager.h"
#include "health_manager.h"

#pragma warning(disable:4996)

int32_t server::init(int argc, char* argv[])
{
    any_map vars;

    //parse cmd line
    int32_t ret = parse_cmd(argc, argv, vars);
    if (ERR_SUCCESS != ret)
    {
        return ERR_SUCCESS;
    }

    //locale
    init_locale();

    //work dir
    path_util::set_work_dir();

    //LOG_INFO << "============ server begins to run ============";

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    ret = init_lua_script(vars);
    if (ERR_SUCCESS != ret)
    {
        LOG_FATAL << "============ server init FAILED and exit ============";
        return ret;
    }
    
    LOG_INFO << "============ server is ready ============";
    return ERR_SUCCESS;
}

int32_t server::exit()
{
    exit_lua_script();

    google::protobuf::ShutdownProtobufLibrary();

    return ERR_SUCCESS;
}

std::shared_ptr<module>& server::operator[](std::string && key)
{
    return m_mdls[key];
}

std::shared_ptr<module>& server::operator[](const std::string& key)
{
    return m_mdls[key];
}

int32_t server::init_lua_script(any_map &vars)
{
    INIT_LUA_LIB;

    //lua_pushlightuserdata(m_lua_state, &vars);
    //lua_setglobal(m_lua_state, "vars");

    path lua_file;

 if (m_svc_type == "trans_cli")
    {
        //common
        luaL_requiref(m_lua_state, "timer_generator", luaopen_timer_generator, 1);

        //trans cli
        luaL_requiref(m_lua_state, "trans_cli_config_manager", luaopen_trans_cli_config_manager, 1);
        luaL_requiref(m_lua_state, "trans_cli_io_manager", luaopen_trans_cli_io_manager, 1);
        luaL_requiref(m_lua_state, "file_reader_manager", luaopen_file_reader_manager, 1);
        luaL_requiref(m_lua_state, "file_writer_manager", luaopen_file_writer_manager, 1);
        luaL_requiref(m_lua_state, "trans_cli_upload_manager", luaopen_trans_cli_upload_manager, 1);
        luaL_requiref(m_lua_state, "trans_cli_download_manager", luaopen_trans_cli_download_manager, 1);
        luaL_requiref(m_lua_state, "trans_cli_sync_manager", luaopen_trans_cli_sync_manager, 1);

        lua_file = path_util::get_cur_dir() / "script/lua/init_trans_cli.lua";
    }
    else if (m_svc_type == "trans_svr")
    {
        //common
        luaL_requiref(m_lua_state, "timer_generator", luaopen_timer_generator, 1);

        //trans server
        luaL_requiref(m_lua_state, "trans_svr_config_manager", luaopen_trans_svr_config_manager, 1);
        luaL_requiref(m_lua_state, "trans_svr_io_manager", luaopen_trans_svr_io_manager, 1);
        luaL_requiref(m_lua_state, "file_reader_manager", luaopen_file_reader_manager, 1);
        luaL_requiref(m_lua_state, "file_writer_manager", luaopen_file_writer_manager, 1);
        luaL_requiref(m_lua_state, "trans_svr_upload_manager", luaopen_trans_svr_upload_manager, 1);
        luaL_requiref(m_lua_state, "trans_svr_download_manager", luaopen_trans_svr_download_manager, 1);
        luaL_requiref(m_lua_state, "trans_svr_sync_manager", luaopen_trans_svr_sync_manager, 1);

        lua_file = path_util::get_cur_dir() / "script/lua/init_trans_svr.lua";
    }
    else
    {
        //common
        luaL_requiref(m_lua_state, "timer_generator", luaopen_timer_generator, 1);

        //trans cli
        luaL_requiref(m_lua_state, "trans_cli_config_manager", luaopen_trans_cli_config_manager, 1);
        luaL_requiref(m_lua_state, "trans_cli_io_manager", luaopen_trans_cli_io_manager, 1);
        luaL_requiref(m_lua_state, "file_reader_manager", luaopen_file_reader_manager, 1);
        luaL_requiref(m_lua_state, "file_writer_manager", luaopen_file_writer_manager, 1);
        luaL_requiref(m_lua_state, "trans_cli_upload_manager", luaopen_trans_cli_upload_manager, 1);
        luaL_requiref(m_lua_state, "trans_cli_download_manager", luaopen_trans_cli_download_manager, 1);
        luaL_requiref(m_lua_state, "trans_cli_sync_manager", luaopen_trans_cli_sync_manager, 1);

        lua_file = path_util::get_cur_dir() / "script/lua/init_trans_cli.lua";
    }

    if (luaL_dofile(m_lua_state, lua_file.string().c_str()))
    {
        LOG_ERROR << "lua do file error: " << lua_file;
        return ERR_FAILED;
    }

    lua_getglobal(m_lua_state, "init_server");
    lua_pushlightuserdata(m_lua_state, &vars);

    lua_pcall(m_lua_state, 1, 1, 0); 

    lua_getglobal(m_lua_state, "init_ret");
    int32_t rt = lua_tointeger(m_lua_state, -1);

    return rt;
}

int32_t server::exit_lua_script()
{
    if (luaL_dofile(m_lua_state, "init.lua"))
    {
        return ERR_FAILED;
    }

    lua_getglobal(m_lua_state, "exit_server");

    lua_pcall(m_lua_state, 0, 1, 0);

    EXIT_LUA_LIB;

    return ERR_SUCCESS;
}

int32_t server::init_locale()
{
    try
    {
        //set to system env
        std::locale::global(std::locale(""));
    }
    catch (const std::runtime_error &e)
    {
        LOG_ERROR << "server init locale exception: " << e.what();
        std::locale::global(std::locale("C"));
    }

    LOG_DEBUG << "locale set: " << std::locale().name();
    return ERR_SUCCESS;
}

int32_t server::parse_cmd(int argc, const char* const argv[], any_map &vars)
{
    const char * web_start_param = "srendingtransfer:";
    int web_start_param_len = strlen(web_start_param);

    //analyze web start params
    for (int i = 0; i < argc; i++)
    {
        //std::cout << argv[i] << std::endl;
        if (strlen(argv[i]) >= web_start_param_len && (0 == stricmp(argv[i], web_start_param)))
        {
            LOG_DEBUG << "svc type: SRending Transfer";
            m_svc_type = "trans_cli";
            return ERR_SUCCESS;
        }
    }

    //analyze local.conf
    bpo::variables_map vm;
    options_description opts("svc command options");

    opts.add_options()
        ("help,h", "get help info")
        ("version,v", "get version info")
        ("type,t", bpo::value<std::string>(), "service type")
        ("file,f", bpo::value<std::string>(), "upload file")
        ("dir,d", bpo::value<std::string>(), "upload directory")
        ("remote", bpo::value<std::string>(), "remote store path");

    try
    {
        //parse
        bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
        bpo::notify(vm);

        //help
        if (vm.count("help") || vm.count("h"))
        {
            std::cout << opts;
            return ERR_CMD_EXIT;
        }
        //version
        else if (vm.count("version") || vm.count("v"))
        {
            std::cout << SOFTWARE_VERSION << "\n";
            return ERR_CMD_EXIT;
        }
        else if (vm.count("type") || vm.count("t"))
        {
            m_svc_type = vm["type"].as<std::string>();

            if (vm.count("file") || vm.count("f"))
            {
                vars.set("file", vm["file"].as<std::string>());
            }
            else if (vm.count("dir") || vm.count("d"))
            {
                vars.set("dir", vm["dir"].as<std::string>());
            }
            
            if (vm.count("remote"))
            {
                vars.set("remote", vm["remote"].as<std::string>());
            }
        }

        return ERR_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cout << "invalid command option " << e.what() << std::endl;
        std::cout << opts;
        return ERR_FAILED;
    }
    catch (...)
    {
        std::cout << argv[0] << " invalid command option" << std::endl;
        std::cout << opts;
        return ERR_FAILED;
    }
}

