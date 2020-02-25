#pragma once

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include <boost/crc.hpp>
#include <codecvt>
#include <locale>

#include <string>
#include <sstream>
#include <iostream>

#include <key.h>
#include <base58.h>
#include <common/error.hpp>
#include <common/common.hpp>
#include <logger/logger.hpp>




namespace bf = boost::filesystem;
using namespace boost::archive::iterators;

typedef boost::asio::ip::udp::endpoint udp_endpoint;
typedef boost::asio::ip::tcp::endpoint tcp_endpoint;
typedef boost::asio::ip::address    ip_address;

#define UDP_ENDPOINT(IP, PORT)      udp_endpoint(ip_address::from_string(IP), PORT)
#define TCP_ENDPOINT(IP, PORT)      tcp_endpoint(ip_address::from_string(IP), PORT)


#ifdef WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(MAC_OSX)
#include <unistd.h>
#endif

using namespace boost::filesystem;

#define FILE_HASH_LOAD_SIZE                             1200
#define TRIM_STRING                                               " \t\n\r"

#define UTF8_TEXT(str) gbk_to_utf8(str)
#define OS_TEXT(str) utf8_to_gbk(str)

#define DO_NULL

#if defined(WIN32)
#define FILE_SEP            "\\"
#elif defined(__linux__)
#define FILE_SEP            "/"
#endif

#define DEFAULT_TO_DO_COUNT           100

