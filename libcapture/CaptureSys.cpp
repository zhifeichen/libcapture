#include "CaptureSys.h"

CCaptureSys::CCaptureSys(uv_loop_t* loop):
m_pLoop(loop),
m_x264Enc(loop),
m_aacEnc(loop),
m_access(loop),
m_remote(loop),
m_pMsgSocket(NULL),
m_fnMsgCb(NULL), m_pMsgCbUserData(NULL)
{}

CCaptureSys::~CCaptureSys()
{}

void CCaptureSys::DoReceivedMsg(MyMSG* msg, void* data)
{
    CCaptureSys* sys = (CCaptureSys*)data;
    sys->DoReceivedMsg(msg);
}

void CCaptureSys::DoReceivedMsg(MyMSG* msg)
{
    if (!msg) return;
    switch (msg->code){
    case CODE_CONNECTED:
        DoConnectedMsg(msg);
        break;
    case CODE_CLOSED:
        DoClosedMsg(msg);
        break;
    case CODE_NEWUSER:
        DoNewUserMsg(msg);
        break;
    case CODE_USERLOGOUT:
        DoUserLogoutMsg(msg);
        break;
    case CODE_SOCKETERROR:
        DoSocketErrMsg(msg);
        break;
    }
}

/* process msg functions */
void CCaptureSys::DoConnectedMsg(MyMSG* msg)
{

}

void CCaptureSys::DoClosedMsg(MyMSG* msg)
{

}

void CCaptureSys::DoNewUserMsg(MyMSG* msg)
{

}

void CCaptureSys::DoUserLogoutMsg(MyMSG* msg)
{

}

void CCaptureSys::DoSocketErrMsg(MyMSG* msg)
{

}

void CCaptureSys::DoReceivedFrame(int userid, cc_src_sample_t* frame, void* userdata)
{
    CCaptureSys* sys = (CCaptureSys*)userdata;
    sys->DoReceivedFrame(userid, frame);
}

void CCaptureSys::DoReceivedFrame(int userid, cc_src_sample_t* frame)
{
    m_remote.putframe(frame);
}

void CCaptureSys::DoDshowRawFrame(cc_src_sample_t* frame, void* userdata)
{
    CCaptureSys* sys = (CCaptureSys*)userdata;
    sys->DoDshowRawFrame(frame);
}

void CCaptureSys::DoDshowRawFrame(cc_src_sample_t* frame)
{
    if (!frame) return;
    if (frame->sampletype == SAMPLEVIDEO){
        m_x264Enc.put_sample(*frame);
    } else if (frame->sampletype == SAMPLEAUDIO){
        m_aacEnc.put_sample(*frame);
    } else {
        for (int i = 0; i < frame->iplan; i++){
            free(frame->buf[i]);
        }
    }
}

int CCaptureSys::SetUserInfo(cc_userinfo_t* info)
{
    m_userinfo.userid = info->userid;
    m_userinfo.roomid = info->roomid;
    m_userinfo.port = info->port;
    memcpy(m_userinfo.ip, info->ip, 16);
    return 0;
}

void CCaptureSys::after_work_connect(uv_work_t* req, int status)
{
    CCaptureSys* sys = (CCaptureSys*)req->data;
    sys->after_work_connect();
    delete req;
}

void CCaptureSys::after_work_connect(void)
{
    m_pMsgSocket->connect_sever(m_userinfo.ip, m_userinfo.port);
}

int CCaptureSys::ConnectToServer(char* ip, uint16_t port)
{
    if (m_pMsgSocket){
        m_pMsgSocket->dis_connect(true);
        m_pMsgSocket = NULL;
    }
    m_pMsgSocket = new CMsgSocket(m_pLoop);
    if (!m_pMsgSocket){
        return -1;
    }
    m_pMsgSocket->set_local_user(&m_userinfo);
    m_pMsgSocket->on_received(this, DoReceivedMsg);
    m_pMsgSocket->on_received_frame(this, DoReceivedFrame);
    uv_work_t* req = new uv_work_t();
    req->data = this;
    int ret = uv_queue_work(m_pLoop, req, work_connect, after_work_connect);
    return ret;
}

int CCaptureSys::DisconnectToServer(void)
{
    if (m_pMsgSocket){
        m_pMsgSocket->dis_connect(true);
        m_pMsgSocket = NULL;
    }
    return 0;
}

int CCaptureSys::SetReceivedMsgCb(void* userdata, void(*cb)(MyMSG*, void* userdata))
{
    m_fnMsgCb = cb;
    m_pMsgCbUserData = userdata;
    return 0;
}

HRESULT CCaptureSys::StartPreview(HWND h)
{
    HRESULT hr = S_FALSE;
    hr = m_access.StartPreview(h);
    return hr;
}

HRESULT CCaptureSys::StopPreview(void)
{
    HRESULT hr = S_FALSE;
    return hr;
}

HRESULT CCaptureSys::StartCapture(void)
{
    HRESULT hr = S_FALSE;
    return hr;
}

HRESULT CCaptureSys::StopCapture(void)
{
    HRESULT hr = S_FALSE;
    return hr;
}

HRESULT CCaptureSys::StartRemotePreview(int userid, HWND h)
{
    HRESULT hr = S_FALSE;
    return hr;
}

HRESULT CCaptureSys::StopRemotePreview(int userid)
{
    HRESULT hr = S_FALSE;
    return hr;
}


void CCaptureSys::ResizeVideoWindow(HWND h)
{

}

void CCaptureSys::ResizeRemoteVideoWindow(int userid, HWND h)
{

}