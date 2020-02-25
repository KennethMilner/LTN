#include "file_reader.h"
#include <boost/filesystem/path.hpp>
#include <logger/logger.hpp>
#include <common/error.hpp>
#include "util.h"


int32_t file_reader::open()
{
    if (!bf::exists(m_os_charset_path))
    {
        LOG_DEBUG << "file reader NOT exists: " << m_os_charset_path;
        return ERR_FAILED;
    }

    uint64_t file_size = get_file_size();
    if (0 == file_size)         //how to do?
    {
        LOG_DEBUG << "file reader size is ZERO: " << m_os_charset_path;
        m_file_blocks_count = 0;
        return ERR_SUCCESS;
    }

    uint64_t blks_count = file_size / DEFAULT_FILE_BLOCK_SIZE;
    m_file_blocks_count = file_size % DEFAULT_FILE_BLOCK_SIZE ? ++blks_count : blks_count;

    //load file blocks
    //uint64_t load_blks_count = m_file_blocks_count > MIN_LOAD_BLOCKS_COUNT ? MIN_LOAD_BLOCKS_COUNT : m_file_blocks_count;

    std::ifstream ifs(m_os_charset_path, std::ios::in | std::ios::binary);
    m_ifs.swap(ifs);

    //LOG_DEBUG << "file reader begins to load file blocks: " << m_path;

    //sequence_load(MIN_FILE_BLOCK_NUM, load_blks_count);

    //LOG_DEBUG << "file reader ends to load file blocks: " << m_path;

    return ERR_SUCCESS;
}

int32_t file_reader::close()
{
    if (m_ifs.is_open())
    {
        m_ifs.close();
    }

    return ERR_SUCCESS;
}

uint64_t file_reader::get_file_size()
{
    if (false == m_size_calced)
    {
        try
        {
            m_file_size = file_util::get_file_size(m_os_charset_path);
            m_size_calced = true;
        }
        catch (...)
        {
            LOG_ERROR << "file reader get file size error: " << m_os_charset_path;
        }
    }

    return m_file_size;
}

unsigned int file_reader::get_file_hash()
{ 
    if (false == m_hash_calced)
    {
        m_crc_file_hash = file_util::calc_file_hash(m_os_charset_path);
        m_hash_calced = true;
    }

    return m_crc_file_hash; 
}

unsigned int file_reader::calc_file_hash()
{
    return file_util::calc_file_hash(m_os_charset_path);
}

void file_reader::notify_missing(uint64_t file_block_num)
{
    {
        std::unique_lock<std::mutex> lock(m_missing_mutex);
        m_missing_block_nums[file_block_num] = file_block_num;
    }
}

file_block_data_type file_reader::get(uint64_t file_block_num)
{
    if (file_block_num > m_file_blocks_count)
    {
        LOG_ERROR << "file reader get file block num error: " << std::to_string(file_block_num);
        return nullptr;
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        //file cache
        if (m_block_datas.end() != m_block_datas.find(file_block_num))
        {
            return m_block_datas[file_block_num];
        }
    }

    return nullptr;
}