#define BEGIN_COUNT_TO_DO(STATIC_COUNTER, COUNT) \
static int32_t STATIC_COUNTER = COUNT; \
if (--STATIC_COUNTER <= 0) \
{ \
    STATIC_COUNTER = COUNT;

#define DEFAULT_BEGIN_COUNT_TO_DO(STATIC_COUNTER) \
static int32_t STATIC_COUNTER = DEFAULT_TO_DO_COUNT; \
if (--STATIC_COUNTER <= 0) \
{ \
    STATIC_COUNTER = DEFAULT_TO_DO_COUNT;

#define END_COUNT_TO_DO \
}

extern std::unique_ptr<ECCVerifyHandle> g_ecc_verify_handle;
extern std::unique_ptr<std::recursive_mutex[]> g_ssl_lock;
extern void ssl_locking_callback(int mode, int type, const char *file, int line);

class path_util
{
public:

    static path get_cur_dir()
    {
        path dir;

#if defined(WIN32)
        char szFilePath[1024] = { 0 };
        if (GetModuleFileNameA(NULL, szFilePath, sizeof(szFilePath)))
        {
            dir = szFilePath;
            dir.remove_filename();
        }
        else
        {
            //LOG
        }

#elif defined(__linux__)
        char szFilePath[PATH_MAX + 1] = { 0 };
        int ret = readlink("/proc/self/exe", szFilePath, PATH_MAX);
        if (ret > 0 && ret < PATH_MAX)
        {
            dir = szFilePath;
            dir.remove_filename();
        }
#elif defined(MAC_OSX)
        char szFilePath[PROC_PIDPATHINFO_MAXSIZE] = { 0 };

        pid_t pid = getpid();
        int ret = proc_pidpath(pid, szFilePath, sizeof(szFilePath));
        if (ret > 0)
        {
            dir = szFilePath;
            dir.remove_filename();
        }
#endif
        return dir;
    }

    static void set_work_dir()
    {
#if defined(WIN32)
        SetCurrentDirectory(path_util::get_cur_dir().c_str());
#elif defined(__linux__)
        chdir(path_util::get_cur_dir());
#elif defined(MAC_OSX)
        chdir(path_util::get_cur_dir());
#endif
    }

    static std::string get_home_dir()
    {
        std::string path = "";

#if defined(WIN32)
        char homePath[1024] = { 0 };
        const char * homeProfile = "USERPROFILE";

        unsigned int pathSize = GetEnvironmentVariableA(homeProfile, homePath, 1024);
        if (pathSize == 0 || pathSize > 1024)
        {
            int ret = GetLastError();
        }
        else
        {
            path = std::string(homePath);
        }

        return path;
#elif defined(__linux__)
        
#elif defined(MAC_OSX)
        
#endif
    }

    static std::string get_file_name(const std::string & file_path)
    {
        bf::path fpath(file_path);
        return fpath.filename().string();
    }

};


class endian_util
{
public:

    enum endian_type
    {
        unknown = 0,
        big_endian,
        little_endian
    };

    static void init()
    {
        uint32_t a = 0x12345678;
        uint8_t  *p = (uint8_t *)(&a);
        m_type = (*p == 0x78) ? little_endian : big_endian;
    }

    static endian_type get()
    {
        return m_type;
    }

    static endian_type m_type;
};

class uuid_util
{
public:

    static inline std::string get_uuid()
    {
        boost::uuids::uuid uid = boost::uuids::random_generator()();
        return boost::uuids::to_string(uid);
    }
};

class crypto_util
{
public:

    static std::string encode_id(const CPubKey & pubkey) 
    {
        CKeyID keyID = pubkey.GetID();
        std::vector<unsigned char> id_data;
        std::vector<unsigned char> id_prefix = { 'i', 'd', '.' };
        id_data.reserve(id_prefix.size() + keyID.size());
        id_data.insert(id_data.end(), id_prefix.begin(), id_prefix.end());
        id_data.insert(id_data.end(), keyID.begin(), keyID.end());
        return EncodeBase58Check(id_data);
    }

    static int32_t generate_id(std::string &id, std::string &key)
    {
        CKey secret;

        bool fCompressed = true;
        secret.MakeNewKey(fCompressed);
        assert(secret.size() > 0);

        CPubKey pubkey = secret.GetPubKey();

        CPrivKey private_key = secret.GetPrivKey();
        id = encode_id(pubkey);

        std::vector<unsigned char> private_key_data;
        std::vector<unsigned char> private_key_prefix = {'k', 'e', 'y', '.' };
        private_key_data.reserve(private_key_prefix.size() + private_key.size());
        private_key_data.insert(private_key_data.end(), private_key_prefix.begin(), private_key_prefix.end());
        private_key_data.insert(private_key_data.end(), private_key.begin(), private_key.end());
        key = EncodeBase58Check(private_key_data);

        return ERR_SUCCESS;
    }

};


#ifdef WIN32
inline std::wstring gbk_to_unicode(const char* buf)
{
    int len = ::MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0);
    if (len == 0) return L"";

    std::wstring unicode;
    unicode.resize(len);
    ::MultiByteToWideChar(CP_ACP, 0, buf, -1, &unicode[0], len);

    return unicode;
}

inline std::string unicode_to_utf8(const wchar_t* buf)
{
    int len = ::WideCharToMultiByte(CP_UTF8, 0, buf, -1, NULL, 0, NULL, NULL);
    if (len == 0) return "";

    std::string utf8;
    utf8.resize(len);
    ::WideCharToMultiByte(CP_UTF8, 0, buf, -1, &utf8[0], len, NULL, NULL);

    return utf8;
}

inline std::string gbk_to_utf8(const char *src_str)
{
    return unicode_to_utf8(gbk_to_unicode(src_str).c_str());
}

inline std::string utf8_to_gbk(const char *src_str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);

    wchar_t* wszGBK = new wchar_t[len + 1];
    memset(wszGBK, 0, len * 2 + 2);
    MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
    len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);

    char* szGBK = new char[len + 1];
    memset(szGBK, 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);

    std::string strTemp(szGBK);
    if (wszGBK) delete[] wszGBK;
    if (szGBK) delete[] szGBK;

    return strTemp;
}

inline std::string gbk_to_utf8(const std::string & src_str)
{
    return gbk_to_utf8(src_str.c_str());
}

