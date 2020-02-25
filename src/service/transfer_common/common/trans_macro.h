#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <mutex>
#include <list>
#include <memory>
#include <cassert>
#include <string>
#include <boost/asio.hpp>
#include "util.h"
#include "snappy.h"
#include <leveldb/db.h>

#define DEFAULT_MAX_TRANSFER_RATE                   (100 * 1024 * 1024)                 //100Mbps

#define DEFAULT_SCHEDULING_TIMER_INTERVAL        100                 //ms
#define DEFAULT_HEARTBEAT_COUNTDOWN                  10

#define DEFAULT_FILE_BLOCK_SIZE                                     1200             //bytes
#define MIN_FILE_BLOCK_NUM                                              1

#define DEFAULT_UDP_MSG_TIMER_INTEVAL                   5000

#define DEFAULT_HTTP_STATUS_CODE                                200
#define DEFAULT_HTTP_ERROR_CODE                                 400
#define DEFAULT_HTTP_FS_ERROR_CODE                          1400
#define HTTP_FS_ERROR_CODE_FILE_NOT_EXIST           1401

#define LAST_BLOCK_COUNT_DOWN                                   300

#define DIR_PROCESS_REQ_TIMER                        "dir_process_req_timer"

#define PREFIX_CLI_USR_UPLOAD                         "usr_upload_"
#define PREFIX_CLI_USR_DOWNLOAD                  "usr_download_"

enum trans_status
{
    WAIT_FOR_TRANSFER = 0,
    TRANSFERING = 1,
    TRANSFER_CLOSED = 2,
    TRANSFER_ERROR = 3
};

enum exchange_type
{
    UPLOAD_FILE = 0,
    DOWNLOAD_FILE = 1
};

enum exchange_rsp_type
{
    EXCHANGE_RSP_SUCCESS = 0,
    EXCHANGE_RSP_TASK_TRANS_SESSION_ID_EXIST = 1,
    EXCAHNGE_RSP_FAILED =2
};

class udp_node
{
public:

    enum CONNECTION_STATUS
    {
        UDP_UNKNOWN = 0,
        UDP_LOGIN_FAILED = 1,
        UDP_CONNECTED = 2,
        UDP_BROKEN = 3
    };

    udp_node() : m_status(UDP_UNKNOWN), m_heartbeat_countdown(DEFAULT_HEARTBEAT_COUNTDOWN) {}

    std::string m_node_id;

    CONNECTION_STATUS m_status;

    int8_t m_heartbeat_countdown;

    std::string m_remote_ip;

    boost::asio::ip::udp::endpoint m_remote_endpoint;

};

class file_data_buf
{
public:

    file_data_buf(uint64_t size = DEFAULT_FILE_BLOCK_SIZE) 
        : m_data(new char[size](), std::default_delete<char[]>())
        , m_size(size) 
        , m_compress(false) 
        , m_checksum(0.0) {}

    file_data_buf(const char *data, uint64_t size, unsigned int checksum)
        : m_data(new char[size](), std::default_delete<char[]>())
        , m_size(size)
        , m_compress(false)
        , m_checksum(checksum)
    {
        memcpy(m_data.get(), data, size);
    }

    virtual ~file_data_buf() = default;

    char * data() { return m_data.get(); }

    uint64_t size() const { return m_size; }

    void set_checksum(unsigned int checksum) { m_checksum = checksum; }

    unsigned int get_checksum() const { return m_checksum; }

    void compress()
    {
        if (m_data && m_compress)
        {
            //compress
            std::string compress_str;
            bool enable_skip_bytes = false;
            snappy::Compress(m_data.get(), m_size, &compress_str, enable_skip_bytes);

            m_data = std::shared_ptr<char>(new char[compress_str.length()](), std::default_delete<char[]>());
            memcpy(m_data.get(), compress_str.c_str(), compress_str.length());
            m_size = compress_str.length();
        }
    }

    void uncompress()
    {
        if (m_data && m_compress)
        {
            //uncompress
            std::string uncompress_str;
            if (snappy::Uncompress(m_data.get(), m_size, &uncompress_str))
            {
                m_data = std::shared_ptr<char>(new char[uncompress_str.length()](), std::default_delete<char[]>());
                memcpy(m_data.get(), uncompress_str.c_str(), uncompress_str.length());
                m_size = uncompress_str.length();
            }
            else
            {
                assert(false);
            }
        }
    }

    std::shared_ptr<char> m_data;

    uint64_t m_size;

    unsigned int m_checksum;

