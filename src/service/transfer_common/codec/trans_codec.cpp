#include "trans_codec.h"
#include <cassert>
#include <sstream>
#include <io/udp_channel.hpp>
#include <io/context_chain.hpp>
#include <bus/message_bus.hpp>
#include "bytes_order.h"
#include "TransMessageApi.pb.h"
#include "trans_service_message.h"
#include "util.h"
#include "snappy.h"


send_data *create_send_data(std::shared_ptr<message> msg, const std::string & snd_str)
{
    send_data *snd_data = new send_data();                        //malloc should check

    uv_ip4_addr(GET_IP(msg->get_dst_endpoint()), msg->get_dst_endpoint().port(), &snd_data->m_send_addr);

    snd_data->m_uv_buf_count = 1;
    snd_data->m_uv_buf = (uv_buf_t *)malloc(sizeof(uv_buf_t));

    snd_data->m_uv_buf->base = (char *)malloc(snd_str.length());
    snd_data->m_uv_buf->len = snd_str.length();
    memcpy(snd_data->m_uv_buf->base, snd_str.c_str(), snd_data->m_uv_buf->len);

    return snd_data;
}

void trans_codec::channel_read_complete(context_type &ctx)
{
    auto ch = boost::any_cast<std::shared_ptr<udp_channel>>(ctx.get(std::string(IO_CONTEXT)));
    assert(nullptr != ch);

    boost::asio::ip::udp::endpoint remote_endpoint = ch->get_remote_endpoint();

    std::shared_ptr<io_streambuf> in = ch->recv_buf();
    assert(nullptr != in);

    //LOG_DEBUG << "channel read buf: " << in->to_string();

    if (in->get_valid_read_len() <= 0)
    {
        LOG_ERROR << "trans codec io stream buff NOT enough";
        return ctx.fire_channel_read_complete();
    }

    std::string str;

    if (m_compress)
    {
        if (!snappy::Uncompress(in->get_read_ptr(), in->get_valid_read_len(), &str))
        {
            LOG_ERROR << "trans codec io stream buff uncompress FAILED";
            return ctx.fire_channel_read_complete();
        }
    }
    else
    {
        str.assign(in->get_read_ptr(), in->get_valid_read_len());
    }

    auto svc_msg = std::make_shared<trans_service_message>();
    assert(nullptr != svc_msg && nullptr != svc_msg->m_trans_msg);

    std::runtime_error e("trans message decode error");
    if (!svc_msg->m_trans_msg->ParseFromString(str))
    {
        LOG_ERROR << "trans codec read parse error";
        return ctx.fire_exception_caught(e);
    }

    //in->move_read_ptr(in->get_valid_read_len());

    //inner message sent to module
    auto msg = std::make_shared<message>();
    msg->set_name(svc_msg->m_trans_msg->head().msg_name());
    msg->m_header->m_src.m_endpoint = remote_endpoint;
    msg->m_header->m_src.m_proto_type = UDP_TYPE;

    const std::string &msg_name = msg->get_name();
    if (  msg_name == TRANS_MSG_UPLOAD_FILE_REQ 
        || msg_name == TRANS_MSG_UPLOAD_FILE_RSP 
        || msg_name == TRANS_MSG_FINISH_NOTIFI_REQ 
        || msg_name == TRANS_MSG_FINISH_NOTIFI_RSP
        || msg_name == TRANS_MSG_DIR_PROCESS_REQ
        || msg_name == TRANS_MSG_DIR_PROCESS_RSP)
    {
        msg->set_priority(HIGH_PRIORITY);
    }

    if (msg_name == TRANS_MSG_RESEND_DATA_NOTIFI_REQ)
    {
        msg->set_priority(MIDDLE_PRIORITY);
    }

    msg->m_body = svc_msg;

    //print msg content
    LOG_DEBUG << "udp channel read: " << svc_msg->m_trans_msg->Utf8DebugString();

    MSG_BUS_PUB<int32_t, std::shared_ptr<message>&>(msg->get_name(), msg);

    return ctx.fire_channel_read_complete();
}