int32_t file_reader::random_load(uint64_t file_block_num)
{
    if (file_block_num > m_file_blocks_count)
    {
        LOG_ERROR << "file reader random load file block num: " << std::to_string(file_block_num);
        return ERR_FAILED;
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_block_datas.end() != m_block_datas.find(file_block_num))
        {
            return ERR_SUCCESS;
        }
    }

    assert(file_block_num >= MIN_FILE_BLOCK_NUM);
    uint64_t start_pos = (file_block_num - 1) * DEFAULT_FILE_BLOCK_SIZE;
    m_ifs.seekg(start_pos);

    uint64_t load_size = DEFAULT_FILE_BLOCK_SIZE;

    //size of last block
    if (file_block_num == m_file_blocks_count)
    {
        uint64_t last_blk_size = get_file_size() % DEFAULT_FILE_BLOCK_SIZE;
        load_size = (0 == last_blk_size) ? DEFAULT_FILE_BLOCK_SIZE : last_blk_size;
    }

    if (load_size > 0)
    {
        auto buf = std::make_shared<file_data_buf>(load_size);
        m_ifs.read(buf->data(), buf->size());

        boost::crc_32_type file_block_crc_32;
        file_block_crc_32.process_bytes(&file_block_num, 8);
        file_block_crc_32.process_bytes(buf->data(), load_size);
        unsigned int file_block_hash = file_block_crc_32.checksum();
        buf->set_checksum(file_block_hash);

        assert(0 != file_block_hash);

        LOG_DEBUG << "file reader random load block: " << std::to_string(file_block_num) << " block  hash: " << std::to_string(file_block_hash);

        //compress
        buf->compress();

        file_block_data_type blk_data = std::make_shared<file_block_data>(file_block_num, buf);

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_block_datas[file_block_num] = blk_data;

            assert(0 != blk_data->get_checksum());
        }
    }

    return ERR_SUCCESS;
}

int32_t file_reader::sequence_load(uint64_t file_block_num, uint64_t blocks_count)
{
    if (file_block_num > m_file_blocks_count)
    {
        LOG_ERROR << "file reader sequence load file block num: " << std::to_string(file_block_num) << " larger than blocks count: " << std::to_string(blocks_count);
        return ERR_FAILED;
    }

    assert(file_block_num >= 1);
    uint64_t start_pos = (file_block_num - 1) * DEFAULT_FILE_BLOCK_SIZE;
    m_ifs.seekg(start_pos);

    uint64_t load_block_count = ((file_block_num + blocks_count) > m_file_blocks_count) ? (m_file_blocks_count -file_block_num  + 1) : blocks_count;

    for (uint64_t i = 0; i < load_block_count && !m_ifs.eof(); i++)
    {
        uint64_t cur_blk_num = file_block_num + i;

        uint64_t load_size = DEFAULT_FILE_BLOCK_SIZE;

        //size of last block
        if (cur_blk_num == m_file_blocks_count)
        {
            uint64_t last_blk_size = get_file_size() % DEFAULT_FILE_BLOCK_SIZE;
            load_size = (0 == last_blk_size) ? DEFAULT_FILE_BLOCK_SIZE : last_blk_size;
        }

        if (load_size > 0)
        {
            auto buf = std::make_shared<file_data_buf>(load_size);
            m_ifs.read(buf->data(), buf->size());

            //calc file buf checksum
            if (m_crc_calc_idx++ == cur_blk_num)
            {
                boost::crc_32_type file_block_crc_32;
                file_block_crc_32.process_bytes(&cur_blk_num, 8);
                file_block_crc_32.process_bytes(buf->data(), load_size);
                unsigned int file_block_hash = file_block_crc_32.checksum();
                buf->set_checksum(file_block_hash);

                assert(0 != file_block_hash);

                LOG_DEBUG << "file reader sequence load block: " << std::to_string(cur_blk_num) << " block  hash: " << std::to_string(file_block_hash);
            }

            //calc checksum
            m_crc_32.process_bytes(buf->data(), load_size);
            if (cur_blk_num == m_file_blocks_count)
            {
                m_crc_file_hash = m_crc_32.checksum();
                LOG_DEBUG << "file reader get file: " << m_os_charset_path <<  " checksum: " << std::to_string(m_crc_file_hash);
            }

            //compress
            buf->compress();

            file_block_data_type blk_data = std::make_shared<file_block_data>(cur_blk_num, buf);
            m_pre_load_block_datas.push_back(blk_data);

            if (0 == cur_blk_num % 1000)
            {
                LOG_DEBUG << "file reader sequence load block: " << std::to_string(cur_blk_num);
            }

            //update max num
            if (m_max_block_num_loaded < cur_blk_num)
            {
                m_max_block_num_loaded = cur_blk_num;
            }

            //update min num
            if (MIN_FILE_BLOCK_NUM == cur_blk_num)
            {
                m_min_block_num_loaded = MIN_FILE_BLOCK_NUM;
            }
            else if (cur_blk_num < m_min_block_num_loaded)
            {
                m_min_block_num_loaded = cur_blk_num;
            }
        }
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        for (auto blk_data : m_pre_load_block_datas)
        {
            m_block_datas[blk_data->m_file_block_num] = blk_data;
            assert(0 != blk_data->get_checksum());
        }

        m_pre_load_block_datas.clear();
    }

    if (false == m_ready && load_block_count)
    {
        m_ready = true;
        LOG_DEBUG << "file reader load file blocks READY";
    }

    //LOG_DEBUG << "file reader sequence load blocks count: " << std::to_string(load_block_count);

    return ERR_SUCCESS;
}

