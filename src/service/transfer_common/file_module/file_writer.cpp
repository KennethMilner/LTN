#include "file_writer.h"
#include <boost/filesystem/path.hpp>
#include <logger/logger.hpp>
#include <common/error.hpp>
#include "util.h"
#include<algorithm>


int32_t file_writer::open()
{
    if (!bf::exists(m_os_charset_path))
    {
        create_null_file();
    }

    std::ofstream ofs(m_os_charset_path, std::ios::out | std::ios::binary);
    m_ofs.swap(ofs);

    return ERR_SUCCESS;
}

int32_t file_writer::close()
{
    if (m_ofs.is_open())
    {
        m_ofs.close();
    }

    return ERR_SUCCESS;
}

int32_t file_writer::create_null_file()
{
    std::ofstream ofs;
    
    ofs.open(m_os_charset_path, std::ios::out | std::ios::binary);

    if (!ofs.is_open())
    {
        LOG_ERROR << "file writer open error: " << m_os_charset_path;
        return ERR_FAILED;
    }

    /*if (m_file_size)
    {
        ofs.seekp(m_file_size - 1, std::ios_base::beg);
        ofs.put(0);
    }*/

    ofs.close();
    return ERR_SUCCESS;
}

int32_t file_writer::write_blocks()
{

    for (auto block_data : m_data_list)
    {
        assert(0 != block_data->get_checksum());

        //uncompress
        block_data->m_file_block_data->uncompress();

        //calc file buf checksum
        boost::crc_32_type file_block_crc_32;
        file_block_crc_32.process_bytes(&block_data->m_file_block_num, 8);
        file_block_crc_32.process_bytes(block_data->data(), block_data->size());
        unsigned int file_block_hash = file_block_crc_32.checksum();
        assert(block_data->get_checksum() == file_block_hash);

        uint64_t offset = (block_data->m_file_block_num - 1) * DEFAULT_FILE_BLOCK_SIZE;
        m_ofs.seekp(offset, std::ios_base::beg);
        m_ofs.write(block_data->data(), block_data->size());

        LOG_DEBUG << "file writer write block: " << std::to_string(block_data->m_file_block_num) << " block hash: " << std::to_string(block_data->get_checksum()) << " block size: " << std::to_string(block_data->size());
    }

    m_data_list.clear();

    return ERR_SUCCESS;
}

int32_t file_writer::add(file_block_data_type block_data)
{
    auto it = m_data_bufs.find(block_data->m_file_block_num);
    if (m_data_bufs.end() != it)
    {
        return ERR_FAILED;
    }
    m_data_bufs[block_data->m_file_block_num] = block_data;

    //binary search
    //auto index = std::upper_bound(m_data_list.begin(), m_data_list.end(), block_data, file_block_data_comp);
    //m_data_list.insert(index, block_data);

    m_data_list.push_back(block_data);

    return ERR_SUCCESS;
}

int32_t file_writer::validate_checksum()
{
    uint64_t checksum = file_util::calc_file_hash(m_os_charset_path);
    if (checksum != m_file_hash)
    {
        LOG_ERROR << "file writer validate file checksum FAILED: " << m_os_charset_path << " file hash: " << std::to_string(m_file_hash) << " checksum: " << std::to_string(checksum);
        return ERR_FAILED;
    }
    else
    {
        LOG_INFO << "file writer validate file checksum SUCCESS: " << m_os_charset_path << " file hash: " << std::to_string(m_file_hash);
        return ERR_SUCCESS;
    }
}
