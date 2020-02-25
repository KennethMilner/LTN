#include "util.h"
#include <openssl/crypto.h>

#ifdef WIN32
#include <tchar.h>
#include <winsock2.h> // include must before window.h
#include <iphlpapi.h>
#include <windows.h> 
#include <intrin.h>

#pragma comment(lib, "iphlpapi.lib")
#elif defined(__linux__)

#endif

#pragma warning(disable: 4996)


static const int kMaxInfoBuffer = 256;
#define  GBYTES  1073741824  
#define  MBYTES  1048576  
#define  KBYTES  1024  
#define  DKBYTES 1024.0

std::unique_ptr<ECCVerifyHandle> g_ecc_verify_handle;
std::unique_ptr<std::recursive_mutex[]> g_ssl_lock;

void ssl_locking_callback(int mode, int type, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
    {
        g_ssl_lock[type].lock();
    }
    else
    {
        g_ssl_lock[type].unlock();
    }
}


#ifdef WIN32


std::string sys_util::get_host_name()
{
    char buf[MAX_COMPUTERNAME_LENGTH + 2];
    DWORD buf_size = sizeof buf - 1;
    memset(buf, 0x00, sizeof(buf));

    GetComputerNameA(buf, &buf_size);
    std::string host_name = buf;
    return host_name;
}

std::string sys_util::get_os_info()
{
    // get os name according to version number
    OSVERSIONINFO osver = { sizeof(OSVERSIONINFO) };
    GetVersionEx(&osver);

    std::string os_name;
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0)
        os_name = "Windows 2000";
    else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1)
        os_name = "Windows XP";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0)
        os_name = "Windows 2003";
    else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2)
        os_name = "windows vista";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1)
        os_name = "windows 7";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2)
        os_name = "windows 10";

    return os_name;

    //std::cout << "os name: " << os_name << std::endl;
    //std::cout << "os version: " << osver.dwMajorVersion << '.' << osver.dwMinorVersion << std::endl;
}

// ---- get cpu info ---- //

//#ifdef _WIN64
//
//// method 2: usde winapi, works for x86 and x64
//#include <intrin.h>
//void sys_util::get_cpu_info()
//{
//    int cpuInfo[4] = { -1 };
//    char cpu_manufacture[32] = { 0 };
//    char cpu_type[32] = { 0 };
//    char cpu_freq[32] = { 0 };
//
//    __cpuid(cpuInfo, 0x80000002);
//    memcpy(cpu_manufacture, cpuInfo, sizeof(cpuInfo));
//
//    __cpuid(cpuInfo, 0x80000003);
//    memcpy(cpu_type, cpuInfo, sizeof(cpuInfo));
//
//    __cpuid(cpuInfo, 0x80000004);
//    memcpy(cpu_freq, cpuInfo, sizeof(cpuInfo));
//
//    std::cout << "CPU manufacture: " << cpu_manufacture << std::endl;
//    std::cout << "CPU type: " << cpu_type << std::endl;
//    std::cout << "CPU main frequency: " << cpu_freq << std::endl;
//}
//
//#else



uint32_t sys_util::get_cpu_count()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

uint32_t sys_util::get_cpu_core_count()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

// mothed 1: this kind asm embedded in code only works in x86 build
// save 4 register variables
//DWORD deax;
//DWORD debx;
//DWORD decx;
//DWORD dedx;

// init cpu in assembly language
void sys_util::init_cpu(DWORD veax)
{
    /*__asm
    {
        mov eax, veax
        cpuid
        mov deax, eax
        mov debx, ebx
        mov decx, ecx
        mov dedx, edx
    }*/
}

