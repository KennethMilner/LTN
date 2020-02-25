#include "file_reader_manager.h"
#include "service_message.h"
#include "trans_service_message.h"
#include <memory>
#include "server.h"


STATIC_INIT_LUA_MANAGER_INJECTION(file_reader_manager, FILE_READ_MANAGER);

file_block_datas_type file_reader_manager::get_file_blocks(const std::string & file_path, uint64_t file_block_num, uint64_t file_block_count)
{
    auto it = m_file_readers.find(file_path);
    if (m_file_readers.end() == it)
    {
        LOG_ERROR << "file reader manager get file block datas NOT exists: " << OS_TEXT(file_path);
        return nullptr;
    }

    auto reader = it->second;
    assert(nullptr != reader);

    //LOG_DEBUG << "file reader manager begins to get file block datas: " << file_path;

    auto file_blks = std::make_shared<std::list<file_block_data_type>>();
    uint64_t file_blocks_total_count = reader->get_file_blocks_total_count();
    for (uint64_t i = 0; i < file_block_count && file_block_num <= file_blocks_total_count; i++)
    {
        auto file_block_data = reader->get(file_block_num);
        if (nullptr != file_block_data)
        {
            file_blks->push_back(file_block_data);
        }
        else
        {
            reader->notify_missing(file_block_num);
        }

        file_block_num++;
    }

    //LOG_DEBUG << "file reader manager ends to get file block datas: " << file_path;

    return file_blks;
}

void file_reader_manager::notify_send_per_second(const std::string & file_path, uint64_t send_speed_per_second)
{
    auto it = m_file_readers.find(file_path);
    if (m_file_readers.end() == it)
    {
        LOG_ERROR << "file reader manager get file block datas NOT exists: " << OS_TEXT(file_path);
        return;
    }

    it->second->notify_send_per_second(send_speed_per_second);
}

bool file_reader_manager::are_u_ready(const std::string & file_path) const
{
    auto it = m_file_readers.find(file_path);
    if (m_file_readers.end() == it)
    {
        LOG_ERROR << "file reader manager get file ready NOT exists: " << OS_TEXT(file_path);
        return 0;
    }

    return it->second->are_u_ready();
}

uint64_t file_reader_manager::get_file_size(const std::string & file_path) const
{
    auto it = m_file_readers.find(file_path);
    if (m_file_readers.end() == it)
    {
        LOG_ERROR << "file reader manager get file size NOT exists: " << OS_TEXT(file_path);
        return 0;
    }

    return it->second->get_file_size();
}

unsigned int file_reader_manager::get_file_hash(const std::string & file_path)
{
    auto it = m_file_readers.find(file_path);
    if (m_file_readers.end() == it)
    {
        LOG_ERROR << "file reader manager get file hash NOT exists: " << OS_TEXT(file_path);
        return 0;
    }

    return it->second->get_file_hash();
}

file_block_data_type file_reader_manager::get_file_blocks(const std::string & file_path, uint64_t file_block_num)
{
    auto it = m_file_readers.find(file_path);
    if (m_file_readers.end() == it)
    {
        LOG_ERROR << "file reader manager get file block data NOT exists: " << OS_TEXT(file_path);
        return nullptr;
    }

    return it->second->get(file_block_num);
}

uint64_t file_reader_manager::get_file_blocks_total_count(const std::string & file_path) const
{
    auto it = m_file_readers.find(file_path);
    if (m_file_readers.end() == it)
    {
        LOG_ERROR << "file reader manager get file blocks count NOT exists: " << OS_TEXT(file_path);
        return ERR_FAILED;
    }

    return it->second->get_file_blocks_total_count();
}

int32_t file_reader_manager::begin_read_file(const std::string & file_path)
{
    auto it = m_file_readers.find(file_path);
    if (m_file_readers.end() != it)
    {
        LOG_ERROR << "file reader manager begin read file exists: " << OS_TEXT(file_path);
        return ERR_FAILED;
    }

    auto reader = std::make_shared<file_reader>(file_path);

    if (ERR_SUCCESS != reader->open())
    {
        LOG_ERROR << "file reader manager begin read file error: " << OS_TEXT(file_path);
        return ERR_FAILED;
    }
    else
    {
        m_file_readers[file_path] = reader;
        return ERR_SUCCESS;
    }
}

int32_t file_reader_manager::end_read_file(const std::string & file_path)
{
    auto it = m_file_readers.find(file_path);
    if (m_file_readers.end() == it)
    {
        LOG_DEBUG << "file reader manager end read file NOT exists: " << OS_TEXT(file_path);
        return ERR_FAILED;
    }

    if (ERR_SUCCESS != it->second->close())
    {
        LOG_ERROR << "file reader manager close file error:" << OS_TEXT(file_path);
    }

    m_file_readers.erase(it);

    return ERR_SUCCESS;
}

void file_reader_manager::init_invoker()
{
    INIT_INVOKER(END_FILE_READ_NOTIFICATION, &file_reader_manager::on_msg_end_file_read_notification_req);
}

void file_reader_manager::init_timer()
{
    INIT_TIMER(m_scheduling_timer, "scheduling_timer", 100, MAX_TRIGGER_TIMES, "", &file_reader_manager::on_timer_scheduling_file_read);
}

int32_t file_reader_manager::service_init(any_map &vars)
{
    return ERR_SUCCESS;
}

int32_t file_reader_manager::service_exit()
{
    remove_timer(m_scheduling_timer);

    return ERR_SUCCESS;
}

int32_t file_reader_manager::on_msg_begin_file_read_notification_req(msg_ptr_type msg)
{
    return ERR_SUCCESS;
}

int32_t file_reader_manager::on_msg_end_file_read_notification_req(msg_ptr_type msg)
{
    assert(nullptr != msg && nullptr != msg->m_body);
    auto svc_msg = std::dynamic_pointer_cast<end_file_read_notification>(msg->m_body);

    assert(nullptr != svc_msg);

    //end read file
    end_read_file(svc_msg->m_file_path);
    LOG_DEBUG << "file reader manager end file read notification: " << OS_TEXT(svc_msg->m_file_path);

    return ERR_SUCCESS;
}

int32_t file_reader_manager::on_timer_scheduling_file_read(timer_ptr_type timer)
{
    for (auto it = m_file_readers.begin(); it != m_file_readers.end(); it++)
    {
        auto reader = it->second;
        uint64_t blks_total_count = reader->get_file_blocks_total_count();
        uint64_t max_blk_loaded_num = reader->get_max_block_num_loaded();

        //there has blocks unload
        if (max_blk_loaded_num < blks_total_count)
        {            
            reader->load_blocks_on_timer();
            //LOG_DEBUG << "file reader manager load blocks: " << reader->get_file_path() << " max file block num loaded: " << std::to_string(reader->get_max_block_num_loaded());
        }
    }

    return ERR_SUCCESS;
}

