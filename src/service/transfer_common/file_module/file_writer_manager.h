#pragma once

#include <module/module.hpp>
#include "util.h"
#include "macro.h"
#include "file_writer.h"
__BEGIN_DECLS__
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
__END_DECLS__
#include <common/any_map.hpp>
#include <memory>


using namespace micro::core;

__BEGIN_DECLS__
extern int32_t luaopen_file_writer_manager(lua_State *L);
__END_DECLS__

#define FILE_WRITER_MANAGER boost::serialization::singleton<file_writer_manager>::get_mutable_instance()


class file_writer_manager : public module
{
public:

    file_writer_manager() : m_scheduling_timer(INVALID_TIMER_ID) {}

    virtual ~file_writer_manager() = default;

    std::string name() const { return "file writer manager"; }
    
protected:

    void init_invoker();

    void init_timer();

    int32_t service_init(any_map &vars);

    int32_t service_exit();

protected:

    int32_t on_timer_scheduling(timer_ptr_type timer);

    int32_t on_msg_begin_file_write_notification_req(msg_ptr_type msg);

    int32_t on_msg_file_block_write_notification_req(msg_ptr_type msg);

    int32_t on_msg_end_file_write_notification_req(msg_ptr_type msg);

protected:

    uint64_t m_scheduling_timer;

    std::map<std::string, std::shared_ptr<file_writer>> m_file_writers;

};