std::string sys_util::get_cpu_freq()
{
    //DWORD start = 0, over = 0;

    //_asm
    //{
    //    RDTSC
    //    mov start, eax
    //}

    //Sleep(50);

    //_asm
    //{
    //    RDTSC
    //    mov over, eax
    //}

    DWORD start = 0, over = 0;
    start = get_cur_cycles();
    Sleep(1000);
    over = get_cur_cycles();

    double freq = (over - start) / 1000000000.0;
    return string_util::to_string(freq);

    /*int cpuInfo[4] = { -1 };
    char cpu_freq[32] = { 0 };

    __cpuid(cpuInfo, 0x80000004);
    memcpy(cpu_freq, cpuInfo, sizeof(cpuInfo));

    std::string output = cpu_freq;
    string_util::trim(output, " GHz");

    return output;*/
}

std::string sys_util::get_manufacture_id()
{
    //char manuID[25];
    //memset(manuID, 0, sizeof(manuID));

    //init_cpu(0);

    //memcpy(manuID + 0, &debx, 4);
    //memcpy(manuID + 4, &dedx, 4);
    //memcpy(manuID + 8, &decx, 4);

    //return manuID;

    int cpuInfo[4] = { -1 };
    char cpu_manufacture[32] = { 0 };

    __cpuid(cpuInfo, 0x80000002);
    memcpy(cpu_manufacture, cpuInfo, sizeof(cpuInfo));

    return cpu_manufacture;
}

std::string sys_util::get_cpu_model()
{
    //const DWORD id = 0x80000002; // start 0x80000002 end to 0x80000004

    //char cpuType[49];
    //memset(cpuType, 0, sizeof(cpuType));

    //for (DWORD t = 0; t < 3; t++)
    //{
    //    init_cpu(id + t);

    //    memcpy(cpuType + 16 * t + 0, &deax, 4);
    //    memcpy(cpuType + 16 * t + 4, &debx, 4);
    //    memcpy(cpuType + 16 * t + 8, &decx, 4);
    //    memcpy(cpuType + 16 * t + 12, &dedx, 4);
    //}

    //return cpuType;

    int cpuInfo[4] = { -1 };
    char cpu_type[32] = { 0 };

    __cpuid(cpuInfo, 0x80000003);
    memcpy(cpu_type, cpuInfo, sizeof(cpuInfo));

    return cpu_type;
}

//#endif

// ---- get memory info ---- //
std::string sys_util::get_memory_info()
{
    std::string memory_info;
    
    MEMORYSTATUSEX statusex;
    statusex.dwLength = sizeof(statusex);

    if (GlobalMemoryStatusEx(&statusex))
    {
        unsigned long long total = 0, remain_total = 0, avl = 0, remain_avl = 0;
        double decimal_total = 0, decimal_avl = 0;

        remain_total = statusex.ullTotalPhys % GBYTES;
        total = statusex.ullTotalPhys / GBYTES;
        avl = statusex.ullAvailPhys / GBYTES;
        remain_avl = statusex.ullAvailPhys % GBYTES;

        if (remain_total > 0)
            decimal_total = (remain_total / MBYTES) / DKBYTES;

        if (remain_avl > 0)
            decimal_avl = (remain_avl / MBYTES) / DKBYTES;

        decimal_total += (double)total;
        decimal_avl += (double)avl;
        char  buffer[kMaxInfoBuffer];
        //sprintf_s(buffer, kMaxInfoBuffer, "total %.2f GB (%.2f GB available)", decimal_total, decimal_avl);
        sprintf_s(buffer, kMaxInfoBuffer, "%lld", statusex.ullTotalPhys / MBYTES);
        memory_info.append(buffer);
    }

    //std::cout << memory_info << std::endl;
    return memory_info;
}