void trans_codec::channel_write(context_type &ctx)
{
    auto ch = boost::any_cast<std::shared_ptr<udp_channel>>(ctx.get(std::string(IO_CONTEXT)));
    assert(nullptr != ch);

    //get front message
    std::shared_ptr<message> msg = ch->front_message();
    assert(nullptr != msg);

    //pop front message
    ch->pop_front_message();

    //serialization
    std::string str;
    auto svc_msg = std::dynamic_pointer_cast<trans_service_message>(msg->m_body);
    svc_msg->m_trans_msg->SerializeToString(&str);

    send_data *snd_data = nullptr;

    //compress
    std::string compress_str;

    if (m_compress)
    {
        bool enable_skip_bytes = false;
        snappy::Compress(str.c_str(), str.length(), &compress_str, enable_skip_bytes);
        snd_data = create_send_data(msg, compress_str);
    }
    else
    {
        snd_data = create_send_data(msg, str);
    }

    ch->get_send_bufs().push_back(snd_data);

    //print msg content
    LOG_DEBUG << "udp channel write: " << svc_msg->m_trans_msg->Utf8DebugString();

    //LOG_DEBUG << "udp channel " << ch->channel_source().to_string() << " send buf: " << out->to_string();

    return ctx.fire_channel_write();
}

void trans_codec::channel_batch_write(context_type &ctx)
{
    auto ch = boost::any_cast<std::shared_ptr<udp_channel>>(ctx.get(std::string(IO_CONTEXT)));
    assert(nullptr != ch);

    //get front message
    auto batch_msg = ch->front_batch_message();
    assert(nullptr != batch_msg);

    //pop front message
    ch->pop_front_batch_message();

    size_t send_buf_count = batch_msg->m_msgs.size();
    if (0 == send_buf_count)
    {
        return ctx.fire_channel_batch_write();
    }

    send_data *snd_data = new send_data();

    snd_data->m_uv_buf = (uv_buf_t *)malloc(sizeof(uv_buf_t) * send_buf_count);
    snd_data->m_uv_buf_count = send_buf_count;

    uv_buf_t *uv_buf = nullptr;

    for (int i = 0; i < send_buf_count; i++)
    {
        auto msg = batch_msg->m_msgs.front();
        batch_msg->m_msgs.pop_front();

        uv_buf = snd_data->m_uv_buf + i;

        //serialization
        std::string str;
        auto svc_msg = std::dynamic_pointer_cast<trans_service_message>(msg->m_body);
        svc_msg->m_trans_msg->SerializeToString(&str);

        //compress
        std::string compress_str;

        if (m_compress)
        {
            bool enable_skip_bytes = false;
            snappy::Compress(str.c_str(), str.length(), &compress_str, enable_skip_bytes);

            uv_buf->base = (char *)malloc(compress_str.length());
            uv_buf->len = compress_str.length();
            memcpy(uv_buf->base, compress_str.c_str(), uv_buf->len);
        }
        else
        {
            uv_buf->base = (char *)malloc(str.length());
            uv_buf->len = str.length();
            memcpy(uv_buf->base, str.c_str(), uv_buf->len);
        }

        //LOG_DEBUG << "udp channel write: " << svc_msg->m_trans_msg->Utf8DebugString();
    }

    uv_ip4_addr(GET_IP(batch_msg->m_dst_endpoint), batch_msg->m_dst_endpoint.port(), &snd_data->m_send_addr);

    ch->get_send_bufs().push_back(snd_data);

    return ctx.fire_channel_batch_write();
}

void trans_codec::exception_caught(context_type & ctx, const std::exception & e)
{
    auto ch = boost::any_cast<std::shared_ptr<udp_channel>>(ctx.get(std::string(IO_CONTEXT)));
    assert(nullptr != ch);

    //boost::asio::ip::udp::endpoint remote_endpoint = ch->get_remote_endpoint();
    //LOG_ERROR << "udp channel exception: " << e.what();

    return ctx.fire_exception_caught(e);
}
