#include "resend_scheduler.h"
#include <common/error.hpp>
#include <logger/logger.hpp>
#include "util.h"
#include "trans_macro.h"


uint64_t resend_scheduler::get_resend_blocks_count(const std::string & trans_session_id) 
{
    auto it = m_loss_blocks.find(trans_session_id);
    if (m_loss_blocks.end() == it)
    {
        return 0;
    }

    return it->second->m_losses->size();
}

int32_t resend_scheduler::get_resend_blocks(const std::string & trans_session_id, uint64_t resend_count, std::list<uint64_t> & resend_blocks)
{
    auto it = m_loss_blocks.find(trans_session_id);
    if (m_loss_blocks.end() == it)
    {
        return ERR_FAILED;
    }

    //loss blocks
    uint64_t count = 0;
    auto losses = it->second->m_losses;
    for (auto it_block = losses->begin(); losses->end() != it_block; it_block++)
    {
        if (++count > resend_count)
        {
            break;
        }

        resend_blocks.push_back(*it_block);
    }

    return ERR_SUCCESS;
}

uint64_t resend_scheduler::get_first_resend_blocks_num(const std::string & trans_session_id)
{
    auto it = m_loss_blocks.find(trans_session_id);
    if (m_loss_blocks.end() == it)
    {
        return 0;
    }

    auto losses = it->second->m_losses;
    if (losses->empty())
    {
        return 0;
    }

    return *(losses->begin());
}

int32_t resend_scheduler::add(const std::string & trans_session_id, const std::list<uint64_t> & loss_blocks)
{
    resend_loss_blocks_type blks = nullptr;

    auto it = m_loss_blocks.find(trans_session_id);
    if (m_loss_blocks.end() == it)
    {
        blks = std::make_shared<resend_loss_blocks>();
        m_loss_blocks[trans_session_id] = blks;
    }
    else
    {
        blks = it->second;
    }

    //check each loss block
    for (auto loss : loss_blocks)
    {
        blks->m_losses->insert(loss);
    }

    /*std::string str;
    for (auto blk : *blks)
    {
        str += std::to_string(blk);
        str += " ";
    }

    LOG_DEBUG << "resend schedule add, NOW loss blocks num: " << str;*/

    return ERR_SUCCESS;
}

int32_t resend_scheduler::remove(const std::string & trans_session_id, const std::list<uint64_t> &  loss_blocks)
{ 
    auto it = m_loss_blocks.find(trans_session_id);
    if (m_loss_blocks.end() == it)
    {
        return ERR_FAILED;
    }

    auto blks = it->second->m_losses;
    for (auto loss : loss_blocks)
    {
        blks->erase(loss);
    }

    /*std::string str;
    std::string str;
    for (auto blk : *blks)
    {
        str += std::to_string(blk);
        str += " ";
    }

    LOG_DEBUG << "resend schedule remove, NOW loss blocks num: " << str;*/

    return ERR_SUCCESS;
}

int32_t resend_scheduler::remove(const std::string & trans_session_id, uint64_t  loss_block)
{
    auto it = m_loss_blocks.find(trans_session_id);
    if (m_loss_blocks.end() == it)
    {
        return ERR_FAILED;
    }

    auto blks = it->second->m_losses;
    blks->erase(loss_block);

    LOG_DEBUG << "resend schedule removed loss block: " << std::to_string(loss_block);

    /*std::string str;
    for (auto blk : *blks)
    {
        str += std::to_string(blk);
        str += " ";
    }

    LOG_DEBUG << "resend schedule remove, NOW loss blocks num: " << str;*/

    return ERR_SUCCESS;
}


int32_t resend_scheduler::clear(const std::string & trans_session_id) 
{ 
    auto it = m_loss_blocks.find(trans_session_id);
    if (m_loss_blocks.end() == it)
    {
        return ERR_SUCCESS;
    }

    auto blks = it->second->m_losses;
    blks->clear();

    m_loss_blocks.erase(it);

    return ERR_SUCCESS;
}

bool resend_scheduler::due_to_resend(const std::string & trans_session_id)
{
    auto it = m_loss_blocks.find(trans_session_id);
    if (m_loss_blocks.end() == it)
    {
        return false;
    }

    auto blks = it->second;

    uint64_t timestamp = time_util::get_mill_seconds_from_19700101();

    if (timestamp >= blks->m_next_resend_timestamp)
    {
        LOG_DEBUG << "current timestamp: " << std::to_string(timestamp / 1000.0) << " next resend timestamp: " << std::to_string(blks->m_next_resend_timestamp / 1000.0);
        return true;
    }

    return false;
}

void resend_scheduler::update_path_rtt(const std::string & trans_session_id, uint64_t new_rtt)
{
    auto it = m_loss_blocks.find(trans_session_id);
    if (m_loss_blocks.end() == it)
    {
        return;
    }

    auto blks = it->second;
    blks->m_path_rtt->calc(new_rtt);
}

void resend_scheduler::calc_next_send_timestamp(const std::string & trans_session_id, uint64_t cur_timestamp)
{
    auto it = m_loss_blocks.find(trans_session_id);
    if (m_loss_blocks.end() == it)
    {
        return;
    }

    auto blks = it->second;
    blks->m_next_resend_timestamp = cur_timestamp + blks->m_path_rtt->get();
    //LOG_INFO << "resend scheduler calc next send timestamp: " << std::to_string(blks->m_next_resend_timestamp / 1000.0);
}
