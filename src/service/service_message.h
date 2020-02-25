#pragma once

#include <memory>
#include "NetMessageApi.pb.h"
#include <message/message.hpp>

using namespace ::micro::core;


#define CHANNEL_NOTIFICATION       "channel_notification"
#define LOGIN_REQ                                   "login_req"
#define LOGIN_RSP                                    "login_rsp"
#define DEV_REG_REQ                              "dev_reg_req"
#define DEV_REG_RSP                               "dev_reg_rsp"
#define HEARTBEAT_REQ                       "heartbeat_req"
#define HEARTBEAT_RSP                        "heartbeat_rsp"
#define START_TASK_REQ                       "start_task_req"
#define STOP_TASK_REQ                          "stop_task_req"
#define REPORT_TASK_STATE_RSP      "report_task_state_rsp"
#define GET_TASK_STATE_REQ              "get_task_state_req"


class service_message : public base_body
{
public:

    service_message() : m_net_msg(std::make_shared<NetMessage>())  {}

    virtual ~service_message() = default;

    std::shared_ptr<NetMessage> m_net_msg;              //message for net transfer

};

class channel_notification : public base_body
{
public:

    enum status
    {
        BROKEN = 0
    };

    status m_status;

    std::string m_info;

};

