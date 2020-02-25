#include "resend_control.h"
#include <logger/logger.hpp>
#include <algorithm>
#include <common/common.hpp>


uint32_t default_resend_control::get_resend_blocks_count(const std::string & resend_key)
{
    auto it = m_resend_blocks.find(resend_key);
    if (m_resend_blocks.end() == it)
    {
        return 0;
    }

    return it->second->size();
}

int32_t default_resend_control::get_resend_blocks(const std::string & resend_key, uint64_t resend_count, std::list<resend_block_num> & resend_blocks)
{
    auto it = m_resend_blocks.find(resend_key);
    if (m_resend_blocks.end() == it)
    {
        return ERR_FAILED;
    }

    auto losses = it->second;
    auto loss_it = losses->begin();

    //resend blocks
    uint64_t count = 0;
    for (; loss_it != losses->end(); loss_it++)
    {
        if (count++ > resend_count)
        {
            break;
        }

        resend_block_num resend_blk;
        resend_blk.m_file_block_num = loss_it->m_file_block_num;
        resend_blk.m_resend_notifi_timestamp = loss_it->m_resend_notifi_timestamp;

        resend_blocks.push_back(*loss_it);
        //LOG_INFO << std::to_string(loss_it->m_file_block_num) << " ";
    }

    if (resend_blocks.size())
    {
        losses->erase(losses->begin(), loss_it);
    }

    return ERR_SUCCESS;
}

//int32_t default_resend_control::get_resend_blocks(const std::string & resend_key, uint64_t resend_count, std::list<uint64_t> & resend_blocks)
//{
//    auto it = m_resend_blocks.find(resend_key);
//    if (m_resend_blocks.end() == it)
//    {
//        return ERR_FAILED;
//    }
//
//    auto losses = it->second;
//    auto loss_it = losses->upper_bound(m_record_last_time_block_num);
//
//    if (loss_it != losses->end())
//    {
//        LOG_INFO << "resend block upper bound: " << *loss_it;
//    }
//    else
//    {
//        LOG_INFO << "resend block NOT find upper bound: " << std::to_string(m_record_last_time_block_num);
//    }
//
//    //resend blocks
//    uint64_t count = 0;
//    for (; loss_it != losses->end(); loss_it++)
//    {
//        if (count++ >= resend_count)
//        {
//            break;
//        }
//
//        resend_blocks.push_back(*loss_it);
//        //LOG_INFO << std::to_string(*loss_it) << " ";
//    }
//
//    LOG_INFO << "resend block count: " << std::to_string(resend_blocks.size());
//
//    if (resend_blocks.size())
//    {
//        m_record_last_time_block_num = resend_blocks.back();
//    }
//    else if (losses->size())
//    {
//        resend_blocks.push_back(*(losses->rbegin()));
//    }
//
//
//    /*else if (loss_it == losses->end() && losses->size())
//    {
//        m_record_last_time_block_num = *(losses->begin());
//    }*/
//
//    std::string log_str;
//    for (auto blk_num : resend_blocks)
//    {
//        log_str += std::to_string(blk_num);
//        log_str += " ";
//    }
//
//    LOG_INFO << log_str;
//
//    return ERR_SUCCESS;
//}

int32_t default_resend_control::add(const std::string & resend_key, const std::list<resend_block_num> & resend_blocks)
{
    loss_blocks_type blks = nullptr;

    auto it = m_resend_blocks.find(resend_key);
    if (m_resend_blocks.end() == it)
    {
        blks = std::make_shared<std::set<resend_block_num, resend_block_num_compare>>();
        m_resend_blocks[resend_key] = blks;
    }
    else
    {
        blks = it->second;
    }

    //check each resend block
    for (auto resend_block : resend_blocks)
    {
        blks->insert(resend_block);
    }

    /*std::string str;
    for (auto it_blk = blks->begin(); it_blk != blks->end(); it_blk++)
    {
        str += std::to_string(it_blk->m_file_block_num);
        str += " ";
    }

    LOG_DEBUG << "resend control add, NOW resend blocks num: " << str;*/

    return ERR_SUCCESS;
}

int32_t default_resend_control::remove(const std::string & resend_key, const std::list<uint64_t> &  file_blocks)
{
    auto it = m_resend_blocks.find(resend_key);
    if (m_resend_blocks.end() != it)
    {
        LOG_DEBUG << "resend control NOT find file path: " << resend_key;
        return ERR_FAILED;
    }

    auto blks = it->second;
    resend_block_num resend_blk;

    for (auto resend_block_num : file_blocks)
    {
        resend_blk.m_file_block_num = resend_block_num;
        blks->erase(resend_blk);
    }

    /*std::string str;
    for (auto it_blk = blks->begin(); it_blk != blks->end(); it_blk++)
    {
        str += std::to_string(it_blk->m_file_block_num);
        str += " ";
    }

    LOG_DEBUG << "resend control remove, NOW resend blocks num: " << str;*/

    return ERR_SUCCESS;
}

int32_t default_resend_control::remove_less_block_num(const std::string & resend_key, uint64_t  block_num)
{
    /*auto it = m_resend_blocks.find(resend_key);
    if (m_resend_blocks.end() == it)
    {
        return ERR_SUCCESS;
    }

    auto blks = it->second;
    std::list<uint64_t> removes;

    for (auto it = blks->begin(); it != blks->end(); it++)
    {
        if (*it < block_num)
        {
            removes.push_back(*it);
        }
        else
        {
            break;
        }
    }

    for (auto remove : removes)
    {
        blks->erase(remove);
    }*/

    return ERR_SUCCESS;
}

int32_t default_resend_control::clear(const std::string & resend_key)
{
    auto it = m_resend_blocks.find(resend_key);
    if (m_resend_blocks.end() == it)
    {
        return ERR_SUCCESS;
    }

    auto blks = it->second;
    blks->clear();

    m_record_last_time_block_num = 0;

    return ERR_SUCCESS;
}