inline std::string utf8_to_gbk(const std::string & src_str)
{
    return utf8_to_gbk(src_str.c_str());
}

#else

//Android/iOS中不处理.
#define UTEXT(str) str

#endif

class string_util
{
public:

    template <typename OutputIterator>
    static bool generate_raw(OutputIterator sink, std::string s)
    {
        return boost::spirit::karma::generate(sink, '\"' << *("\\x" << boost::spirit::karma::hex) << '\"', s);
    }

    static std::string to_hex(const std::string &str)
    {
        std::string hex_str;
        if (!generate_raw(std::back_inserter(hex_str), str))
        {
            throw std::runtime_error("parse error");
        }

        return hex_str;
    }

    static std::string base64_encode(const std::string &input)
    {
        std::stringstream base64_str;
        typedef base64_from_binary<transform_width<std::string::const_iterator, 6, 8>> Base64EncodeIterator;

        try
        {
            std::copy(Base64EncodeIterator(input.begin()), Base64EncodeIterator(input.end()), std::ostream_iterator<char>(base64_str));
        }
        catch (...)
        {
            base64_str.clear();
            LOG_ERROR << "task manager base64 encode exception: " << input;

            return base64_str.str();
        }

        size_t equal_count = (3 - input.length() % 3) % 3;
        for (size_t i = 0; i < equal_count; i++)
        {
            base64_str.put('=');
        }

        return base64_str.str();
    }

    static std::string utf16_to_utf8(const std::u16string &input)
    {
        return std::wstring_convert< std::codecvt_utf8_utf16<char16_t>, char16_t >{}.to_bytes(input);
    }

    static std::u16string utf8_to_utf16(const std::string &input)
    {
        return std::wstring_convert< std::codecvt_utf8_utf16<char16_t>, char16_t >{}.from_bytes(input);
    }

    // trim from end of string (right)
    static inline std::string& rtrim(std::string& s, const char* t = TRIM_STRING)
    {
        s.erase(s.find_last_not_of(t) + 1);
        return s;
    }

    // trim from beginning of string (left)
    static inline std::string& ltrim(std::string& s, const char* t = TRIM_STRING)
    {
        s.erase(0, s.find_first_not_of(t));
        return s;
    }

    // trim from both ends of string (right then left)
    static inline std::string& trim(std::string& s, const char* t = TRIM_STRING)
    {
        return ltrim(rtrim(s, t), t);
    }

    template <class T>
    static inline void from_string(T &value, const std::string &s) 
    {
        std::stringstream ss(s);
        ss >> value;
    }

    template <class T>
    static inline std::string to_string(T value) 
    {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

};

#define GENERIC_PATH(PATH)      UTF8_TEXT(bf::path(OS_TEXT(PATH)).generic_string())

class file_util
{
public:

    static unsigned int calc_file_hash(const std::string & file_path)
    {
        if (!bf::exists(file_path))
        {
            LOG_ERROR << "calc file hash BUT file NOT exists: " << file_path;
            return 0;
        }

        uint64_t file_size = get_file_size(file_path);
        if (0 == file_size)
        {
            return 0;
        }

        boost::crc_32_type crc_32;
        uint64_t left_size = file_size;

        char cache[FILE_HASH_LOAD_SIZE];

        std::ifstream ifs(file_path, std::ios::in | std::ios::binary);
        while (!ifs.eof() && left_size > 0)
        {
            memset(cache, 0x00, sizeof(cache));

            uint64_t load_size = left_size > FILE_HASH_LOAD_SIZE ? FILE_HASH_LOAD_SIZE : left_size;
            ifs.read(cache, load_size);
            left_size -= load_size;

            crc_32.process_bytes(cache, load_size);
        }
        ifs.close();

        return crc_32.checksum();
    }

    static uint64_t get_file_size(const std::string & file_path)
    {
        return boost::filesystem::file_size(file_path);
    }

