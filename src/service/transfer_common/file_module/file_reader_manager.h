#pragma once

#include <module/module.hpp>
#include "util.h"
#include "macro.h"
__BEGIN_DECLS__
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
__END_DECLS__
#include <common/any_map.hpp>
#include <memory>
#include "file_reader.h"


using namespace micro::core;

#define FILE_READ_MANAGER boost::serialization::singleton<file_reader_manager>::get_mutable_instance()


__BEGIN_DECLS__
extern int32_t luaopen_file_reader_manager(lua_State *L);
__END_DECLS__

class file_reader_manager : public module
{
public:

    file_reader_manager() : m_scheduling_timer(INVALID_TIMER_ID) {}
    
    virtual ~file_reader_manager() = default;

    std::string name() const { return "file reader manager"; }

public:

    bool are_u_ready(const std::string & file_path) const;

    uint64_t get_file_size(const std::string & file_path) const;

    unsigned int get_file_hash(const std::string & file_path);

    uint64_t get_file_blocks_total_count(const std::string & file_path) const;

    file_block_data_type get_file_blocks(const std::string & file_path, uint64_t file_block_num);

    file_block_datas_type get_file_blocks(const std::string & file_path, uint64_t file_block_num, uint64_t file_block_count);

    void notify_send_per_second(const std::string & file_path, uint64_t send_speed_per_second);

    int32_t begin_read_file(const std::string & file_path);

    int32_t end_read_file(const std::string & file_path);

protected:

    void init_invoker();

    void init_timer();

    int32_t service_init(any_map &vars);

    int32_t service_exit();

protected:

    int32_t on_timer_scheduling_file_read(timer_ptr_type timer);

    int32_t on_msg_begin_file_read_notification_req(msg_ptr_type msg);

    int32_t on_msg_end_file_read_notification_req(msg_ptr_type msg);

protected:

    uint64_t m_scheduling_timer;

    std::map<std::string, std::shared_ptr<file_reader>> m_file_readers;

};
