#include "trans_macro.h"
#include "trans_task.pb.h"

bool file_block_data_comp(const file_block_data_type &a, const file_block_data_type &b)
{
    return a->m_file_block_num < b->m_file_block_num;
}

int32_t trans_cli_usr_upload_task::serialize(std::shared_ptr<leveldb::DB> db)
{
    assert(nullptr != db);
    
    try
    {
        auto db_task = std::make_shared<db_trans_cli_usr_upload_task>();

        //progress
        db_task->mutable_cli_usr_task()->mutable_progress()->set_trans_session_id(m_progress.m_trans_session_id);
        db_task->mutable_cli_usr_task()->mutable_progress()->set_progress(m_progress.m_progress);
        db_task->mutable_cli_usr_task()->mutable_progress()->set_trans_size(m_progress.m_trans_size);
        db_task->mutable_cli_usr_task()->mutable_progress()->set_total_size(m_progress.m_total_size);
        db_task->mutable_cli_usr_task()->mutable_progress()->set_remote_path(m_progress.m_remote_path);

        db_task->mutable_cli_usr_task()->set_trans_id(m_trans_id);
        db_task->mutable_cli_usr_task()->set_direction(m_direction);
        db_task->mutable_cli_usr_task()->set_file_or_dir(m_file_or_dir);

        //db_net_entity
        for (auto task_ne : m_addrs)
        {
            auto db_ne = db_task->mutable_cli_usr_task()->add_addr();

            db_ne->set_host(task_ne->m_host);
            db_ne->set_port(task_ne->m_port);
            db_ne->set_protocol(task_ne->m_protocol);
        }

        db_task->mutable_cli_usr_task()->set_local_path(m_local_path);

        db_task->set_remote_path(m_remote_path);
        db_task->set_root_path(m_root_path);

        for (auto upload_file : m_upload_files)
        {
            db_task->add_upload_file(upload_file);
        }

        db_task->set_uploaded_received_file_size(m_uploaded_received_file_size);
        db_task->set_uploading_received_file_szie(m_uploading_received_file_szie);

        //serialization
        std::string str;
        db_task->SerializeToString(&str);

        //flush to db
        leveldb::WriteOptions write_options;
        write_options.sync = true;

        leveldb::Slice slice(str.c_str(), str.length());
        leveldb::Status s = db->Put(write_options, PREFIX_CLI_USR_UPLOAD + m_trans_id, slice);
        if (!s.ok())
        {
            LOG_ERROR << "write trans upload db ERROR, usr trans id: " << m_trans_id;
            return ERR_FAILED;
        }

        LOG_DEBUG << "write trans upload db, usr trans id: " << m_trans_id;
    }
    catch (const std::exception & e)
    {
        LOG_ERROR << "serialize trans usr upload task error: " << e.what();
        return ERR_FAILED;
    }
    catch (const boost::exception & e)
    {
        LOG_ERROR << "serialize trans usr upload task error: " << diagnostic_information(e);
        return ERR_FAILED;
    }

    return ERR_SUCCESS;
}

int32_t trans_cli_usr_upload_task::deserialize(std::shared_ptr<leveldb::DB> db, const std::string & usr_trans_id)
{
    assert(nullptr != db);

    try
    {
        std::string task_value;
        leveldb::Status status = db->Get(leveldb::ReadOptions(), PREFIX_CLI_USR_UPLOAD + usr_trans_id, &task_value);
        if (!status.ok() || status.IsNotFound())
        {
            LOG_ERROR << "read trans upload db ERROR, usr trans id NOT FOUND: " << usr_trans_id;
            return ERR_FAILED;
        }

        //deserialization
        auto db_task = std::make_shared<db_trans_cli_usr_upload_task>();

        if (!db_task->ParseFromString(task_value))
        {
            LOG_DEBUG << "deserialize trans usr upload task error: " << usr_trans_id;
            return ERR_FAILED;
        }

        m_trans_id = db_task->cli_usr_task().trans_id();
        m_direction = db_task->cli_usr_task().direction();
        m_file_or_dir = db_task->cli_usr_task().file_or_dir();

        for (auto it = db_task->cli_usr_task().addr().cbegin(); it != db_task->cli_usr_task().addr().cend(); it++)
        {
            auto task_ne = std::make_shared<net_entity>();

            task_ne->m_host = it->host();
            task_ne->m_port = it->port();
            task_ne->m_protocol = it->protocol();

            m_addrs.push_back(task_ne);
        }

        m_progress.m_progress = db_task->cli_usr_task().progress().progress();
        m_progress.m_remote_path = db_task->cli_usr_task().progress().remote_path();
        m_progress.m_total_size = db_task->cli_usr_task().progress().total_size();
        m_progress.m_trans_session_id = db_task->cli_usr_task().progress().trans_session_id();
        m_progress.m_trans_size = db_task->cli_usr_task().progress().trans_size();

        m_local_path = db_task->cli_usr_task().local_path();

        m_remote_path = db_task->remote_path();
        m_root_path = db_task->root_path();

        for (auto it = db_task->upload_file().cbegin(); it != db_task->upload_file().cend(); it++)
        {
            m_upload_files.push_back(*it);
        }

        m_uploaded_received_file_size = db_task->uploaded_received_file_size();
        m_uploading_received_file_szie = db_task->uploading_received_file_szie();

        return ERR_SUCCESS;
    }
    catch (...)
    {
        LOG_ERROR << "read trans upload db EXCEPTION";
        return ERR_FAILED;
    }
}