    static std::string get_last_dir_name(const std::string & dir_path)
    {
        std::vector<std::string> vec;
        boost::split(vec, dir_path, boost::is_any_of("/\\"));

        std::string dir_name;

        if (vec.size() > 0)
        {
            dir_name = vec[vec.size() - 1];
        }

        return dir_name;
    }

    static std::string remove_prefix_path(const std::string & begin_dir, const std::string & dir_path)
    {
        std::string son_dir;
        auto string_dir_path = bf::path(dir_path).string();

        //remove prefix
        if (string_dir_path.length() > begin_dir.length() && string_dir_path.substr(0, begin_dir.length()) == begin_dir)
        {
            son_dir = string_dir_path.substr(begin_dir.length(), string_dir_path.length() - begin_dir.length());
        }

        return son_dir;
    }

    static std::string remove_prefix_path_and_file_name(const std::string & begin_dir, const std::string & file_path)
    {
        std::string son_dir;

        //get parent dir
        bf::path total_path(file_path);
        auto parent_path = total_path.parent_path().string();

        //remove prefix
        if (parent_path.length() > begin_dir.length() && parent_path.substr(0, begin_dir.length()) == begin_dir)
        {
            son_dir = parent_path.substr(begin_dir.length(), parent_path.length() - begin_dir.length());
        }

        return son_dir;
    }
};

class endpoint_util
{
public:

    static std::string to_string(udp_endpoint ep)
    {
        return ep.address().to_string() + std::to_string(ep.port());
    }

    static std::string to_string(tcp_endpoint ep)
    {
        return ep.address().to_string() + " " + std::to_string(ep.port());
    }

};

class url_util
{
public:

    static unsigned char to_hex(unsigned char x)
    {
        return  x > 9 ? x + 55 : x + 48;
    }

    static unsigned char from_hex(unsigned char x)
    {
        unsigned char y;
        if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
        else if (x >= '0' && x <= '9') y = x - '0';
        else assert(0);
        return y;
    }

    static std::string url_encode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (isalnum((unsigned char)str[i]) ||
                (str[i] == '-') ||
                (str[i] == '_') ||
                (str[i] == '.') ||
                (str[i] == '~'))
                strTemp += str[i];
            else if (str[i] == ' ')
                strTemp += "+";
            else
            {
                strTemp += '%';
                strTemp += to_hex((unsigned char)str[i] >> 4);
                strTemp += to_hex((unsigned char)str[i] % 16);
            }
        }
        return strTemp;
    }

    static std::string url_decode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (str[i] == '+') strTemp += ' ';
            else if (str[i] == '%')
            {
                assert(i + 2 < length);
                unsigned char high = from_hex((unsigned char)str[++i]);
                unsigned char low = from_hex((unsigned char)str[++i]);
                strTemp += high * 16 + low;
            }
            else strTemp += str[i];
        }
        return strTemp;
    }

};

class network_card_info
{
public:

    std::string m_network_card_name;

    std::string m_mac_addr;

    std::list<std::string> m_ip_list;
};

class sys_util
{
public:

    static std::string get_host_name();

    static std::string get_os_info();

#ifdef WIN32
    static void init_cpu(DWORD veax);
#endif

    static uint32_t get_cpu_count();

    static uint32_t get_cpu_core_count();

    static std::string get_cpu_freq();

    static std::string get_manufacture_id();

    static std::string get_cpu_model();

    static std::string get_memory_info();

    static std::string get_hard_disk_info();

    static uint64_t get_total_disk();

    static int32_t get_network_card_info(std::list<network_card_info> &network_card_infos);

    static int get_process_info();

    static std::string get_mainboard_uuid();

protected:

    static std::string exec_cmd(const char *cmd);

};

#ifdef WIN32
extern "C" DWORD __stdcall get_cur_cycles();
//extern "C" void __stdcall init_cpu(DWORD veax, DWORD *deax, DWORD *debx, DWORD *decx, DWORD *dedx);
#endif
