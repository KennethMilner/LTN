#pragma once

#include <io/io_handler.hpp>
#include <io/io_streambuf.hpp>

using namespace micro::core;


class trans_codec : public channel_inbound_handler, public channel_outbound_handler
{
public:

    trans_codec() : m_compress(false) {}

    void channel_read_complete(context_type &ctx);

    void channel_write(context_type &ctx);

    void channel_batch_write(context_type &ctx);

    void exception_caught(context_type & ctx, const std::exception & e);

protected:

    bool m_compress;

};
