#pragma once

#include <cstdint>


#define HEARTBEAT_REQ_AND_RSP_SIZE                       (183+144)
#define RTT_SMOOTHING_COEFFICIENT                                0.1
#define STARTUP_MBPS                                                    (1 * 1024 * 1024)                       //1Mbps

#define RTT_GAIN_COEFFICIENT                                               30

class rate_control
{
public:

    rate_control() : m_trans_file_blocks_count_per_second(1) {}

    virtual ~rate_control() = default;

    //return file blocks count per second
    virtual uint64_t get_transfer_rate() const { return 0; }

    virtual uint64_t calc_transfer_rate() { return 0; }

    virtual double update(uint64_t rtt, uint32_t trans_bytes = HEARTBEAT_REQ_AND_RSP_SIZE) { return 0.0; }

protected:

    uint64_t m_trans_file_blocks_count_per_second;

};

class nolinear_increase_rate_control : public rate_control
{
public:

    nolinear_increase_rate_control(double max_bps) : rate_control(), m_max_bps(max_bps), m_x_axis(-6.0), m_x_step(1) {}

    virtual uint64_t get_transfer_rate() const { return m_trans_file_blocks_count_per_second; }

    virtual uint64_t calc_transfer_rate();

protected:

    double m_x_axis;

    double m_x_step;

    double m_bps;

    double m_max_bps;
};

class self_adaptive_rate_control : public rate_control
{
public:

    self_adaptive_rate_control() : rate_control(), m_bps(STARTUP_MBPS), m_rtt(1.0) {}

    virtual ~self_adaptive_rate_control() = default;

    //return file blocks count per second
    virtual uint64_t get_transfer_rate() const { return m_trans_file_blocks_count_per_second; }

    virtual uint64_t calc_transfer_rate();

   virtual double update(uint64_t rtt, uint32_t trans_bytes = HEARTBEAT_REQ_AND_RSP_SIZE);

protected:

    double m_bps;

    double m_rtt;

    double m_bdp;

};
