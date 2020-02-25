#include "path_rtt.h"
#include <cmath>
#include <string>
#include <logger/logger.hpp>
#include "util.h"

uint64_t smooth_path_rtt::calc(uint64_t measure_rtt)
{
    if (false == m_rtt_inited)
    {
        m_rtt = (int64_t)measure_rtt;
        m_srtt = measure_rtt;
        m_vrtt = DEFAULT_PREDICT_RTT;

        m_rtt_inited = true;
    }

    m_next_srtt = m_srtt + m_gain_factor * (m_rtt - m_srtt);
    m_next_vrtt = m_vrtt + m_attenuation_factor * std::fabs((m_rtt - m_srtt) * 1.0);
    m_predict_rtt = m_next_srtt + OSCILLATION_FACTOR * m_next_vrtt;

    m_rtt = (int64_t)measure_rtt;
    m_srtt = m_next_srtt;
    m_vrtt = m_next_vrtt;

    BEGIN_COUNT_TO_DO(RTT, 100)
    LOG_INFO << "measurement rtt: " << std::to_string(measure_rtt) << " predict rtt: " << std::to_string(m_predict_rtt) << " srtt: " << std::to_string(m_srtt) << " vrtt: " << std::to_string(m_vrtt);
    END_COUNT_TO_DO

        int64_t rtt = 0;
    rtt = (m_predict_rtt < MIN_PREDICT_RTT) ? MIN_PREDICT_RTT : m_predict_rtt;
    rtt = (m_predict_rtt > MAX_PREDICT_RTT) ? MAX_PREDICT_RTT : m_predict_rtt;
    return rtt;
}

uint64_t kalman_path_rtt::calc(uint64_t measure_rtt)
{
    m_mid_rtt = m_last_rtt;                                                                                               //m_mid_rtt = x(k|k-1),           m_last_rtt = x(k-1|k-1)
    m_mid_p = m_last_p + KPT_Q;                                                                                  //m_mid_p = P(k|k-1),              m_last_p=P(k-1|k-1),                KPT_Q = noise
    m_kg = m_mid_p / (m_mid_p + KPT_R);                                                                //m_kg kalman gain factor,     KPT_R = noise

    m_predict_rtt = m_mid_rtt + m_kg * ((int64_t)measure_rtt - m_mid_rtt);                   //predict rtt

    m_now_p = (1 - m_kg) * m_mid_p;
    m_last_p = m_now_p;
    m_last_rtt = m_predict_rtt;

    BEGIN_COUNT_TO_DO(RTT, 1000)
    LOG_INFO << "measurement rtt: " << std::to_string(measure_rtt) << " predict rtt: " << std::to_string(m_predict_rtt);
    END_COUNT_TO_DO

    int64_t rtt = 0;
    rtt = (m_predict_rtt < MIN_PREDICT_RTT) ? MIN_PREDICT_RTT : m_predict_rtt;
    rtt = (m_predict_rtt > MAX_PREDICT_RTT) ? MAX_PREDICT_RTT : m_predict_rtt;
    return rtt;
}
