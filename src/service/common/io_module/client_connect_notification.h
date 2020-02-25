#pragma once


#include <message/message.hpp>

using namespace micro::core;


#define CLICENT_CONNECT_NOTIFICATION "client_connect_notification"

class client_connect_notification : public base_body
{
public:

    enum connect_status
    {
        SUCCESS = 0,
        FAILED
    };

    connect_status m_status;

};

