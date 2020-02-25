#pragma once


#include <io/io_handler_initializer.hpp>
#include "client_connector_handler.h"


using namespace micro::core;


class connector_initializer : public io_handler_initializer
{
public:

    void init(context_chain & chain)
    {
        chain.add_last("client connector handler", std::make_shared<client_connector_handler>());
    }
};