// ---- get harddisk info ---- //
std::string sys_util::exec_cmd(const char *cmd)
{
    char buffer[128] = { 0 };
    std::string result;
    FILE *pipe = _popen(cmd, "r");
    if (!pipe) throw std::runtime_error("_popen() failed!");
    
    while (!feof(pipe))
    {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    _pclose(pipe);

    return result;
}

std::string sys_util::get_hard_disk_info()
{
    std::string hd_serial = exec_cmd("wmic path win32_physicalmedia get SerialNumber");
    //std::cout << "HardDisk Serial Number: " << hd_seiral << std::endl;

    return hd_serial;
}

uint64_t sys_util::get_total_disk()
{
    uint64_t all_Total = 0;
    uint64_t all_Free = 0;
    DWORD dwSize = MAX_PATH;
    TCHAR szLogicalDrives[MAX_PATH] = { 0 };

    DWORD dwResult = GetLogicalDriveStrings(dwSize, szLogicalDrives);
    if (dwResult > 0 && dwResult <= MAX_PATH)
    {
        TCHAR* szSingleDrive = szLogicalDrives;

        while (*szSingleDrive)
        {
            uint64_t available, total, free;
            if (GetDiskFreeSpaceEx(szSingleDrive, (ULARGE_INTEGER*)&available, (ULARGE_INTEGER*)&total, (ULARGE_INTEGER*)&free))
            {
                uint64_t Total, Available, Free;
                Total = total >> 20;
                Available = available >> 20;
                Free = free >> 20;

                all_Total += Total;
                all_Free += Free;
            }

            szSingleDrive += _tcslen(szSingleDrive) + 1;
        }
    }

    return all_Total / 1000;
}

int32_t sys_util::get_network_card_info(std::list<network_card_info> &network_card_infos)
{
    // PIP_ADAPTER_INFO struct contains network information
    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
    unsigned long adapter_size = sizeof(IP_ADAPTER_INFO);
    int ret = GetAdaptersInfo(pIpAdapterInfo, &adapter_size);

    if (ret == ERROR_BUFFER_OVERFLOW)
    {
        // overflow, use the output size to recreate the handler
        delete pIpAdapterInfo;
        pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[adapter_size];
        ret = GetAdaptersInfo(pIpAdapterInfo, &adapter_size);
    }

    if (ret == ERROR_SUCCESS)
    {
        int card_index = 0;

        // may have many cards, it saved in linklist
        while (pIpAdapterInfo)
        {
            network_card_info nic_info;

            nic_info.m_network_card_name = pIpAdapterInfo->AdapterName;

            //std::cout << "---- " << "NetworkCard " << ++card_index << " ----" << std::endl;

            //std::cout << "Network Card Name: " << pIpAdapterInfo->AdapterName << std::endl;
            //std::cout << "Network Card Description: " << pIpAdapterInfo->Description << std::endl;

            // get IP, one card may have many IPs
            PIP_ADDR_STRING pIpAddr = &(pIpAdapterInfo->IpAddressList);
            while (pIpAddr)
            {
                char local_ip[128] = { 0 };
                memset(local_ip, 0x00, sizeof(local_ip));
                strncpy(local_ip, pIpAddr->IpAddress.String, sizeof(pIpAddr->IpAddress.String) - 1);

                nic_info.m_ip_list.push_back(local_ip);
                //std::cout << "Local IP: " << local_ip << std::endl;

                pIpAddr = pIpAddr->Next;
            }

            char local_mac[128] = { 0 };
            memset(local_mac, 0x00, sizeof(local_mac) - 1);
            int char_index = 0;
            for (unsigned int i = 0; i < pIpAdapterInfo->AddressLength; i++)
            {
                char temp_str[10] = { 0 };
                sprintf_s(temp_str, "%02X-", pIpAdapterInfo->Address[i]); // X for uppercase, x for lowercase
                strncpy(local_mac + char_index, temp_str, sizeof(temp_str) - 1);
                char_index += 3;
            }
            local_mac[17] = '\0'; // remove tail '-'

            nic_info.m_mac_addr = local_mac;
            //std::cout << "Local Mac: " << local_mac << std::endl;

            // here just need the first card info

            //break;

            network_card_infos.push_back(nic_info);

            // iterate next
            pIpAdapterInfo = pIpAdapterInfo->Next;
        }
    }

    if (pIpAdapterInfo)
        delete pIpAdapterInfo;

    return ERR_SUCCESS;
}

int sys_util::get_process_info()
{
    int pid = GetCurrentProcessId();
    // TODO: cpu and mem usage
    //printf("Current Pid: %d\n", pid);

    return pid;
}

std::string sys_util::get_mainboard_uuid()
{
    std::string cmd_output = sys_util::exec_cmd("wmic csproduct get UUID");
    size_t pos = cmd_output.find("\r\n");
    if (std::string::npos != pos)
    {
        cmd_output = cmd_output.substr(pos);
        string_util::trim(cmd_output);
    }

    return cmd_output;
}

#elif defined(__linux__)

// ---- get os info ---- //
std::string sys_util::get_os_info()
{
    FILE *fp = fopen("/proc/version", "r");
    if (NULL == fp)
        printf("failed to open version\n");
    char szTest[1000] = { 0 };
    while (!feof(fp))
    {
        memset(szTest, 0, sizeof(szTest));
        fgets(szTest, sizeof(szTest) - 1, fp); // leave out \n
        printf("%s", szTest);
    }
    fclose(fp);

    return szTest;
}

// ---- get cpu info ---- //
std::string sys_util::get_cpu_info()
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (NULL == fp)
        printf("failed to open cpuinfo\n");
    char szTest[1000] = { 0 };
    // read file line by line
    while (!feof(fp))
    {
        memset(szTest, 0, sizeof(szTest));
        fgets(szTest, sizeof(szTest) - 1, fp); // leave out \n
        printf("%s", szTest);
    }
    fclose(fp);

    return szTest;
}


