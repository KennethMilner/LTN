#include "client_connector_handler.h"


void client_connector_handler::connected(context_type &ctx)
{
    auto connector = boost::any_cast<std::shared_ptr<tcp_connector>>(ctx.get(std::string(IO_CONTEXT)));
    assert(nullptr != connector);

    std::shared_ptr<tcp_channel> ch = connector->channel();
    assert(nullptr != ch);

    auto msg_body = std::make_shared<client_connect_notification>();
    msg_body->m_status = client_connect_notification::SUCCESS;

    std::shared_ptr<message> msg = std::make_shared<message>();
    msg->m_header->m_src = ch->channel_source();
    msg->set_name(CLICENT_CONNECT_NOTIFICATION);
    msg->m_body = msg_body;

    MSG_BUS_PUB<int32_t, std::shared_ptr<message>&>(CLICENT_CONNECT_NOTIFICATION, msg);

    ctx.fire_connected();
}

void client_connector_handler::exception_caught(context_type & ctx, const std::exception & e)
{
    auto connector = boost::any_cast<std::shared_ptr<tcp_connector>>(ctx.get(std::string(IO_CONTEXT)));
    assert(nullptr != connector);

    std::shared_ptr<tcp_channel> ch = connector->channel();
    assert(nullptr != ch);

    auto msg_body = std::make_shared<client_connect_notification>();
    msg_body->m_status = client_connect_notification::FAILED;

    std::shared_ptr<message> msg = std::make_shared<message>();
    msg->m_header->m_src = ch->channel_source();
    msg->set_name(CLICENT_CONNECT_NOTIFICATION);
    msg->m_body = msg_body;

    MSG_BUS_PUB<int32_t, std::shared_ptr<message>&>(CLICENT_CONNECT_NOTIFICATION, msg);

    ctx.fire_exception_caught(e);
}
