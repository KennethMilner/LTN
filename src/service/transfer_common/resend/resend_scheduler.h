#pragma once

#include <set>
#include <list>
#include <map>
#include <string>
#include <cstdint>
#include <memory>
#include "path_rtt.h"


#define MAX_RESEND_FILE_BLOCKS_COUNT                                128
#define MIN_RESEND_GROUP_TOTAL_PACKET_COUNT             1
#define MAX_RESEND_GROUP_TOTAL_PACKET_COUNT            100

typedef std::shared_ptr<std::set<uint64_t>> loss_blocks_type;


class resend_loss_blocks
{
public:

    resend_loss_blocks() : m_losses(std::make_shared<std::set<uint64_t>>()), m_path_rtt(std::make_shared<kalman_path_rtt>()), m_next_resend_timestamp(0) {}

    loss_blocks_type m_losses;

    std::shared_ptr<path_rtt> m_path_rtt;

    uint64_t m_next_resend_timestamp;

};

class resend_scheduler
{
public:

    typedef std::shared_ptr<resend_loss_blocks> resend_loss_blocks_type;

    resend_scheduler() = default;

    virtual uint64_t get_resend_blocks_count(const std::string & trans_session_id);

    virtual int32_t get_resend_blocks(const std::string & trans_session_id, uint64_t resend_count, std::list<uint64_t> & resend_blocks);

    virtual uint64_t get_first_resend_blocks_num(const std::string & trans_session_id);

    virtual int32_t add(const std::string & trans_session_id, const std::list<uint64_t> & loss_blocks);

    virtual int32_t remove(const std::string & trans_session_id, const std::list<uint64_t> &  loss_blocks);

    virtual int32_t remove(const std::string & trans_session_id, uint64_t  loss_block);

    virtual int32_t clear(const std::string & trans_session_id);

    virtual bool due_to_resend(const std::string & trans_session_id);

    virtual void update_path_rtt(const std::string & trans_session_id, uint64_t new_rtt);

    virtual void calc_next_send_timestamp(const std::string & trans_session_id, uint64_t cur_timestamp);

protected:

    std::map<std::string, resend_loss_blocks_type> m_loss_blocks;              //trans session id -> resend loss file block num

};

