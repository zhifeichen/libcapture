#ifndef __DSHOW_CAPTURE_H__
#define __DSHOW_CAPTURE_H__

#include <dshow.h>
#include <Wmcodecdsp.h>
#include <deque>
#include <map>

#include "IFilter.h"
#include "MsgSocket.h"
#include "decode_video.h"
#include "decode_audio.h"


using namespace std;

#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }

typedef struct MyMediaSample
{
	IMediaSample *p_sample;
	REFTIME i_timestamp;

} MyMediaSample;

/*****************************************************************************
* DirectShow elementary stream descriptor
*****************************************************************************/
class CaptureFilter;
typedef struct dshow_stream_t
{
	string          devicename;
	IBaseFilter     *p_device_filter;
	CaptureFilter   *p_capture_filter;
	AM_MEDIA_TYPE   mt;

	union
	{
		VIDEOINFOHEADER video;
		WAVEFORMATEX    audio;
	} header;

	int             i_fourcc;

	bool      b_pts;

	deque<MyMediaSample> samples_queue;
} dshow_stream_t;

enum PLAYSTATE { Stopped, Paused, Running, Init };

class CRemoteSys
{
	video_decoder			vdecoder;
	audio_decoder			adecoder;
	uv_loop_t			   *p_loop;

	HWND					h_wnd; /* preview window handle */
	cc_userinfo_t			m_userinfo;

	bool					b_has_interface;

public:
	CRemoteSys(uv_loop_t* loop);

	HRESULT StartRemotePreview(int userid, HWND h);
	HRESULT StopRemotePreview(void);

	void ResizeVideoWindow(HWND h);

	void Msg(TCHAR *szFormat, ...);

    void putframe(cc_src_sample_t* frame);
};

class remote_map
{
	typedef std::map<int, CRemoteSys*> rMap;
	typedef std::map<int, CRemoteSys*>::iterator rIt;
	rMap					m_remote;

public:
	remote_map();
	~remote_map();
	int set(int userid, uv_loop_t* loop);
	CRemoteSys* get(int userid);
	int remove(int userid);
	int remove_all(void);
};

/****************************************************************************
* Access descriptor declaration
****************************************************************************/
class CAccessSys
{
	/* These 2 must be left at the beginning */
	//vlc_mutex_t lock;
	//vlc_cond_t  wait;

	IGraphBuilder          *p_graph;
	ICaptureGraphBuilder2  *p_capture_graph_builder2;
	IMediaControl          *p_control;
	IVideoWindow		   *p_video_window;
	IMediaEventEx		   *p_media_event;
	IAMStreamConfig		   *p_VSC;
    CMyCapVideoFilter      *p_VideoFilter;
    CMyCapAudioFilter      *p_AudioFilter;

	HWND					h_wnd; /* preview window handle */
	/* list of elementary streams */
	dshow_stream_t p_streams[2]; /* 0: video; 1: audio */
	int            i_streams;
	int            i_current_stream;

	CCritSec	   m_lock; /* serialize state changes */
	CCritSec	   m_alock; /* serialize state changes for audio */
	/* misc properties */
	int            i_width;
	int            i_height;
	int            i_chroma;
	bool           b_chroma; /* Force a specific chroma on the dshow input */

	bool		   b_getInterfaces;
	bool		   b_buildPreview;
	bool		   b_buildCapture;

	bool		   b_savefile;
	bool		   b_sendsample;

	uv_loop_t*	   p_loop;
	/* play stat */
	PLAYSTATE e_psCurrent;

    /* private functions */
    HRESULT GetInterfaces(void);
    void CloseInterfaces(void);

    HRESULT FindCaptureDevice(void);

    HRESULT BuildPreview(void);
    HRESULT BuildCapture(void);

    HRESULT HandleGraphEvent(void);

    void Msg(TCHAR *szFormat, ...);

	/* public functions */
public:
	CAccessSys(uv_loop_t* loop);
    ~CAccessSys(void);

	HRESULT StartPreview(HWND h);
	HRESULT StopPreview(void);

	HRESULT StartCapture(void);
	HRESULT StopCapture(void);
	
	HRESULT SetupVideoWindow(HWND h);
	
	void ResizeVideoWindow(HWND h);

    int SetRawFrameCallback(RawFrameCallBack cb, void* data);

};


#endif //__DSHOW_CAPTURE_H__