int32_t trans_cli_usr_download_task::serialize(std::shared_ptr<leveldb::DB> db)
{
    assert(nullptr != db);

    try
    {
        auto db_task = std::make_shared<db_trans_cli_usr_download_task>();

        //progress
        db_task->mutable_cli_usr_task()->mutable_progress()->set_trans_session_id(m_progress.m_trans_session_id);
        db_task->mutable_cli_usr_task()->mutable_progress()->set_progress(m_progress.m_progress);
        db_task->mutable_cli_usr_task()->mutable_progress()->set_trans_size(m_progress.m_trans_size);
        db_task->mutable_cli_usr_task()->mutable_progress()->set_total_size(m_progress.m_total_size);
        db_task->mutable_cli_usr_task()->mutable_progress()->set_remote_path(m_progress.m_remote_path);

        db_task->mutable_cli_usr_task()->set_trans_id(m_trans_id);
        db_task->mutable_cli_usr_task()->set_direction(m_direction);
        db_task->mutable_cli_usr_task()->set_file_or_dir(m_file_or_dir);

        //db_net_entity
        for (auto task_ne : m_addrs)
        {
            auto db_ne = db_task->mutable_cli_usr_task()->add_addr();

            db_ne->set_host(task_ne->m_host);
            db_ne->set_port(task_ne->m_port);
            db_ne->set_protocol(task_ne->m_protocol);
        }

        db_task->mutable_cli_usr_task()->set_local_path(m_local_path);

        for (auto remote_path : m_remote_paths)
        {
            db_task->add_remote_path(remote_path);
        }

        db_task->set_root_path(m_root_path);

        db_task->set_downloaded_received_file_size(m_downloaded_received_file_size);
        db_task->set_downloading_received_file_szie(m_downloading_received_file_szie);

        //serialization
        std::string str;
        db_task->SerializeToString(&str);

        //flush to db
        leveldb::WriteOptions write_options;
        write_options.sync = true;

        leveldb::Slice slice(str.c_str(), str.length());
        leveldb::Status s = db->Put(write_options, PREFIX_CLI_USR_DOWNLOAD + m_trans_id, slice);
        if (!s.ok())
        {
            LOG_ERROR << "write trans download db ERROR, usr trans id: " << m_trans_id;
            return ERR_FAILED;
        }

        LOG_DEBUG << "write trans download db, usr trans id: " << m_trans_id;
    }
    catch (const std::exception & e)
    {
        LOG_ERROR << "serialize trans usr download task error: " << e.what();
        return ERR_FAILED;
    }
    catch (const boost::exception & e)
    {
        LOG_ERROR << "serialize trans usr download task error: " << diagnostic_information(e);
        return ERR_FAILED;
    }

    return ERR_SUCCESS;
}

int32_t trans_cli_usr_download_task::deserialize(std::shared_ptr<leveldb::DB> db, const std::string & usr_trans_id)
{
    assert(nullptr != db);

    try
    {
        std::string task_value;
        leveldb::Status status = db->Get(leveldb::ReadOptions(), PREFIX_CLI_USR_DOWNLOAD + usr_trans_id, &task_value);
        if (!status.ok() || status.IsNotFound())
        {
            LOG_ERROR << "read trans download db ERROR, usr trans id NOT FOUND: " << usr_trans_id;
            return ERR_FAILED;
        }

        //deserialization
        auto db_task = std::make_shared<db_trans_cli_usr_download_task>();

        if (!db_task->ParseFromString(task_value))
        {
            LOG_DEBUG << "deserialize trans usr download task error: " << usr_trans_id;
            return ERR_FAILED;
        }

        m_trans_id = db_task->cli_usr_task().trans_id();
        m_direction = db_task->cli_usr_task().direction();
        m_file_or_dir = db_task->cli_usr_task().file_or_dir();

        for (auto it = db_task->cli_usr_task().addr().cbegin(); it != db_task->cli_usr_task().addr().cend(); it++)
        {
            auto task_ne = std::make_shared<net_entity>();

            task_ne->m_host = it->host();
            task_ne->m_port = it->port();
            task_ne->m_protocol = it->protocol();

            m_addrs.push_back(task_ne);
        }

        m_progress.m_progress = db_task->cli_usr_task().progress().progress();
        m_progress.m_remote_path = db_task->cli_usr_task().progress().remote_path();
        m_progress.m_total_size = db_task->cli_usr_task().progress().total_size();
        m_progress.m_trans_session_id = db_task->cli_usr_task().progress().trans_session_id();
        m_progress.m_trans_size = db_task->cli_usr_task().progress().trans_size();

        m_local_path = db_task->cli_usr_task().local_path();

        for (auto it = db_task->remote_path().cbegin(); it != db_task->remote_path().cend(); it++)
        {
            m_remote_paths.push_back(*it);
        }

        m_root_path = db_task->root_path();

        m_downloaded_received_file_size = db_task->downloaded_received_file_size();
        m_downloading_received_file_szie = db_task->downloading_received_file_szie();

        return ERR_SUCCESS;
    }
    catch (...)
    {
        LOG_ERROR << "read trans download db EXCEPTION";
        return ERR_FAILED;
    }
}
