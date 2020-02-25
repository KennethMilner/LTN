#pragma once

#include <module/module.hpp>
#include <io/tcp_connector.hpp>
#include <thread/nio_thread_pool.hpp>
#include "macro.h"
__BEGIN_DECLS__
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
__END_DECLS__


using namespace micro::core;

#define POOL_THREADS_COUNT    4
#define IO_MANAGER boost::serialization::singleton<io_manager>::get_mutable_instance()

#define RECONNECT_TIMER                        "reconnect_timer"

__BEGIN_DECLS__
extern int32_t luaopen_io_manager(lua_State *L);
__END_DECLS__


class io_manager : public module
{
public:

    io_manager() : m_reconnect_timer(INVALID_TIMER_ID) {}

    std::string name() const { return "io manager"; }

    virtual int32_t send_msg(msg_ptr_type msg);

    virtual int32_t close(channel_source sid);

    virtual int32_t reconnect(channel_source sid);

protected:

    void init_invoker();

    virtual void init_timer();

    virtual int32_t service_init(any_map &vars);

    virtual int32_t service_exit();

protected:

    int32_t on_msg_login_rsp(msg_ptr_type msg);

    int32_t on_msg_dev_reg_rsp(msg_ptr_type msg);

    int32_t on_msg_channel_notification(msg_ptr_type msg);

    int32_t on_msg_client_connect_notification(msg_ptr_type msg);

    int32_t on_msg_heartbeat_rsp(msg_ptr_type msg);

    int32_t on_timer_reconnect(timer_ptr_type timer);

    int32_t on_timer_heartbeat(timer_ptr_type timer);

protected:

    uint64_t m_heartbeat_timer;

    uint64_t m_reconnect_timer;

    std::shared_ptr<nio_thread_pool> m_pool;

    //client connector id as key
    std::unordered_map<uint64_t, std::shared_ptr<tcp_connector>> m_connectors;

    std::unordered_map<uint64_t, std::shared_ptr<tcp_connector>> m_reconnectors;
    
};
