/*  Copyright (c) 2013-2015 Tencent. All rights reserved.  */
/*
 ============================================================================
 Name       : net_comm.h
 Author     : yerungui
 Created on : 2012-7-18
 ============================================================================
 */

#ifndef NETWORK_SRC_NET_COMM_H_
#define NETWORK_SRC_NET_COMM_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "../comm/autobuffer.h"

namespace mars{
    namespace stn{


struct Task {
public:
    //channel type
    static const int kChannelShort = 0x1;
    static const int kChannelLong = 0x2;
    static const int kChannelBoth = 0x3;
    
    static const int kChannelNormalStrategy = 0;
    static const int kChannelFastStrategy = 1;
    
    static const int kTaskPriorityHighest = 0;
    static const int kTaskPriority0 = 0;
    static const int kTaskPriority1 = 1;
    static const int kTaskPriority2 = 2;
    static const int kTaskPriority3 = 3;
    static const int kTaskPriorityNormal = 3;
    static const int kTaskPriority4 = 4;
    static const int kTaskPriority5 = 5;
    static const int kTaskPriorityLowest = 5;
    
    static const uint32_t kInvalidTaskID = 0;
    static const uint32_t kNoopTaskID = 0xFFFFFFFF;
    static const uint32_t kLongLinkIdentifyCheckerTaskID = 0xFFFFFFFE;
    static const uint32_t kSignallingKeeperTaskID = 0xFFFFFFFD;
    
    
    Task();

    //require
    uint32_t taskid;
    int32_t channel_select;
    uint32_t cmdid;
    std::string cgi;    // user
    std::string host;   // host, user

    //optional
    bool send_only;  // user
    bool need_authed;  // user
    bool limit_flow;  // user
    bool limit_frequency;  // user
    
    int32_t channel_strategy;
    bool network_status_sensitive;  // user
    int32_t priority;  // user
    
    int32_t retry_count;  // user
    int32_t server_process_cost;  // user
    int32_t total_timetout;  // user ms
    
    void* user_context;  // user
    
   // std::string report_arg;  // user for cgi report
};

enum TaskFailHandleType {
	kTaskFailHandleNormal = 0,
	kTaskFailHandleDefault = -1,
	kTaskFailHandleSessionTimeout = -13,
	kTaskFailHandleTaskEnd = -14,
};
        
//error type
enum ErrCmdType {
	kEctOK = 0,
	kEctFalse = 1,
	kEctDial = 2,
	kEctDns = 3,
	kEctSocket = 4,
	kEctHttp = 5,
	kEctNetMsgXP = 6,
	kEctEnDecode = 7,
	kEctServer = 8,
	kEctLocal = 9,
};

//error code
enum {
	kEctLocalTaskTimeout = -1,
    kEctLocalTaskRetry = -2,
	kEctLocalStartTaskFail = -3,
	kEctLocalAntiAvalanche = -4,
	kEctLocalChannelSelect = -5,
	kEctLocalNoNet = -6,
    kEctLocalCancel = -7,
    kEctLocalClear = -8,
    kEctLocalReset = -9,
	kEctLocalTaskParam = -12,
	kEctLocalCgiFrequcencyLimit = -13,
    
};

// -600 ~ -500
enum {
    kEctLongFirstPkgTimeout = -500,
    kEctLongPkgPkgTimeout = -501,
    kEctLongReadWriteTimeout = -502,
    kEctLongTaskTimeout = -503,
};

// -600 ~ -500
enum {
    kEctHttpFirstPkgTimeout = -500,
    kEctHttpPkgPkgTimeout = -501,
    kEctHttpReadWriteTimeout = -502,
    kEctHttpTaskTimeout = -503,
};

// -20000 ~ -10000
enum {
    kEctSocketNetworkChange = -10086,
    kEctSocketMakeSocketPrepared = -10087,
    kEctSocketWritenWithNonBlock = -10088,
    kEctSocketReadOnce = -10089,
    kEctSocketShutdown = -10090,
    kEctSocketRecvErr = -10091,
    kEctSocketSendErr = -10092,

    kEctHttpSplitHttpHeadAndBody = -10194,
    kEctHttpParseStatusLine = -10195,

    kEctNetMsgXPHandleBufferErr = -10504,

    kEctDnsMakeSocketPrepared = -10606,
};

enum NetStatus {
    kNetworkUnkown = -1,
    kNetworkUnavailable = 0,
    kGateWayFailed = 1,
    kServerFailed = 2,
    kConnecting = 3,
    kConnected = 4,
    kServerDown = 5
};

enum IdentifyMode {
    kCheckNow = 0,
    kCheckNext,
    kCheckNever
};
        
enum IPSourceType {
    kIPSourceNULL = 0,
    kIPSourceDebug,
    kIPSourceDNS,
    kIPSourceNewDns,
    kIPSourceProxy,
    kIPSourceBackup,
};

const char* const IPSourceTypeString[] = {
    "NullIP",
    "DebugIP",
    "DNSIP",
    "NewDNSIP",
    "ProxyIP",
    "BackupIP",
};
        
extern bool MakesureAuthed();
//底层询问上层该host对应的ip列表
extern std::vector<std::string> OnNewDns(const std::string& host);
//网络层收到push消息回调
extern void OnPush(int32_t cmdid, const AutoBuffer& msgpayload);
//底层获取task要发送的数据
extern bool Req2Buf(int32_t taskid,  void* const user_context, AutoBuffer& outbuffer, int& error_code, const int channel_select);
//底层回包返回给上层解析
extern int Buf2Resp(int32_t taskid, void* const user_context, const AutoBuffer& inbuffer, int& error_code, const int channel_select);
//任务执行结束
extern int  OnTaskEnd(int32_t taskid, void* const user_context, int error_type, int error_code);
//上报流量数据
extern void ReportFlow(int32_t wifi_recv, int32_t wifi_send, int32_t mobile_recv, int32_t mobile_send);
//上报网络连接状态
extern void ReportConnectStatus(int status, int longlink_status);
//长连信令校验 ECHECK_NOW = 0, ECHECK_NEVER = 1, ECHECK_NEXT = 2
extern int  GetLonglinkIdentifyCheckBuffer(AutoBuffer& identify_buffer, AutoBuffer& buffer_hash, int32_t& cmdid);
//长连信令校验回包
extern bool OnLonglinkIdentifyResponse(const AutoBuffer& response_buffer, const AutoBuffer& identify_buffer_hash);

extern void RequestSync();
        
}}
#endif // NETWORK_SRC_NET_COMM_H_