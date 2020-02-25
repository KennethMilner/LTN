#include "timer_generator_init.h"
#include <timer/timer_generator.hpp>
#include <logger/logger.hpp>
#include <common/error.hpp>
#include <common/any_map.hpp>
#include "server.h"

using namespace micro::core;

STATIC_INIT_LUA_MANAGER_INJECTION(timer_generator, TIMER_GENERATOR);