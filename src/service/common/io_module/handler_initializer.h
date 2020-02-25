#pragma once


#include <io/io_handler_initializer.hpp>
#include "codec.h"

using namespace micro::core;


class handler_initializer : public io_handler_initializer
{
public:

    void init(context_chain & chain)
    {
        chain.add_last("codec handler", std::make_shared<codec>());
    }
};
