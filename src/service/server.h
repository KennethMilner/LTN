#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>
#include <module/module.hpp>
#include "macro.h"
__BEGIN_DECLS__
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
__END_DECLS__

using namespace micro::core;
namespace bpo = boost::program_options;


#define INIT_LUA_LIB \
    m_lua_state = luaL_newstate(); \
    luaL_openlibs(m_lua_state);

#define EXIT_LUA_LIB \
    lua_close(m_lua_state);


#define THE_SERVER boost::serialization::singleton<server>::get_mutable_instance()


class server
{
public:

    server() : m_lua_state(nullptr)  {}

    virtual int32_t init(int argc, char* argv[]);

    virtual int32_t exit();

    std::shared_ptr<module>& operator[](std::string && key);

    std::shared_ptr<module>& operator[](const std::string& key);

protected:

    virtual int32_t init_locale();

    int32_t init_lua_script(any_map &vars);

    int32_t exit_lua_script();

    int32_t parse_cmd(int argc, const char* const argv[], any_map &vars);

protected:

   lua_State* m_lua_state;

    std::allocator<char> m_std_allocator;

    std::unordered_map<std::string, std::shared_ptr<module>> m_mdls;

    std::string m_svc_type;
};
