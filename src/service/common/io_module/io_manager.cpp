#include "io_manager.h"
#include <io/bootstrap.hpp>
#include "connector_initializer.h"
#include "handler_initializer.h"
#include "config_manager.h"
#include "service_message.h"
#include "util.h"
#include "macro.h"
#include "server.h"
#include "health_manager.h"

STATIC_INIT_LUA_MANAGER_INJECTION(io_manager, IO_MANAGER);

void io_manager::init_timer()
{
    INIT_TIMER(m_heartbeat_timer, "heartbeat", 10000, MAX_TRIGGER_TIMES, "", &io_manager::on_timer_heartbeat);
    INIT_TIMER(m_reconnect_timer, RECONNECT_TIMER, 30000, MAX_TRIGGER_TIMES, "", &io_manager::on_timer_reconnect);
}

void io_manager::init_invoker()
{
    INIT_INVOKER(CLICENT_CONNECT_NOTIFICATION, &io_manager::on_msg_client_connect_notification);
    INIT_INVOKER(CHANNEL_NOTIFICATION, &io_manager::on_msg_channel_notification);
    INIT_INVOKER(LOGIN_RSP, &io_manager::on_msg_login_rsp);
    INIT_INVOKER(HEARTBEAT_RSP, &io_manager::on_msg_heartbeat_rsp);
    INIT_INVOKER(DEV_REG_RSP, &io_manager::on_msg_dev_reg_rsp);
}

int32_t io_manager::service_init(any_map &vars)
{
    BOOTSTRAP_POOL(m_pool, POOL_THREADS_COUNT);

    BOOTSTRAP_CONNECTOR(connector, CONFIG_MANAGER.get_cloud_ip(), CONFIG_MANAGER.get_cloud_port(), m_pool, m_pool, connector_initializer, handler_initializer, handler_initializer);    
    assert(nullptr != connector);
    m_connectors[connector->channel_source().m_channel_id] = connector;

    return ERR_SUCCESS; 
}

int32_t io_manager::service_exit() 
{

    this->remove_timer(m_heartbeat_timer);
    this->remove_timer(m_reconnect_timer);

    m_pool->stop();
    m_pool->exit();

    for (auto it = m_connectors.begin(); it != m_connectors.end(); it++)
    {
        auto connector = it->second;
        assert(nullptr != connector);

        auto channel = connector->channel();
        if (channel)
        {
            channel->exit();
        }
    }
    m_connectors.clear();

    return ERR_SUCCESS;
}

int32_t io_manager::send_msg(msg_ptr_type msg)
{
    assert(nullptr != msg);

    LOG_DEBUG << "io manager send msg: " << msg->get_name();

    auto sid = msg->m_header->m_dst;
    assert(CLIENT_TYPE == sid.m_channel_type);

    auto it = m_connectors.find(sid.m_channel_id);
    if (m_connectors.end() == it)
    {
        LOG_ERROR << "io manager send message error: NOT find channel " << sid.to_string();
        return ERR_FAILED;
    }

    auto connector = it->second;
    assert(nullptr != connector);

    if (nullptr == connector->channel())
    {
        LOG_ERROR << "io manager send message error: channel is NULL" << sid.to_string();
        return ERR_FAILED;
    }

    if (CHANNEL_INACTIVE == connector->channel()->get_state())
    {
        LOG_ERROR << "io manager send message error: channel is INACTIVE" << sid.to_string();
        return ERR_FAILED;
    }

    return connector->channel()->write(msg);
}

int32_t io_manager::on_msg_login_rsp(msg_ptr_type msg)
{
    //response got
    assert(nullptr != msg && nullptr != msg->m_body);
    auto svc_rsp = std::dynamic_pointer_cast<service_message>(msg->m_body);

    auto net_msg = svc_rsp->m_net_msg;
    assert(nullptr != net_msg);    

    if (0 != net_msg->login_rsp_body().status())
    {
        LOG_ERROR << "io manager received log rsp status: " << net_msg->login_rsp_body().status();
    }

    return ERR_SUCCESS;
}

int32_t io_manager::on_msg_dev_reg_rsp(msg_ptr_type msg)
{
    assert(nullptr != msg && nullptr != msg->m_body);
    auto svc_rsp = std::dynamic_pointer_cast<service_message>(msg->m_body);

    auto net_msg = svc_rsp->m_net_msg;
    assert(nullptr != net_msg);

    if (0 != net_msg->dev_reg_rsp_body().status())
    {
        LOG_ERROR << "io manager received dev reg rsp status: " << net_msg->dev_reg_rsp_body().status();
        return ERR_FAILED;
    }
    else
    {
        //makr registered local
        CONFIG_MANAGER.set_registered();

        //login
        auto svc_msg = std::make_shared<service_message>();
        auto svc_login_head = svc_msg->m_net_msg->mutable_head();
        uint64_t timestamp = time_util::get_mill_seconds_from_19700101();
        svc_login_head->set_msg_name(LOGIN_REQ);
        svc_login_head->set_timestamp(timestamp);
        svc_login_head->set_magic(MAGIC_FLAG);
        svc_login_head->set_nonce(uuid_util::get_uuid());
        svc_login_head->set_session_id(uuid_util::get_uuid());
        svc_login_head->set_sign("");

        auto svc_login_body = svc_msg->m_net_msg->mutable_login_req_body();

        svc_login_body->set_node_id(CONFIG_MANAGER.get_id());
        svc_login_body->set_worker_id(CONFIG_MANAGER.get_id());
        svc_login_body->set_version(SOFTWARE_VERSION);

        auto login_msg = std::make_shared<message>();
        login_msg->m_header->m_name = LOGIN_REQ;
        login_msg->m_header->m_dst = msg->m_header->m_src;
        login_msg->m_body = svc_msg;

        return this->send_msg(login_msg);
    }
}