    bool m_compress;
};

typedef std::shared_ptr<file_data_buf> data_buf_type;

class file_block_data
{
public:

    file_block_data(uint64_t file_block_num, data_buf_type block_data) 
        : m_file_block_num(file_block_num)
        , m_file_block_data(block_data)
        {}

    virtual ~file_block_data() = default;

    char * data() { return m_file_block_data->data(); }

    uint64_t size() { return m_file_block_data->size(); }

    unsigned int get_checksum() const { return m_file_block_data->get_checksum(); }

    bool expired()
    {
        assert(0 != m_timestamp);
        uint64_t cur_timestamp = time_util::get_mill_seconds_from_19700101();

        //10 minutes left since loaded
        if (cur_timestamp > m_timestamp && cur_timestamp - m_timestamp > 600000)
        {
            return true;
        }

        return false;
    }

    uint64_t m_timestamp = 0;

    uint64_t m_file_block_num = 0;

    data_buf_type m_file_block_data;

};

typedef std::shared_ptr<file_block_data> file_block_data_type;
typedef std::shared_ptr <std::list<file_block_data_type>> file_block_datas_type;

extern bool file_block_data_comp(const file_block_data_type &a, const file_block_data_type &b);

class http_error_rsp
{
public:

    int32_t m_http_status_code;

    std::string m_status_adjective;

    int32_t m_code;

    std::string m_message;
};

class net_entity
{
public:

    std::string m_host;

    std::string m_port;

    std::string m_protocol;
};

class http_fs_upload_req
{
public:

    std::string m_remote_path;

    std::string m_local_path;

    std::string m_root_path;

    std::list<std::shared_ptr<net_entity>> m_addrs;
};

class http_fs_upload_rsp
{
public:

    std::string m_trans_id;

};

class http_fs_download_req
{
public:

    std::string m_local_path;

    std::string m_root_path;

    std::list<std::string> m_remote_paths;

    std::list<std::shared_ptr<net_entity>> m_addrs;

};

class http_fs_download_rsp
{
public:

    std::string m_trans_id;

};

class http_fs_progress_req
{
public:

    std::string m_trans_id;

};

class trans_entity
{
public:

    std::string m_trans_id;

    std::string m_direction;

    float m_progress;

    int32_t m_trans_size;

    int32_t m_total_size;

    std::string m_local_path;

    std::string m_remote_path;

};

class http_fs_progress_rsp
{
public:

    std::list<std::shared_ptr<trans_entity>> m_trans_info;

};

class http_fs_rm_req
{
public:

    std::string m_trans_id;

};

class http_fs_rm_rsp
{
public:

};


////////////////////////////////////
//
// used by trans cli manager
//
////////////////////////////////////

#define UPLOAD_DIRECTION                "UP"
#define DOWNLOAD_DIRECTION         "DOWN"

#define FILE_TYPE                                       0
#define DIRECTORY_TYPE                         1

#define FS_ADD_TYPE                                 0
#define FS_DELETE_TYPE                          1

#define SAVE_TASK_COUNT_DOWN        600


class trans_progress
{
public:

    std::string m_trans_session_id;

    float m_progress = 0.0;

    int32_t m_trans_size = 0;

    int32_t m_total_size = 0;

    std::string m_remote_path;

};

class trans_cli_usr_task
{
public:

    std::string m_trans_id;

    std::string m_direction;

    int8_t m_file_or_dir;

    std::list<std::shared_ptr<net_entity>> m_addrs;

    trans_progress m_progress;

    std::string m_local_path;

};

class trans_cli_usr_upload_task : public trans_cli_usr_task
{
public:

    std::string m_remote_path;

    std::string m_root_path;

    std::list<std::string> m_upload_files;              //if upload dir, upload files should be needed to store here

    uint64_t m_uploaded_received_file_size = 0;

    uint64_t m_uploading_received_file_szie = 0;

    int32_t serialize(std::shared_ptr<leveldb::DB> db);

    int32_t deserialize(std::shared_ptr<leveldb::DB> db, const std::string & usr_trans_id);
};

class trans_cli_usr_download_task : public trans_cli_usr_task
{
public:

    std::list<std::string> m_remote_paths;

    std::string m_root_path;

    uint64_t m_downloaded_received_file_size = 0;

    uint64_t m_downloading_received_file_szie = 0;

    int32_t serialize(std::shared_ptr<leveldb::DB> db);

    int32_t deserialize(std::shared_ptr<leveldb::DB> db, const std::string & usr_trans_id);

};

