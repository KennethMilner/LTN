#pragma once

#include <set>
#include <list>
#include <string>
#include <map>
#include <common/error.hpp>


class resend_block_num
{
public:

    uint64_t m_file_block_num = 0;

    uint64_t m_resend_notifi_timestamp = 0;
};

struct resend_block_num_compare
{
    bool operator()(const resend_block_num & lhs, const resend_block_num & rhs) const
    {
        return lhs.m_file_block_num < rhs.m_file_block_num;
    }
};

class resend_control
{
public:

    virtual ~resend_control() = default;

    virtual uint32_t get_resend_blocks_count(const std::string & resend_key) { return 0; }

    virtual int32_t get_resend_blocks(const std::string & resend_key, uint64_t resend_count, std::list<resend_block_num> & resend_blocks) { return ERR_SUCCESS; }

    virtual int32_t add(const std::string & resend_key, const std::list<resend_block_num> & resend_blocks) { return ERR_SUCCESS; }

    virtual int32_t remove(const std::string & resend_key, const std::list<uint64_t> &  file_blocks) { return ERR_SUCCESS; }

    virtual int32_t remove_less_block_num(const std::string & resend_key, uint64_t  block_num) { return ERR_SUCCESS; }

    virtual int32_t clear(const std::string & resend_key) { return ERR_SUCCESS; }

    virtual void set_timestamp() {}

    virtual uint64_t get_timestamp() const { return 0; }

};

class default_resend_control : public resend_control
{
public:

    typedef std::shared_ptr<std::set<resend_block_num, resend_block_num_compare>> loss_blocks_type;

    default_resend_control() = default;

    virtual uint32_t get_resend_blocks_count(const std::string & resend_key);

    virtual int32_t get_resend_blocks(const std::string & resend_key, uint64_t resend_count, std::list<resend_block_num> & resend_blocks);

    virtual int32_t add(const std::string & resend_key, const std::list<resend_block_num> & resend_blocks);

    virtual int32_t remove(const std::string & resend_key, const std::list<uint64_t> &  file_blocks);

    virtual int32_t remove_less_block_num(const std::string & resend_key, uint64_t  block_num);

    virtual int32_t clear(const std::string & resend_key);

    virtual void set_timestamp() { ++m_record_timestamp; }

    virtual uint64_t get_timestamp() const { return m_record_timestamp; }

protected:

    volatile uint64_t m_record_timestamp = 0;

    uint64_t m_record_last_time_block_num = 0;

    std::map<std::string, loss_blocks_type> m_resend_blocks;
};