int32_t io_manager::on_msg_channel_notification(msg_ptr_type msg)
{
    assert(nullptr != msg && nullptr != msg->m_body);
    auto svc_msg = std::dynamic_pointer_cast<channel_notification>(msg->m_body);

    assert(nullptr != svc_msg);

    auto sid = msg->m_header->m_src;
    auto channel_status = svc_msg->m_status;
    if (channel_notification::BROKEN == channel_status)
    {
        LOG_ERROR << "io manager received channel notification: BROKEN" << msg->m_header->m_src.to_string();
        this->close(msg->m_header->m_src);
        //this->reconnect(msg->m_header->m_src);

        auto it = m_connectors.find(sid.m_channel_id);
        if (m_connectors.end() == it)
        {
            return ERR_FAILED;
        }

        m_reconnectors[sid.m_channel_id] = it->second;

    }
    else
    {
        //LOG
    }

    return ERR_SUCCESS;
}

int32_t io_manager::on_msg_client_connect_notification(msg_ptr_type msg)
{
    assert(nullptr != msg && nullptr != msg->m_body && nullptr != msg->m_header);
    auto svc_req = std::dynamic_pointer_cast<client_connect_notification>(msg->m_body);

    auto sid = msg->m_header->m_src;
    client_connect_notification::connect_status status = svc_req->m_status;

    if (client_connect_notification::SUCCESS == status && CONFIG_MANAGER.is_registered())
    {

        LOG_DEBUG << "io manager received client connector notification: SUCCESS and login: " << msg->m_header->m_src.to_string();

        m_reconnectors.erase(sid.m_channel_id);

        //login
        auto svc_msg = std::make_shared<service_message>();
        auto svc_login_head = svc_msg->m_net_msg->mutable_head();
        uint64_t timestamp = time_util::get_mill_seconds_from_19700101();
        svc_login_head->set_msg_name(LOGIN_REQ);
        svc_login_head->set_timestamp(timestamp);
        svc_login_head->set_magic(MAGIC_FLAG);
        svc_login_head->set_nonce(uuid_util::get_uuid());
        svc_login_head->set_session_id(uuid_util::get_uuid());
        svc_login_head->set_sign("");

        auto svc_login_body = svc_msg->m_net_msg->mutable_login_req_body();
        svc_login_body->set_node_id(CONFIG_MANAGER.get_id());
        svc_login_body->set_worker_id(CONFIG_MANAGER.get_id());
        svc_login_body->set_version(SOFTWARE_VERSION);

        auto login_msg = std::make_shared<message>();
        login_msg->m_header->m_name = LOGIN_REQ;
        login_msg->m_header->m_dst = msg->m_header->m_src;
        login_msg->m_body = svc_msg;

        return this->send_msg(login_msg);
    }
    else if (client_connect_notification::SUCCESS == status && !CONFIG_MANAGER.is_registered())
    {
        LOG_DEBUG << "io manager received client connector notification: SUCCESS and register: " << msg->m_header->m_src.to_string();

        m_reconnectors.erase(sid.m_channel_id);

        //register to cloud
        auto svc_msg = std::make_shared<service_message>();
        auto svc_login_head = svc_msg->m_net_msg->mutable_head();
        uint64_t timestamp = time_util::get_mill_seconds_from_19700101();
        svc_login_head->set_msg_name(DEV_REG_REQ);
        svc_login_head->set_timestamp(timestamp);
        svc_login_head->set_magic(MAGIC_FLAG);
        svc_login_head->set_nonce(uuid_util::get_uuid());
        svc_login_head->set_session_id(uuid_util::get_uuid());
        svc_login_head->set_sign("");

        auto svc_msg_body = svc_msg->m_net_msg->mutable_dev_reg_req_body();

        svc_msg_body->set_node_id(CONFIG_MANAGER.get_id());

#define MAP_STR_STR_PAIR(KEY, VALUE) google::protobuf::MapPair<std::string, std::string>(KEY, VALUE)

        ::MapStrValue cpu_info;
        google::protobuf::Map<std::string, std::string> &cpu_val = *(cpu_info.mutable_value());

        LOG_DEBUG << std::to_string(HEALTH_MANAGER.get_cpu_core_count());
        LOG_DEBUG << HEALTH_MANAGER.get_manufacture_id() + HEALTH_MANAGER.get_cpu_model();
        LOG_DEBUG << HEALTH_MANAGER.get_cpu_frequency();

        cpu_val["cpu_num"] = std::to_string(HEALTH_MANAGER.get_cpu_core_count());
        cpu_val["cpu_model"] = HEALTH_MANAGER.get_manufacture_id() + HEALTH_MANAGER.get_cpu_model();
        cpu_val["cpu_frequency"] = HEALTH_MANAGER.get_cpu_frequency();

        ::MapStrValue gpu_info;

        ::MapStrValue mem_info;
        google::protobuf::Map<std::string, std::string> &mem_val = *(mem_info.mutable_value());
        mem_val["total"] = HEALTH_MANAGER.get_total_mem_capacity();

        ::MapStrValue disk_info;
        google::protobuf::Map<std::string, std::string> &disk_val = *(disk_info.mutable_value());
        disk_val["total"] = std::to_string(HEALTH_MANAGER.get_total_disk_capacity());

        ::MapStrValue machine_info;
        google::protobuf::Map<std::string, std::string> &machine_val = *(machine_info.mutable_value());
        machine_val["uuid"] = HEALTH_MANAGER.get_uuid();
        machine_val["os"] = HEALTH_MANAGER.get_os();
        machine_val["name"] = HEALTH_MANAGER.get_host_name();

        ::MapStrValue network_info;
        google::protobuf::Map<std::string, std::string> &network_val = *(network_info.mutable_value());
        const std::list<network_card_info> &network_card_info_list = HEALTH_MANAGER.get_nic_infos();
        if (!network_card_info_list.empty())
        {
            network_val["nic_name"] = network_card_info_list.front().m_network_card_name;
            if (!network_card_info_list.front().m_ip_list.empty())
            {
                network_val["ip"] = network_card_info_list.front().m_ip_list.front();
            }
            network_val["mac"] = network_card_info_list.front().m_mac_addr;
        }

        google::protobuf::Map< std::string, ::MapStrValue > & dev_info = *(svc_msg_body->mutable_dev_info());
        dev_info["cpu_info"] = cpu_info;
        dev_info["gpu_info"] = gpu_info;
        dev_info["disk_info"] = disk_info;
        dev_info["mem_info"] = mem_info;
        dev_info["machine_info"] = machine_info;
        dev_info["network_info"] = network_info;

        auto req_msg = std::make_shared<message>();
        req_msg->m_header->m_name = DEV_REG_REQ;
        req_msg->m_header->m_dst = msg->m_header->m_src;
        req_msg->m_body = svc_msg;

        return this->send_msg(req_msg);
    }
    else
    {
        //reconnect
        LOG_ERROR << "io manager received client connector notification: CONNECT FAILED, " << msg->m_header->m_src.to_string();

        auto it = m_connectors.find(sid.m_channel_id);
        if (m_connectors.end() == it)
        {
            return ERR_FAILED;
        }

        m_reconnectors[sid.m_channel_id] = it->second;
        return ERR_SUCCESS;
    }
}