// ---- get memory info ---- //
std::string sys_util::get_memory_info()
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (NULL == fp)
        printf("failed to open meminfo\n");
    char szTest[1000] = { 0 };
    while (!feof(fp))
    {
        memset(szTest, 0, sizeof(szTest));
        fgets(szTest, sizeof(szTest) - 1, fp);
        printf("%s", szTest);
    }
    fclose(fp);

    return szTest;
}

// ---- get harddisk info ---- //
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hdreg.h>

std::string sys_util::get_hard_disk_info()
{
    // use cmd, this is the only way
    static struct hd_driveid hd;
    int fd;

    if ((fd = open("/dev/sda", O_RDONLY | O_NONBLOCK)) < 0)
    {
        printf("ERROR opening /dev/sda\n");
        return;
    }

    if (!ioctl(fd, HDIO_GET_IDENTITY, &hd))
    {
        printf("model ", hd.model);
        printf("HardDisk Serial Number: %.20s\n", hd.serial_no);
    }
    else
        printf("no available harddisk info");

    return hd.serial_no;
}

// ---- get network info ---- //
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/if.h>


std::string sys_util::get_network_info()
{
    // get hostname and external ip, simple and best way
    char host_name[256] = { 0 };
    gethostname(host_name, sizeof(host_name));

    struct hostent* host = gethostbyname(host_name);
    char ip_str[32] = { 0 };
    const char* ret = inet_ntop(host->h_addrtype, host->h_addr_list[0], ip_str, sizeof(ip_str));
    if (ret != NULL)
    {
        printf("Current HostName: %s\n", host_name);
        printf("Current external IP: %s\n", ip_str);
    }

    // get multiple ip, works for linux
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;

        // check if IP4
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            void *tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char local_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, local_ip, INET_ADDRSTRLEN);

            // actually only need external ip
            printf("%s IP: %s\n", ifa->ifa_name, local_ip);
        }
    }

    if (ifAddrStruct)
        freeifaddrs(ifAddrStruct);


    // get mac, need to create socket first, may not work for mac os
    struct ifreq ifr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    char local_mac[128] = { 0 };

    strcpy(ifr.ifr_name, "eth0"); // only need ethernet card
    if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr))
    {
        char temp_str[10] = { 0 };
        memcpy(temp_str, ifr.ifr_hwaddr.sa_data, 6);
        sprintf(local_mac, "%02x-%02x-%02x-%02x-%02x-%02x", temp_str[0] & 0xff, temp_str[1] & 0xff, temp_str[2] & 0xff, temp_str[3] & 0xff, temp_str[4] & 0xff, temp_str[5] & 0xff);
    }

    printf("Local Mac: %s\n", local_mac);

}

#endif