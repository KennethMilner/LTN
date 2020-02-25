#pragma once


#include <io/io_handler_initializer.hpp>
#include "trans_codec.h"

using namespace micro::core;


class trans_handler_initializer : public io_handler_initializer
{
public:

    void init(context_chain & chain)
    {
        chain.add_last("trans codec handler", std::make_shared<trans_codec>());
    }
};
