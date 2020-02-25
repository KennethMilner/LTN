#include "network_detect.h"
#include <logger/logger.hpp>
#include <string>


double network_detect::update(uint64_t rtt, uint32_t trans_bytes)
{
    //transfered bits
    uint32_t trans_bits = trans_bytes * 8;

    //calc rtt: us
    m_rtt = (1 - RTT_COEFFICIENT) * m_rtt + RTT_COEFFICIENT * rtt;

    //calc real-time network speed: mbps
    m_bps = trans_bits / m_rtt * 1000 * 1000;

    LOG_DEBUG << "network detect mbps: " << std::to_string(m_bps);

    return m_bps;
}