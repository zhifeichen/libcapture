#ifndef __CAPTURE_SYS_H__
#define __CAPTURE_SYS_H__

#include "uv/uv.h"
#include "aacencode.h"
#include "x264encode.h"
#include "DshowCapture.h"
#include "MsgSocket.h"

class CCaptureSys
{
    uv_loop_t      *m_pLoop;
    CX264Encoder         m_x264Enc;
    CAacEncoder          m_aacEnc;
    CAccessSys      m_access;
    CRemoteSys      m_remote;
    CMsgSocket     *m_pMsgSocket;
    cc_userinfo_t   m_userinfo;

    FNMSGCALLBACK   m_fnMsgCb;
    void           *m_pMsgCbUserData;

    /* is connected to the server */
    bool IsConnected(void);

    /* received socket msg callback funtion */
    static void DoReceivedMsg(MyMSG* msg, void* data);
    void DoReceivedMsg(MyMSG* msg);

    /* process msg functions */
    void DoConnectedMsg(MyMSG* msg);
    void DoClosedMsg(MyMSG* msg);
    void DoNewUserMsg(MyMSG* msg);
    void DoUserLogoutMsg(MyMSG* msg);
    void DoSocketErrMsg(MyMSG* msg);

    /* received frame callback function */
    static void DoReceivedFrame(int userid, cc_src_sample_t*, void* userdata);
    void DoReceivedFrame(int userid, cc_src_sample_t* frame);

    /* received dshow raw frame callback function */
    static void DoDshowRawFrame(cc_src_sample_t* frame, void* userdata);
    void DoDshowRawFrame(cc_src_sample_t* frame);

    // x264 encode call back
    static void DoX264Enc_cb(cc_src_sample_t smple, void* data);
    void DoX264Enc_cb(cc_src_sample_t smple);

    // aac encode call back
    static void DoAacEnc_cb(cc_src_sample_t smple, void* data);
    void DoAacEnc_cb(cc_src_sample_t smple);

    // connect to server function
    static void work_connect(uv_work_t* req){}
    static void after_work_connect(uv_work_t* req, int status);
    void after_work_connect(void);
public:
    CCaptureSys(uv_loop_t* loop);
    ~CCaptureSys();

    int SetUserInfo(cc_userinfo_t* info);

    int SetReceivedMsgCb(void* userdata, void(*cb)(MyMSG*, void* userdata));

    int ConnectToServer(char* ip, uint16_t port);
    int DisconnectToServer(void);

    HRESULT StartPreview(HWND h);
    HRESULT StopPreview(void);

    HRESULT StartCapture(void);
    HRESULT StopCapture(void);

    HRESULT StartRemotePreview(int userid, HWND h);
    HRESULT StopRemotePreview(int userid);


    void ResizeVideoWindow(HWND h);
    void ResizeRemoteVideoWindow(int userid, HWND h);

};

#endif // __CAPTURE_SYS_H__