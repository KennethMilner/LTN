#pragma once

#include <cstdint>


#define GAIN_FACTOR                                 0.125
#define ATTENUATION_FACTOR              0.25
#define OSCILLATION_FACTOR                0.1

#define DEFAULT_PREDICT_RTT             200                //microseconds
#define MIN_PREDICT_RTT                       100
#define MAX_PREDICT_RTT                      3000

#define KPT_Q                           0.018
#define KPT_R                           0.542
#define KPT_LAST_P                 0.02


class path_rtt
{
public:

    path_rtt() = default;

    virtual ~path_rtt() = default;

    virtual uint64_t calc(uint64_t measure_rtt) { return 0; }

    virtual uint64_t get() const { return 0; }

};

class smooth_path_rtt : public path_rtt
{
public:

    smooth_path_rtt() : m_rtt_inited(false), m_gain_factor(GAIN_FACTOR), m_attenuation_factor(ATTENUATION_FACTOR), m_predict_rtt(DEFAULT_PREDICT_RTT), m_srtt(DEFAULT_PREDICT_RTT), m_vrtt(0) {}

    virtual ~smooth_path_rtt() = default;

    virtual uint64_t calc(uint64_t measure_rtt);

    virtual uint64_t get() const { return m_predict_rtt; }

protected:

    bool m_rtt_inited;

    float m_gain_factor;

    float m_attenuation_factor;

    int64_t m_rtt;

    int64_t m_srtt;

    int64_t m_vrtt;

    int64_t m_next_srtt;

    int64_t m_next_vrtt;

    int64_t m_predict_rtt;                 //microseconds

};

class kalman_path_rtt : public path_rtt
{
public:

    kalman_path_rtt()
        : m_last_rtt(DEFAULT_PREDICT_RTT)
        , m_mid_rtt(DEFAULT_PREDICT_RTT)
        , m_predict_rtt(DEFAULT_PREDICT_RTT)
    {}

    virtual ~kalman_path_rtt() = default;

    virtual uint64_t calc(uint64_t measure_rtt);

    virtual uint64_t get() const { return m_predict_rtt; }

protected:

    double m_mid_p;

    double m_now_p;

    double m_kg;

    double m_predict_p;

    double m_last_p;

    int64_t m_last_rtt;

    int64_t m_mid_rtt;

    int64_t m_predict_rtt;                 //microseconds
};