int32_t io_manager::on_timer_reconnect(timer_ptr_type timer)
{
    for (auto it = m_reconnectors.begin(); it != m_reconnectors.end(); it++)
    {
        LOG_DEBUG << "io manager reconnect" << it->second->channel()->channel_source().to_string();
        it->second->connect();
    }

    return ERR_SUCCESS;
}

int32_t io_manager::reconnect(channel_source sid)
{
    auto it = m_connectors.find(sid.m_channel_id);
    if (m_connectors.end() == it)
    {
        return ERR_FAILED;
    }

    return it->second->connect();
}

int32_t io_manager::close(channel_source sid)
{
    auto it = m_connectors.find(sid.m_channel_id);
    if (m_connectors.end() == it)
    {
        return ERR_FAILED;
    }

    return it->second->close();
}

int32_t io_manager::on_timer_heartbeat(timer_ptr_type timer)
{
    auto svc_msg = std::make_shared<service_message>();

    auto svc_login_head = svc_msg->m_net_msg->mutable_head();
    uint64_t timestamp = time_util::get_mill_seconds_from_19700101();
    svc_login_head->set_msg_name(HEARTBEAT_REQ);
    svc_login_head->set_timestamp(timestamp);
    svc_login_head->set_magic(MAGIC_FLAG);
    svc_login_head->set_nonce(uuid_util::get_uuid());
    svc_login_head->set_session_id(uuid_util::get_uuid());
    svc_login_head->set_sign("");

    auto heartbeat_msg = std::make_shared<message>();
    heartbeat_msg->m_header->m_name = HEARTBEAT_REQ;
    heartbeat_msg->m_body = svc_msg;

    for (auto it = m_connectors.begin(); it != m_connectors.end(); it++)
    {
        auto connector = it->second;
        assert(nullptr != connector);

        auto ch = connector->channel();
        assert(nullptr != ch);
        
        if (CHANNEL_ACTIVE == ch->get_state())
        {
            ch->write(heartbeat_msg);
        }
    }

    return ERR_SUCCESS;
}

int32_t io_manager::on_msg_heartbeat_rsp(msg_ptr_type msg)
{
    return ERR_SUCCESS;
}

