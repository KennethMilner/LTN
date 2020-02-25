#include "rate_control.h"
#include "file_reader.h"


uint64_t nolinear_increase_rate_control::calc_transfer_rate()
{
    //coefficient
    double coefficient = 1 / (1 + exp(-m_x_axis));
    assert(coefficient <= 1.0);

    //transfer rate
    m_bps = m_max_bps * coefficient;
    m_x_axis += m_x_step;

    //transfer blocks count
    uint64_t blocks_count = (uint64_t)(m_bps / 8 / DEFAULT_FILE_BLOCK_SIZE);
    m_trans_file_blocks_count_per_second = blocks_count ? blocks_count : 1;

    //LOG_DEBUG << "rate control coefficient: " << std::to_string(coefficient) << " bps: " << std::to_string(m_bps) << " trans blocks count: " << std::to_string(m_trans_file_blocks_count_per_second);

    return m_trans_file_blocks_count_per_second;
}

uint64_t self_adaptive_rate_control::calc_transfer_rate()
{
    //transfer blocks count
    uint64_t blocks_count = (uint64_t)(m_bps / 8 / DEFAULT_FILE_BLOCK_SIZE);
    m_trans_file_blocks_count_per_second = blocks_count ? blocks_count : 1;

    LOG_DEBUG << "rate control bps: " << std::to_string(m_bps) << " trans blocks count: " << std::to_string(m_trans_file_blocks_count_per_second);

    return m_trans_file_blocks_count_per_second;
}

double self_adaptive_rate_control::update(uint64_t rtt, uint32_t trans_bytes)
{
    //transfered bits
    uint32_t trans_bits = trans_bytes * 8;

    //calc rtt: us
    m_rtt = (1 - RTT_SMOOTHING_COEFFICIENT) * m_rtt + RTT_SMOOTHING_COEFFICIENT * rtt;

    //calc real-time network speed: bps
    m_bps = trans_bits / m_rtt * RTT_GAIN_COEFFICIENT * 1000 * 1000;

    //m_bdp = (m_bps / 8) * (m_rtt / 1000 / 1000);

    LOG_DEBUG << "self adaptive rate control bps: " << std::to_string(m_bps) << " rtt: " << std::to_string(m_rtt);

    return m_bps;
}
