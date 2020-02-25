#pragma once


#include <memory>
#include <io/io_handler.hpp>
#include <io/tcp_connector.hpp>
#include <io/tcp_channel.hpp>
#include <bus/message_bus.hpp>
#include "client_connect_notification.h"


using namespace micro::core;

class client_connector_handler : public tcp_connector_handler
{
public:

    void connected(context_type &ctx);

    void exception_caught(context_type & ctx, const std::exception & e);

};