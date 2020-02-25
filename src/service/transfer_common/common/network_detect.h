#pragma once

#include <cstdint>


#define HEARTBEAT_REQ_AND_RSP_SIZE                       (142+142)
#define RTT_COEFFICIENT                                                          0.2
#define STARTUP_MBPS                                                    (1 * 1024 * 1024)                       //1Mbps


class network_detect
{
public:

    network_detect() : m_bps(STARTUP_MBPS), m_rtt(0.0) {}

    virtual ~network_detect() = default;

    double update(uint64_t rtt, uint32_t trans_bytes = HEARTBEAT_REQ_AND_RSP_SIZE);

protected:

    double m_bps;

    double m_rtt;

};
