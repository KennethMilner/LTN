#pragma once

#include <memory>
#include "TransMessageApi.pb.h"
#include <message/message.hpp>
#include "trans_macro.h"

using namespace ::micro::core;

#define BEGIN_FILE_READ_NOTIFICATION                                              "begin_file_read_notification"
#define END_FILE_READ_NOTIFICATION                                                  "end_file_read_notification"
#define FILE_TRANSFER_NOTIFICATION                                                  "file_transfer_notification"

#define BEGIN_FILE_WRITE_NOTIFICATION                                           "begin_file_write_notification"
#define END_FILE_WRITE_NOTIFICATION                                               "end_file_write_notification"
#define FILE_BLOCK_WRITE_NOTIFICATION                                          "file_block_write_notification"

#define TRANS_MSG_LOGIN_REQ                                                                 "trans_msg_login_req"
#define TRANS_MSG_LOGIN_RSP                                                                 "trans_msg_login_rsp"
#define TRANS_MSG_HEARTBEAT_REQ                                                     "trans_msg_heartbeat_req"
#define TRANS_MSG_HEARTBEAT_RSP                                                      "trans_msg_heartbeat_rsp"
#define TRANS_MSG_LOGOUT_REQ                                                            "trans_msg_logout_req"
#define TRANS_MSG_LOGOUT_RSP                                                             "trans_msg_logout_rsp"

#define TRANS_MSG_UPLOAD_FILE_REQ                                                  "trans_msg_upload_file_req"
#define TRANS_MSG_UPLOAD_FILE_RSP                                                   "trans_msg_upload_file_rsp"

#define TRANS_MSG_STOP_UPLOAD_REQ                                                 "trans_msg_stop_upload_req"
#define TRANS_MSG_STOP_UPLOAD_RSP                                                 "trans_msg_stop_upload_rsp"

#define TRANS_MSG_SEND_DATA_REQ                                                      "trans_msg_send_data_req"
#define TRANS_MSG_BATCH_SEND_DATA_REQ                                       "trans_msg_batch_send_data_req"
#define TRANS_MSG_RESEND_DATA_NOTIFI_REQ                                 "trans_msg_resend_data_notifi_req"
#define TRANS_MSG_RESEND_DATA_NOTIFI_RSP                                  "trans_msg_resend_data_notifi_rsp"
#define TRANS_MSG_FINISH_NOTIFI_REQ                                                "trans_msg_finish_notifi_req"
#define TRANS_MSG_FINISH_NOTIFI_RSP                                                "trans_msg_finish_notifi_rsp"
#define TRANS_MSG_RESEND_DATA_REQ                                                 "trans_msg_resend_data_req"
#define TRANS_MSG_DIR_PROCESS_REQ                                                  "trans_msg_dir_process_req"
#define TRANS_MSG_DIR_PROCESS_RSP                                                   "trans_msg_dir_process_rsp"

#define TRANS_MSG_CLI_NOTIFY_SVR_DOWNLOAD_DATA_REQ      "trans_msg_cli_notify_svr_download_data_req"
#define TRANS_MSG_CLI_NOTIFY_SVR_DOWNLOAD_DATA_RSP       "trans_msg_cli_notify_svr_download_data_rsp"
#define TRANS_MSG_DOWNLOAD_FILE_REQ                                          "trans_msg_download_file_req"
#define TRANS_MSG_DOWNLOAD_FILE_RSP                                           "trans_msg_download_file_rsp"
#define TRANS_MSG_SVR_NOTIFY_CLI_END_DOWNLOAD_REQ        "trans_msg_svr_notify_cli_end_download_req"
#define TRANS_MSG_SVR_NOTIFY_CLI_END_DOWNLOAD_RSP         "trans_msg_svr_notify_cli_end_download_rsp"

#define TRANS_MSG_SVR_NOTIFY_SYNC_FILE_REQ                              "trans_msg_svr_notify_sync_file_req"
#define TRANS_MSG_SVR_NOTIFY_SYNC_DATA_REQ                            "trans_msg_svr_notify_sync_data_req"
#define TRANS_MSG_SVR_NOTIFY_SYNC_DIR_PROCESS_REQ            "trans_msg_svr_notify_sync_dir_process_req"

