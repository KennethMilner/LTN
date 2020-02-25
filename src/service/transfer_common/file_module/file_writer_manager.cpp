#include "file_writer_manager.h"
#include "trans_service_message.h"
#include "server.h"

STATIC_INIT_LUA_MANAGER_INJECTION(file_writer_manager, FILE_WRITER_MANAGER);

void file_writer_manager::init_invoker()
{
    INIT_INVOKER(BEGIN_FILE_WRITE_NOTIFICATION, &file_writer_manager::on_msg_begin_file_write_notification_req);
    INIT_INVOKER(END_FILE_WRITE_NOTIFICATION, &file_writer_manager::on_msg_end_file_write_notification_req);
    INIT_INVOKER(FILE_BLOCK_WRITE_NOTIFICATION, &file_writer_manager::on_msg_file_block_write_notification_req);
}

void file_writer_manager::init_timer()
{
    INIT_TIMER(m_scheduling_timer, "file writer manager", 30000, MAX_TRIGGER_TIMES, "", &file_writer_manager::on_timer_scheduling);
}

int32_t file_writer_manager::service_init(any_map &vars)
{
    return ERR_SUCCESS;
}

int32_t file_writer_manager::service_exit()
{
    remove_timer(m_scheduling_timer);

    return ERR_SUCCESS;
}

int32_t file_writer_manager::on_timer_scheduling(timer_ptr_type timer)
{
    //LOG_DEBUG << "file writer manager scheduling write file blocks";

    LOG_DEBUG << "file wirter manager queue size: " << std::to_string(this->m_worker_queue->size());

    for (auto it = m_file_writers.begin(); it != m_file_writers.end(); it++)
    {
        LOG_DEBUG << "file write manager on timer scheduling write file blocks: " << OS_TEXT(it->second->get_file_path());
        it->second->write_blocks();
    }

    return ERR_SUCCESS;
}

int32_t file_writer_manager::on_msg_begin_file_write_notification_req(msg_ptr_type msg)
{
    assert(nullptr != msg && nullptr != msg->m_body);
    auto svc_msg = std::dynamic_pointer_cast<begin_file_write_notification>(msg->m_body);

    //check exists
    const std::string & os_charset_store_file_path = OS_TEXT(svc_msg->m_store_file_path);
    auto it = m_file_writers.find(svc_msg->m_trans_session_id);
    if (m_file_writers.end() != it)
    {
        LOG_ERROR << "file writers begin notification exists: " << svc_msg->m_trans_session_id << " store file path: " << os_charset_store_file_path;
        return ERR_FAILED;
    }

    LOG_DEBUG << "file writer manager on msg begin file writer notification, trans session id: " << svc_msg->m_trans_session_id << " store file path: " << os_charset_store_file_path;

    //create file writer
    std::shared_ptr<file_writer> writer = std::make_shared<file_writer>(svc_msg->m_store_file_path, svc_msg->m_file_size, svc_msg->m_total_file_blocks_count);
    m_file_writers[svc_msg->m_trans_session_id] = writer;

    //create null file
    writer->create_null_file();

    //open file
    if (svc_msg->m_file_size)
    {
        writer->open();
    }

    return ERR_SUCCESS;
}

int32_t file_writer_manager::on_msg_file_block_write_notification_req(msg_ptr_type msg)
{
    assert(nullptr != msg && nullptr != msg->m_body);
    auto svc_msg = std::dynamic_pointer_cast<file_block_write_notification>(msg->m_body);

    //check exists
    auto it = m_file_writers.find(svc_msg->m_trans_session_id);
    if (m_file_writers.end() == it)
    {
        LOG_ERROR << "file writers write notification NOT exists: " << svc_msg->m_trans_session_id;
        return ERR_FAILED;
    }

    if (0 == svc_msg->m_file_block_num % 1000)
    {
        LOG_DEBUG << "file writer manager on msg file block write notification, trans session id: " << svc_msg->m_trans_session_id << " file block num: " << svc_msg->m_file_block_num;
    }

    assert(0 != svc_msg->m_file_block_data->get_checksum());

    //add to block list
    int32_t ret = it->second->add(svc_msg->m_file_block_data);
    if (ERR_SUCCESS != ret)
    {
        //LOG_ERROR << "file writer manager add file block error: " << svc_msg->m_file_block_data->m_file_block_num;
    }

    return ret;
}

int32_t file_writer_manager::on_msg_end_file_write_notification_req(msg_ptr_type msg)
{
    assert(nullptr != msg && nullptr != msg->m_body);
    auto svc_msg = std::dynamic_pointer_cast<end_file_write_notification>(msg->m_body);

    //check exists
    auto it = m_file_writers.find(svc_msg->m_trans_session_id);
    if (m_file_writers.end() == it)
    {
        LOG_ERROR << "file writers end notification NOT exists: " << svc_msg->m_trans_session_id;
        return ERR_FAILED;
    }

    LOG_DEBUG << "file writer manager on msg end file writer notification, trans session id: " << svc_msg->m_trans_session_id;

    //dump block to disk
    it->second->write_blocks();

    //close
    it->second->close();

    //validate checksum
    it->second->set_file_hash(svc_msg->m_file_hash);
    if (0 != svc_msg->m_file_hash)
    {
        assert(ERR_SUCCESS == it->second->validate_checksum());
    }
    else
    {
        LOG_INFO << "file writer manager file hash zero NOT validate checksum: " << it->second->get_os_charset_file_path();
    }
    
    //erase
    m_file_writers.erase(svc_msg->m_trans_session_id);

    LOG_DEBUG << "file writer manager on msg end file writer notification erase trans session id: " << svc_msg->m_trans_session_id;

    return ERR_SUCCESS;
}