void file_reader::load_missing()
{
    std::map<uint64_t, uint64_t> missing_block_nums;

    {
        std::unique_lock<std::mutex> lock(m_missing_mutex);
        missing_block_nums.swap(m_missing_block_nums);
    }

    for (auto pair : missing_block_nums)
    {
        random_load(pair.first);
        LOG_DEBUG << "file read load missing block: " << std::to_string(pair.first);
    }
}

int32_t file_reader::get_block_start_position(uint64_t file_block_num, uint64_t &start_pos, uint64_t &size)
{
    uint64_t pos = (file_block_num - 1) * DEFAULT_FILE_BLOCK_SIZE;
    if (pos < get_file_size())
    {
        start_pos = pos;
        size = ((pos + DEFAULT_FILE_BLOCK_SIZE) >= get_file_size()) ? get_file_size() - start_pos : DEFAULT_FILE_BLOCK_SIZE;

        LOG_DEBUG << "file reader get block start position, block num" << std::to_string(file_block_num) << " start position: " << std::to_string(start_pos) << " size: " << std::to_string(size);
        return ERR_SUCCESS;
    }

    start_pos = 0;
    size = 0;

    LOG_DEBUG << "file reader get block start position error, block num" << std::to_string(file_block_num);
    return ERR_FAILED;
}

void file_reader::load_new_blocks()
{
    uint64_t unloaded = m_file_blocks_count > m_max_block_num_loaded ? m_file_blocks_count - m_max_block_num_loaded : 0;
    if (unloaded > 0)
    {
        uint64_t load_count = m_load_speed_per_second / (1000 / DEFAULT_SCHEDULING_TIMER_INTERVAL);
        load_count = load_count < MIN_LOAD_BLOCKS_COUNT ? MIN_LOAD_BLOCKS_COUNT : load_count;
        load_count = load_count > unloaded ? unloaded : load_count;
        //load_count = (0 == load_count) ? 1 : load_count;

        sequence_load(m_max_block_num_loaded + 1, load_count);
    }
}

int32_t file_reader::load_blocks_on_timer()
{
    //load missing
    load_missing();

    //has unloaded
    load_new_blocks();

    return ERR_SUCCESS;
}

void file_reader::notify_send_per_second(uint64_t send_speed_per_second)
{
    m_send_speed_per_second = send_speed_per_second;
    uint64_t load_speed_per_second = send_speed_per_second * AHEAD_OF_SCHEDULE_BATCH;
    m_load_speed_per_second = (load_speed_per_second < MIN_LOAD_BLOCKS_COUNT) ? MIN_LOAD_BLOCKS_COUNT : load_speed_per_second;

    //LOG_DEBUG << "file reader load speed per second: " << std::to_string(m_load_speed_per_second);
}

void file_reader::expire()
{
    //here needs optimization
    std::list<uint64_t> m_expires;
    for (auto it = m_block_datas.begin(); it != m_block_datas.end(); it++)
    {
        auto file_blk_data = it->second;
        if (file_blk_data->expired())
        {
            m_expires.push_back(file_blk_data->m_file_block_num);
        }
    }

    for (auto expired : m_expires)
    {
        m_block_datas.erase(expired);

        if (expired == m_min_block_num_loaded)
        {
            m_min_block_num_loaded++;
        }
    }

    m_expires.clear();
}