#define TRANS_MSG_SYNC_DOWNLOAD_FILE_REQ                              "trans_msg_sync_download_file_req"
#define TRANS_MSG_SYNC_DOWNLOAD_FILE_RSP                               "trans_msg_sync_download_file_rsp"
#define TRANS_MSG_SYNC_SEND_DATA_REQ                                          "trans_msg_sync_send_data_req"
#define TRANS_MSG_SYNC_RESEND_DATA_NOTIFI_REQ                     "trans_msg_sync_resend_data_notifi_req"
#define TRANS_MSG_SYNC_RESEND_DATA_NOTIFI_RSP                      "trans_msg_sync_resend_data_notifi_rsp"
#define TRANS_MSG_SYNC_FINISH_NOTIFI_REQ                                    "trans_msg_sync_finish_notifi_req"
#define TRANS_MSG_SYNC_FINISH_NOTIFI_RSP                                     "trans_msg_sync_finish_notifi_rsp"
#define TRANS_MSG_SYNC_RESEND_DATA_REQ                                     "trans_msg_sync_resend_data_req"
#define TRANS_MSG_SYNC_DIR_PROCESS_REQ                                       "trans_msg_sync_dir_process_req"
#define TRANS_MSG_SYNC_DIR_PROCESS_RSP                                        "trans_msg_sync_dir_process_rsp"

#define TRANS_MSG_STOP_DOWNLOAD_REQ                                          "trans_msg_stop_upload_req"
#define TRANS_MSG_STOP_DOWNLOAD_RSP                                          "trans_msg_stop_upload_rsp"

#define STOP_UPLOAD_NOTIFICATION                                                     "stop_upload_notification"
#define STOP_DOWNLOAD_NOTIFICATION                                              "stop_download_notification"


class trans_service_message : public base_body
{
public:

    trans_service_message() : m_trans_msg(std::make_shared<TransMessage>()) {}

    virtual ~trans_service_message() = default;

    std::shared_ptr<TransMessage> m_trans_msg;              //message for net transfer

};

class file_transfer_notification : public base_body
{
public:

    std::string m_file_path;

    std::string m_remote_path;
};

class begin_file_read_notification : public base_body
{
public:

    std::string m_file_path;

    uint64_t m_file_block_num;
};

class end_file_read_notification : public base_body
{
public:

    std::string m_file_path;
};

class begin_file_write_notification : public base_body
{
public:

    int32_t  m_exchange_type;

    std::string m_file_path;

    std::string m_store_file_path;

    uint64_t m_file_size;

    uint64_t m_total_file_blocks_count;

    uint64_t m_file_hash;

    std::string m_trans_session_id;
};

class end_file_write_notification : public base_body
{
public:

    std::string m_trans_session_id;

    uint64_t m_file_hash;
};

class file_block_write_notification : public base_body
{
public:

    std::string m_trans_session_id;

    uint64_t m_file_block_num;

    file_block_data_type m_file_block_data;
};

class notify_sync_file_req : public base_body
{
public:

    std::string m_node_id;

    std::string m_usr_trans_id;

    std::string m_upload_trans_session_id;

    int32_t m_exchange_type;

    std::string m_remote_path;

    std::string m_local_path;

    uint64_t m_file_size;

    uint64_t m_total_file_blocks_count;

    uint64_t m_file_block_size;

    uint64_t m_file_block_start_num;

    uint64_t m_file_hash;

};

class notify_sync_data_req : public base_body
{
public:

    std::string m_node_id;

    std::string m_trans_session_id;

    uint64_t m_file_block_num;

    file_block_data_type m_file_block_data;

    uint64_t m_block_hash;

    uint64_t m_file_hash;

};

class notify_sync_dir_process_req : public base_body
{
public:

    std::string m_node_id;

    std::string m_trans_session_id;

    int32_t  m_process_type;

    std::string m_remote_path;

};

class batch_send_data_req : public base_body
{
public:

    std::list<std::shared_ptr<TransMessage>> m_batch_trans_msgs;

};

class stop_task_notification : public base_body
{
public:

    std::string m_usr_trans_id;

};
