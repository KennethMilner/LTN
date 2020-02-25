#pragma once

#include <common/error.hpp>
#include <common/common.hpp>


#ifdef __EXPORT
#undef __EXPORT
#endif


#define SOFTWARE_VERSION            "1.4.4"
#define DEFAULT_IP                              "127.0.0.1"
#define MESSAGE_IN_SESSION          "msg"
#define ERR_CMD_EXIT                        -2

#define MAGIC_MAIN                            0x00000000
#define MAGIC_TEST                             0XFFFFFFFF
#define MAGIC_FLAG                             MAGIC_MAIN

#define INIT_LUA_MANAGER_INJECTION(MANAGER_NAME) \
static int init_##MANAGER_NAME(lua_State* L) \
{ \
    LOG_DEBUG << "init " << #MANAGER_NAME; \
    if (!lua_isuserdata(L, -1)) \
    { \
        return ERR_FAILED; \
    } \
    void *usr_data = lua_touserdata(L, -1); \
    luaL_argcheck(L, usr_data != NULL, 1, "`vars pointer expected"); \
    any_map &vars = *(any_map *)usr_data; \
    int32_t rt = ERR_SUCCESS; \
    std::shared_ptr<module> mdl = nullptr; \
    mdl = DYN_CAST(module, std::make_shared<MANAGER_NAME>()); \
    rt = mdl->init(vars); \
    if (ERR_SUCCESS != rt) \
    { \
        return rt; \
    } \
    rt = mdl->start(); \
    if (ERR_SUCCESS != rt) \
    { \
        return rt; \
    } \
    THE_SERVER[mdl->name()] = mdl; \
    return ERR_SUCCESS; \
} \
static int exit_##MANAGER_NAME(lua_State* L) \
{ \
    return ERR_SUCCESS; \
} \
static luaL_Reg libs_##MANAGER_NAME[] = \
{ \
    {"init", init_##MANAGER_NAME}, \
    {"exit", exit_##MANAGER_NAME }, \
    {NULL,NULL} \
}; \
int32_t luaopen_##MANAGER_NAME(lua_State *L) \
{ \
    luaL_newlib(L, libs_##MANAGER_NAME); \
    return 1; \
}

#define STATIC_INIT_LUA_MANAGER_INJECTION(MANAGER_NAME, MANAGER_SINGLETON) \
static int init_##MANAGER_NAME(lua_State* L) \
{ \
    LOG_DEBUG << "init " << #MANAGER_NAME; \
    if (!lua_isuserdata(L, -1)) \
    { \
        return ERR_FAILED; \
    } \
    void *usr_data = lua_touserdata(L, -1); \
    luaL_argcheck(L, usr_data != NULL, 1, "`vars pointer expected"); \
    any_map &vars = *(any_map *)usr_data; \
    int32_t rt = ERR_SUCCESS; \
    rt = MANAGER_SINGLETON.init(vars); \
    if (ERR_SUCCESS != rt) \
    { \
        return rt; \
    } \
    rt = MANAGER_SINGLETON.start(); \
    if (ERR_SUCCESS != rt) \
    { \
        return rt; \
    } \
    return ERR_SUCCESS; \
} \
static int exit_##MANAGER_NAME(lua_State* L) \
{ \
    return ERR_SUCCESS; \
} \
static luaL_Reg libs_##MANAGER_NAME[] = \
{ \
    {"init", init_##MANAGER_NAME}, \
    {"exit", exit_##MANAGER_NAME }, \
    {NULL,NULL} \
}; \
\
int32_t luaopen_##MANAGER_NAME(lua_State *L) \
{ \
    luaL_newlib(L, libs_##MANAGER_NAME); \
    return 1; \
}

#define DECLARE_SVC_REQ(MSG_NAME) \
auto svc_req = std::make_shared<trans_service_message>(); \
auto svc_req_head = svc_req->m_trans_msg->mutable_head(); \
uint64_t timestamp = time_util::get_mill_seconds_from_19700101(); \
svc_req_head->set_msg_name(MSG_NAME); \
svc_req_head->set_timestamp(timestamp); \
svc_req_head->set_magic(MAGIC_FLAG); \
std::string uuid = uuid_util::get_uuid(); \
svc_req_head->set_nonce(uuid); \
svc_req_head->set_session_id(uuid); \
svc_req_head->set_sign("");

#define DECLARE_SVC_RSP(MSG_NAME) \
auto svc_rsp = std::make_shared<trans_service_message>(); \
auto svc_rsp_head = svc_rsp->m_trans_msg->mutable_head(); \
uint64_t timestamp = time_util::get_mill_seconds_from_19700101(); \
svc_rsp_head->set_msg_name(MSG_NAME); \
svc_rsp_head->set_timestamp(timestamp); \
svc_rsp_head->set_magic(MAGIC_FLAG); \
std::string uuid = uuid_util::get_uuid(); \
svc_rsp_head->set_nonce(uuid); \
svc_rsp_head->set_session_id(svc_req_head.session_id()); \
svc_rsp_head->set_sign("");

#define GET_MSG_FROM_TIMER(MSG) \
assert(nullptr != timer); \
auto session_id = timer->get_info();\
auto it = m_sessions.find(session_id);\
if (m_sessions.end() == it)\
{\
    return ERR_SUCCESS;\
}\
auto ss = it->second;\
auto MSG = boost::any_cast<std::shared_ptr<message>>(ss->m_vars.get(MESSAGE_IN_SESSION));\
assert(nullptr != MSG);