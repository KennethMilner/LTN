#pragma once


#include "trans_macro.h"


#define SEGMENT_LOAD_BLOCKS_COUNT         4096
#define AHEAD_OF_SCHEDULE_BATCH               10
#define SEGMENT_LOAD_BUFFER_SIZE              (SEGMENT_LOAD_BLOCKS_COUNT * DEFAULT_FILE_BLOCK_SIZE)           //4096 blocks for on time load
#define MIN_LOAD_BLOCKS_COUNT                    (SEGMENT_LOAD_BLOCKS_COUNT * AHEAD_OF_SCHEDULE_BATCH)

#define LOW_WATER_FILE_BLOCK_COUNT       (16 * SEGMENT_LOAD_BLOCKS_COUNT)      //65536 file blocks
#define HIGH_WATER_FILE_BLOCKS_COUNT      (1024 * SEGMENT_LOAD_BLOCKS_COUNT)


class file_reader
{
public:

    typedef std::mutex mutex_type;

    file_reader(const std::string &path) 
        : m_ready(false)
        , m_path(path)
        , m_file_blocks_count(0)
        , m_size_calced(false)
        , m_file_size(0)
        , m_hash_calced(false)
        , m_crc_file_hash(0)
        , m_min_block_num_loaded(0)
        , m_max_block_num_loaded(0)
        , m_send_speed_per_second(1)
        , m_load_speed_per_second(SEGMENT_LOAD_BLOCKS_COUNT)
        , m_crc_calc_idx(MIN_FILE_BLOCK_NUM)
    {
        m_os_charset_path = OS_TEXT(m_path);
    }
    
    virtual ~file_reader() { close(); }

public:

    virtual int32_t open();
    
    virtual int32_t close();

    virtual int32_t load_blocks_on_timer();

public:

    bool are_u_ready() const { return m_ready; }

    void notify_send_per_second(uint64_t send_speed_per_second);

    uint64_t get_file_size();

    unsigned int get_file_hash();

    unsigned int calc_file_hash();

    const std::string & get_file_path() const { return m_path; }
    
    uint64_t get_file_blocks_total_count() const { return m_file_blocks_count; }
    
    uint64_t get_max_block_num_loaded() const { return m_max_block_num_loaded; }

    file_block_data_type get(uint64_t file_block_num);

    void notify_missing(uint64_t file_block_num);

    virtual void expire();

protected:

    int32_t random_load(uint64_t file_block_num);

    int32_t sequence_load(uint64_t file_block_num/*begin from file block num, min num from 1*/, uint64_t blocks_count = SEGMENT_LOAD_BLOCKS_COUNT);

    void load_missing();

    void load_new_blocks();

    int32_t get_block_start_position(uint64_t file_block_num, uint64_t &start_pos, uint64_t &size);

protected:

    bool m_ready;

    std::string m_path;

    std::string m_os_charset_path;

    std::ifstream m_ifs;

    bool m_size_calced;

    uint64_t m_file_size;

    bool m_hash_calced;

    unsigned int m_crc_file_hash;

    uint64_t m_file_blocks_count;

    uint64_t m_min_block_num_loaded;

    uint64_t m_max_block_num_loaded;

    mutex_type m_mutex;

    std::list<file_block_data_type> m_pre_load_block_datas;

    std::map<uint64_t, file_block_data_type> m_block_datas;       //rb tree

    mutex_type m_missing_mutex;

    std::map<uint64_t, uint64_t> m_missing_block_nums;

    uint64_t m_send_speed_per_second;

    uint64_t m_load_speed_per_second;

    uint64_t m_crc_calc_idx;

    boost::crc_32_type m_crc_32;

};

