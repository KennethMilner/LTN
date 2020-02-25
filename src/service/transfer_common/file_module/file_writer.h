#pragma once

#include "trans_macro.h"

#define FILE_BLOCK_CACHE_COUNT          128


class file_writer
{
public:

    typedef std::mutex mutex_type;

    file_writer(const std::string &path, uint64_t file_size, uint64_t file_blocks_count): m_path(path), m_file_blocks_count(file_blocks_count), m_file_size(file_size) 
    {
        m_os_charset_path = OS_TEXT(m_path);
    }

    virtual ~file_writer() { close(); }

    virtual int32_t open();

    virtual int32_t close();

    virtual int32_t write_blocks();

    virtual int32_t create_null_file();

    virtual int32_t add(file_block_data_type block_data);

    void set_file_hash(uint64_t file_hash) { m_file_hash = file_hash; }

    const std::string & get_file_path() const { return m_path; }

    const std::string & get_os_charset_file_path() const { return m_os_charset_path; }

    virtual int32_t validate_checksum();

protected:

    uint64_t get_file_blocks_count() const { return m_file_blocks_count; }

    uint64_t get_file_size() const { return m_file_size; }

protected:

    std::string m_path;

    std::string m_os_charset_path;

    std::ofstream m_ofs;

    mutex_type m_mutex;

    uint64_t m_file_size;

    uint64_t m_file_hash;

    uint64_t m_file_blocks_count;

    std::map<uint64_t, file_block_data_type> m_data_bufs;

    std::list<file_block_data_type> m_data_list;

};