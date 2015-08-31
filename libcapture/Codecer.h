#ifndef __CODECER_H__
#define __CODECER_H__

#include "uv/uv.h"
#include "ResourcePool.h"

#ifdef __cplusplus
extern "C"{
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"
}
#endif

typedef void(*ENCODER_CB)(AVPacket* sample, void* user_data);
typedef void(*DECODER_CB)(AVFrame* sample, void* user_data);

class CEncoder: public CResource
{
protected:
    uv_loop_t  *m_pLoop;
    ENCODER_CB  m_fnCb;
    void       *m_pUserdata;
    CEncoder(uv_loop_t* loop, E_RESOURCE_TYPE type):m_pLoop(loop), m_pUserdata(NULL), CResource(type){}
    virtual ~CEncoder(void){}

public:
    virtual int Init(void) = 0;
    virtual int SetParam(void* param) = 0;
    virtual int Put(AVFrame* frame) = 0;
    virtual int SetEncodeCallback(ENCODER_CB cb, void* data){ m_fnCb = cb, m_pUserdata = data; }
    virtual int Stop(void) = 0;
    virtual void Close(CLOSERESOURCECB cb = NULL){ CResource::Close(cb); }
};

class CDecoder: public CResource
{
protected:
    uv_loop_t  *m_pLoop;
    DECODER_CB  m_fnCb;
    void       *m_pUserdata;
    CDecoder(uv_loop_t* loop, E_RESOURCE_TYPE type):m_pLoop(loop), m_pUserdata(NULL), CResource(type){}
    virtual ~CDecoder(void){}

public:
    virtual int Init(void) = 0;
    virtual int Put(AVPacket* frame) = 0;
    virtual int SetDecodeCallback(DECODER_CB cb, void* data){ m_fnCb = cb, m_pUserdata = data; }
    virtual int Stop(void) = 0;
    virtual void Close(CLOSERESOURCECB cb = NULL){ CResource::Close(cb); }
};

#endif // __CODECER_H__