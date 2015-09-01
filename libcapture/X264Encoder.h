#ifndef __X264_ENCODE_H__
#define __X264_ENCODE_H__

#include <stdint.h>
#include "libcapture.h"
#include <deque>
#include "uv\uv.h"
#include "Codecer.h"

#ifdef __cplusplus
extern "C"{
#include "x264.h"
}
#endif

typedef void(*NALCALLBACK)(cc_src_sample_t sample, void* user_data);

class CX264Encoder : public CResource
{
	int width, height;
	x264_param_t param;
	x264_picture_t pic;
	x264_picture_t pic_out;
	x264_t *h;
	int i_frame;
	int i_frame_size;
	x264_nal_t *nal;
	int i_nal;

	bool b_stop;
	uv_loop_t* p_loop;
	uv_work_t enc_worker_req;

	void* p_user_data;
	NALCALLBACK fn_cb;

	uv_mutex_t queue_mutex;
	uv_cond_t queue_not_empty;
	std::deque<cc_src_sample_t> sample_queue;

	static void encode_worker(uv_work_t* req);
	static void after_encode(uv_work_t* req, int status);
	void encode();
	int close();
	int encode_delay();

public:
	CX264Encoder(uv_loop_t* loop);
	~CX264Encoder();
	int set_param(x264_param_t* param = 0);
	int open();
	int put_sample(cc_src_sample_t sample);
	int start_encode(NALCALLBACK cb, void* data);
	int stop_encode();
};

class CX264Encoder2 : public CEncoder
{
private:
    AVCodecContext		   *pCodecCtx;

    uv_mutex_t			   *pQueueMutex;
    uv_cond_t			   *pQueueNotEmpty;
    std::deque<AVFrame*>    queueFrame;

    bool                    bStop;
    uv_work_t				encodeWorkerReq;
    static void EncodeWorker(uv_work_t* req);
    static void AfterEncode(uv_work_t* req, int status);
    void Encode(void);

private:
    friend class CResourcePool;
    /* only get point through CResourcePool */
    CX264Encoder2(uv_loop_t* loop);
    virtual ~CX264Encoder2();

public:
    virtual int Init(void) = 0;
    virtual int SetParam(void* param) = 0;
    virtual int Put(AVFrame* frame);
    virtual int SetEncodeCallback(ENCODER_CB cb, void* data){ m_fnCb = cb, m_pUserdata = data; }
    virtual int Stop(void) = 0;
    virtual void Close(CLOSERESOURCECB cb = NULL){ CResource::Close(cb); }
};
#endif //__X264_ENCODE_H__