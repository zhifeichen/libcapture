#include "stdafx.h"
#include "IFilter.h"
#include "DshowCapture.h"
#include <initguid.h>
#include "css_protocol_package.h"


// {D06A31CB-3518-4054-A167-65CE67D7A931}
DEFINE_GUID(CLSID_MyCapVideoFilter,
	0xd06a31cb, 0x3518, 0x4054, 0xa1, 0x67, 0x65, 0xce, 0x67, 0xd7, 0xa9, 0x31);

// {25A7F004-9B2D-4B77-AEFC-08B0084F2D4F}
DEFINE_GUID(CLSID_CMyCapAudioFilter,
	0x25a7f004, 0x9b2d, 0x4b77, 0xae, 0xfc, 0x08, 0xb0, 0x08, 0x4f, 0x2d, 0x4f);

// {7D732DC9-5770-4D20-9090-14429900992C}
DEFINE_GUID(CLSID_PushSourceFilter,
	0x7d732dc9, 0x5770, 0x4d20, 0x90, 0x90, 0x14, 0x42, 0x99, 0x0, 0x99, 0x2c);


// Constructor

CMyCapVideoFilter::CMyCapVideoFilter(CAccessSys *psys,
	LPUNKNOWN pUnk,
	CCritSec *pLock,
	HRESULT *phr) :
	CBaseFilter(NAME("CMyCapVideoFilter"), pUnk, pLock, CLSID_MyCapVideoFilter),
	m_pLock(pLock),
	//m_hFile(INVALID_HANDLE_VALUE),
    m_cb(NULL), m_userdata(NULL)
{
	m_pPin = new CMyCapVideoInputPin(this);
}

// 
// Set call back
//
void CMyCapVideoFilter::SetRawFrameCallBack(RawFrameCallBack cb, void* data)
{
    m_cb = cb;
    m_userdata = data;
}

//
// GetPin
//
CBasePin * CMyCapVideoFilter::GetPin(int n)
{
	if (n == 0) {
		return m_pPin;
	}
	else {
		return NULL;
	}
}


//
// GetPinCount
//
int CMyCapVideoFilter::GetPinCount()
{
	return 1;
}


//
// Stop
//
// Overriden to close the dump file
//
STDMETHODIMP CMyCapVideoFilter::Stop()
{
	CAutoLock cObjectLock(m_pLock);

	//CloseFile();
	//m_enc.stop_encode();
	return CBaseFilter::Stop();
}


//
// Pause
//
// Overriden to open the dump file
//
STDMETHODIMP CMyCapVideoFilter::Pause()
{
	CAutoLock cObjectLock(m_pLock);

	//OpenFile();
	//m_enc.set_param();
	//m_enc.open();
	return CBaseFilter::Pause();
}


//
// Run
//
// Overriden to open the dump file
//
STDMETHODIMP CMyCapVideoFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cObjectLock(m_pLock);

	// Clear the global 'write error' flag that would be set
	// if we had encountered a problem writing the previous dump file.
	// (eg. running out of disk space).
	//
	// Since we are restarting the graph, a new file will be created.
	//OpenFile();
	//m_enc.start_encode(enc_cb, this);
	//MessageBox(NULL, TEXT("m_enc.start_encode(enc_cb, this);"), TEXT("CMyCapVideoFilter"), MB_OK);
	return CBaseFilter::Run(tStart);
}

//
// encode call back
//
void CMyCapVideoFilter::enc_cb(cc_src_sample_t smple, void* data)
{
	CMyCapVideoFilter* f = (CMyCapVideoFilter*)data;
	f->enc_cb(smple);
}

void CMyCapVideoFilter::enc_cb(cc_src_sample_t smple)
{
	//MyMSG msg;
	//msg.bodylen = sizeof(msg.body.sample);
	//msg.body.sample = smple;
	//msg.code = 0;
	//if (m_psys->m_msgsock.receive_cb_){
	//	m_psys->m_msgsock.receive_cb_(&msg, m_psys->m_msgsock.msgcb_userdata_);
	//}
	if (0){
		JMedia_sample s;
		ssize_t len = 0;
		char *buf = NULL;

		JMedia_sample_init(&s);
		s.FrameType = SAMPLEVIDEO;
		s.FrameData.base = (char*)smple.buf[0];
		s.FrameData.len = smple.len;

		len = css_proto_package_calculate_buf_len((JNetCmd_Header*)&s);
		buf = (char *)malloc(len);
		if (buf == NULL)
			return;

		css_proto_package_encode(buf, len, (JNetCmd_Header*)&s);

		//m_psys->MsgSocket()->send((uint8_t*)buf, len);
	}
}


//
//  Definition of CDumpInputPin
//
CMyCapVideoInputPin::CMyCapVideoInputPin(CMyCapVideoFilter* pFilter) :

	CRenderedInputPin(NAME("CMyCapVideoInputPin"),
	pFilter,                   // Filter
	pFilter->m_pLock,          // Locking
	&pFilter->m_hr,                       // Return code
	L"CMyCapVideoInputPin"),                 // Pin name
	//m_pReceiveLock(pReceiveLock),
	m_pfilter(pFilter),
	m_tLast(0),
	m_pSwsCtx(NULL)
{
}

int CMyCapVideoInputPin::ConvertFmt(const CMediaType *mt)
{
	m_mt = *mt;
	if (mt->subtype == MEDIASUBTYPE_RGB24){
		m_pix_fmt = PIX_FMT_BGR24;// PIX_FMT_RGB24;
	}
	else if (mt->subtype == MEDIASUBTYPE_I420){
		m_pix_fmt = PIX_FMT_YUV420P;
	}
	else if (mt->subtype == MEDIASUBTYPE_YUY2){
		m_pix_fmt = PIX_FMT_YUYV422;
	}
	else {
		m_pix_fmt = PIX_FMT_NONE;
	}
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)mt->pbFormat;
	m_height = pvi->bmiHeader.biHeight;
	m_width = pvi->bmiHeader.biWidth;

	m_pSwsCtx = sws_getContext(m_width, m_height, m_pix_fmt, m_width, m_height, PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
	return 0;
}

//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT CMyCapVideoInputPin::CheckMediaType(const CMediaType *mt)
{
	if (mt->majortype == MEDIATYPE_Video/* && mt->subtype == MEDIASUBTYPE_YUY2*/){
		ConvertFmt(mt);
		return S_OK;
	} else
		return S_FALSE;
}


//
// BreakConnect
//
// Break a connection
//
HRESULT CMyCapVideoInputPin::BreakConnect()
{
	//if (m_pDump->m_pPosition != NULL) {
	//	m_pDump->m_pPosition->ForceRefresh();
	//}

	return CRenderedInputPin::BreakConnect();
}


//
// ReceiveCanBlock
//
// We don't hold up source threads on Receive
//
STDMETHODIMP CMyCapVideoInputPin::ReceiveCanBlock()
{
	return S_FALSE;
}


void CMyCapVideoInputPin::ConvertRGB(uint8_t* dst, uint8_t* src)
{
	int i;
	int pd = 0;
	int l = m_width * 3;
	for (i = m_height - 1; i > -1; i--){
		memcpy(dst + pd, src + i*l, l);
		pd += l;
	}
}

//
// Receive
//
// Do something with this media sample
//
STDMETHODIMP CMyCapVideoInputPin::Receive(IMediaSample *pSample)
{
	CheckPointer(pSample, E_POINTER);

	//CAutoLock lock(m_pReceiveLock);
	PBYTE pbData;
	AM_MEDIA_TYPE *mt;
	HRESULT hr;

	hr = pSample->GetMediaType(&mt);
	::DeleteMediaType(mt);
	// Has the filter been stopped yet?
	//if (m_pfilter->m_hFile == INVALID_HANDLE_VALUE) {
	//	return NOERROR;
	//}

	REFERENCE_TIME tStart, tStop;
	pSample->GetTime(&tStart, &tStop);

	DbgLog((LOG_TRACE, 1, TEXT("tStart(%s), tStop(%s), Diff(%d ms), Bytes(%d)"),
		(LPCTSTR)CDisp(tStart),
		(LPCTSTR)CDisp(tStop),
		(LONG)((tStart - m_tLast) / 10000),
		pSample->GetActualDataLength()));

	m_tLast = tStart;

	// Copy the data to the file

	hr = pSample->GetPointer(&pbData);
	if (FAILED(hr)) {
		return hr;
	}
	
	if (/*m_pfilter->m_psys->b_sendsample*/ /*m_pfilter->m_psys->m_msgsock.receive_cb_*/ 1){
		//m_pfilter->m_psys->m_msgsock.send(pSample);
		MyMSG msg;
		msg.bodylen = sizeof(msg.body.sample);
		msg.body.sample.len = pSample->GetActualDataLength();
		//msg.body.sample.buf[0] = (unsigned char*)malloc(msg.body.sample.len);
		msg.code = 0;
		pSample->GetTime(&msg.body.sample.start, &msg.body.sample.end);
		TCHAR m[MAX_PATH];
		swprintf(m, TEXT("LEN: %d"), msg.body.sample.len);
		//MessageBox(NULL, m, TEXT("CMyCapVideoInputPin"), MB_OK);
		if (pSample->IsSyncPoint() == S_OK){
			msg.body.sample.sync = 1;
		}
		else{
			msg.body.sample.sync = 0;
		}
		//memcpy(msg.body.sample.buf[0], pbData, msg.body.sample.len);
		AVPicture input_pic;
		AVPicture output_pic;
		//AVFrame input_pic;
		//AVFrame output_pic;
		//AVFrame frame;
		avpicture_alloc((AVPicture*)&output_pic, /*PIX_FMT_RGB24*/PIX_FMT_YUV420P, m_width, m_height);
		if (m_pix_fmt == PIX_FMT_BGR24){
			int inlen = avpicture_get_size(m_pix_fmt, m_width, m_height);
			uint8_t* buf = (uint8_t*)malloc(inlen);
			ConvertRGB(buf, pbData);
			avpicture_fill((AVPicture*)&input_pic, buf, m_pix_fmt, m_width, m_height);
			sws_scale(m_pSwsCtx, input_pic.data, input_pic.linesize, 0, m_height, output_pic.data, output_pic.linesize);
			free(buf);
		} else {
			avpicture_fill((AVPicture*)&input_pic, pbData, m_pix_fmt, m_width, m_height);
			sws_scale(m_pSwsCtx, input_pic.data, input_pic.linesize, 0, m_height, output_pic.data, output_pic.linesize);
		}
		//sws_scale(m_pSwsCtx, input_pic.data, input_pic.linesize, 0, m_height, output_pic.data, output_pic.linesize);
		int inlen = avpicture_get_size(m_pix_fmt, m_width, m_height);
		int outlen = avpicture_get_size(PIX_FMT_YUV420P, m_width, m_height);
		msg.body.sample.iplan = 3;
		//free(msg.body.sample.buf[0]);
		/*for (int i = 0; i < 3; i++)*/{
			msg.body.sample.line[0] = output_pic.linesize[0];
			msg.body.sample.buf[0] = (uint8_t*)malloc(output_pic.linesize[0]*m_height);
			memcpy(msg.body.sample.buf[0], output_pic.data[0], output_pic.linesize[0] * m_height);

			msg.body.sample.line[1] = output_pic.linesize[1];
			msg.body.sample.buf[1] = (uint8_t*)malloc(output_pic.linesize[1] * m_height / 2);
			memcpy(msg.body.sample.buf[1], output_pic.data[1], output_pic.linesize[1] * m_height / 2);

			msg.body.sample.line[2] = output_pic.linesize[2];
			msg.body.sample.buf[2] = (uint8_t*)malloc(output_pic.linesize[2] * m_height / 2);
			memcpy(msg.body.sample.buf[2], output_pic.data[2], output_pic.linesize[2] * m_height / 2);
		}
		
		//AVPacket avpkt;
		//int got_packet_ptr = 0;
		//av_init_packet(&avpkt);
		//avpkt.data = outbuf;
		//avpkt.size = outbuf_size;
		//u_size = avcodec_encode_video2(c, &avpkt, m_pYUVFrame, &got_packet_ptr);
		//if (u_size == 0)
		//{
		//	fwrite(avpkt.data, 1, avpkt.size, f);
		//}
		
		avpicture_free((AVPicture*)&output_pic);
		//memcpy(msg.body.sample.buf, pbData, msg.body.sample.len);
		//m_pfilter->m_psys->m_msgsock.receive_cb_(&msg, m_pfilter->m_psys->m_msgsock.msgcb_userdata_);
		//m_pfilter->m_enc.put_sample(msg.body.sample);
        if (m_pfilter->m_cb)
            m_pfilter->m_cb(&msg.body.sample, m_pfilter->m_userdata);
		//MessageBox(NULL, TEXT("m_pfilter->m_enc.put_sample(msg.body.sample);"), TEXT("CMyCapVideoInputPin"), MB_OK);
	}
	return S_OK;
	//return m_pfilter->Write(pbData, pSample->GetActualDataLength());
}

//
// EndOfStream
//
STDMETHODIMP CMyCapVideoInputPin::EndOfStream(void)
{
	//CAutoLock lock(m_pReceiveLock);
	if (m_pSwsCtx) sws_freeContext(m_pSwsCtx);
	return CRenderedInputPin::EndOfStream();

} // EndOfStream


//
// NewSegment
//
// Called when we are seeked
//
STDMETHODIMP CMyCapVideoInputPin::NewSegment(REFERENCE_TIME tStart,
	REFERENCE_TIME tStop,
	double dRate)
{
	m_tLast = 0;
	return S_OK;

} // NewSegment


/*=============*/
/* AUDIO CAP   */
/*=============*/

// Constructor

CMyCapAudioFilter::CMyCapAudioFilter(CAccessSys *psys,
	LPUNKNOWN pUnk,
	CCritSec *pLock,
	HRESULT *phr) :
	CBaseFilter(NAME("CMyCapAudioFilter"), pUnk, pLock, CLSID_CMyCapAudioFilter),
	m_pLock(pLock),
    m_cb(NULL), m_userdata(NULL)
{
	m_pPin = new CMyCapAudioInputPin(this);
}

// 
// Set call back
//
void CMyCapAudioFilter::SetRawFrameCallBack(RawFrameCallBack cb, void* data)
{
    m_cb = cb;
    m_userdata = data;
}

//
// GetPin
//
CBasePin * CMyCapAudioFilter::GetPin(int n)
{
	if (n == 0) {
		return m_pPin;
	}
	else {
		return NULL;
	}
}


//
// GetPinCount
//
int CMyCapAudioFilter::GetPinCount()
{
	return 1;
}


//
// Stop
//
// Overriden to close the dump file
//
STDMETHODIMP CMyCapAudioFilter::Stop()
{
	CAutoLock cObjectLock(m_pLock);

	//m_enc.stop_encode();
	return CBaseFilter::Stop();
}


//
// Pause
//
// Overriden to open the dump file
//
STDMETHODIMP CMyCapAudioFilter::Pause()
{
	CAutoLock cObjectLock(m_pLock);

	//m_enc.set_param(m_pPin->GetMediaType());
	//m_enc.open();
	return CBaseFilter::Pause();
}


//
// Run
//
// Overriden to open the dump file
//
STDMETHODIMP CMyCapAudioFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cObjectLock(m_pLock);

	// Clear the global 'write error' flag that would be set
	// if we had encountered a problem writing the previous dump file.
	// (eg. running out of disk space).
	//
	// Since we are restarting the graph, a new file will be created.

	//m_enc.start_encode(enc_cb, this);
	//MessageBox(NULL, TEXT("m_enc.start_encode(enc_cb, this);"), TEXT("CMyCapVideoFilter"), MB_OK);
	return CBaseFilter::Run(tStart);
}

//
// encode call back
//
void CMyCapAudioFilter::enc_cb(cc_src_sample_t smple, void* data)
{
	CMyCapAudioFilter* f = (CMyCapAudioFilter*)data;
	f->enc_cb(smple);
}

void CMyCapAudioFilter::enc_cb(cc_src_sample_t smple)
{
	//MyMSG msg;
	//msg.bodylen = sizeof(msg.body.sample);
	//msg.body.sample = smple;
	//msg.code = 0;
	//if (m_psys->m_msgsock.receive_cb_){
	//	m_psys->m_msgsock.receive_cb_(&msg, m_psys->m_msgsock.msgcb_userdata_);
	//}
	if (0){
		JMedia_sample s;
		ssize_t len = 0;
		char *buf = NULL;

		JMedia_sample_init(&s);
		s.FrameType = SAMPLEAUDIO;
		s.FrameData.base = (char*)smple.buf[0];
		s.FrameData.len = smple.len;
		
		len = css_proto_package_calculate_buf_len((JNetCmd_Header*)&s);
		buf = (char *)malloc(len);
		if (buf == NULL)
			return ;

		css_proto_package_encode(buf, len, (JNetCmd_Header*)&s);

		//m_psys->MsgSocket()->send((uint8_t*)buf, len);
	}
}



//
//  Definition of CDumpInputPin
//
CMyCapAudioInputPin::CMyCapAudioInputPin(CMyCapAudioFilter* pFilter) :

CRenderedInputPin(NAME("CMyCapVideoInputPin"),
pFilter,                   // Filter
pFilter->m_pLock,          // Locking
&pFilter->m_hr,            // Return code
L"CMyCapVideoInputPin"),   // Pin name
//m_pReceiveLock(pReceiveLock),
m_pfilter(pFilter),
m_tLast(0),
m_pSwsCtx(NULL)
{
}

int CMyCapAudioInputPin::ConvertFmt(const CMediaType *mt)
{
	m_mt = *mt;
	if (mt->formattype == FORMAT_WaveFormatEx){
		WAVEFORMATEX *pai = (WAVEFORMATEX *)mt->pbFormat;
		WORD  wFormatTag = pai->wFormatTag;
		WORD  nChannels = pai->nChannels;
		DWORD nSamplesPerSec = pai->nSamplesPerSec;
		DWORD nAvgBytesPerSec = pai->nAvgBytesPerSec;
		WORD  nBlockAlign = pai->nBlockAlign;
		WORD  wBitsPerSample = pai->wBitsPerSample;
		WORD  cbSize = pai->cbSize;
		//wFormatTag == WAVE_FORMAT_PCM;
		//nAvgBytesPerSec == nSamplesPerSec * nBlockAlign;
		//nBlockAlign == (nChannels * wBitsPerSample)/8;
	}

	return 0;
}

//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT CMyCapAudioInputPin::CheckMediaType(const CMediaType *mt)
{
	if (mt->majortype == MEDIATYPE_Audio && mt->subtype == MEDIASUBTYPE_PCM){
		ConvertFmt(mt);
		return S_OK;
	}
	else
		return S_FALSE;
}

const CMediaType * CMyCapAudioInputPin::GetMediaType(void)
{
	return &m_mt;
}


//
// BreakConnect
//
// Break a connection
//
HRESULT CMyCapAudioInputPin::BreakConnect()
{
	//if (m_pDump->m_pPosition != NULL) {
	//	m_pDump->m_pPosition->ForceRefresh();
	//}

	return CRenderedInputPin::BreakConnect();
}


//
// ReceiveCanBlock
//
// We don't hold up source threads on Receive
//
STDMETHODIMP CMyCapAudioInputPin::ReceiveCanBlock()
{
	return S_FALSE;
}

static FILE* pcm = NULL;
//
// Receive
//
// Do something with this media sample
//
STDMETHODIMP CMyCapAudioInputPin::Receive(IMediaSample *pSample)
{
	CheckPointer(pSample, E_POINTER);

	//CAutoLock lock(m_pReceiveLock);
	PBYTE pbData;
	AM_MEDIA_TYPE *mt;
	HRESULT hr;

	hr = pSample->GetMediaType(&mt);
	::DeleteMediaType(mt);
	// Has the filter been stopped yet?
	//if (m_pfilter->m_hFile == INVALID_HANDLE_VALUE) {
	//	return NOERROR;
	//}

	REFERENCE_TIME tStart, tStop;
	pSample->GetTime(&tStart, &tStop);

	DbgLog((LOG_TRACE, 1, TEXT("tStart(%s), tStop(%s), Diff(%d ms), Bytes(%d)"),
		(LPCTSTR)CDisp(tStart),
		(LPCTSTR)CDisp(tStop),
		(LONG)((tStart - m_tLast) / 10000),
		pSample->GetActualDataLength()));

	m_tLast = tStart;

	// Copy the data to the file

	hr = pSample->GetPointer(&pbData);
	if (FAILED(hr)) {
		return hr;
	}

	if (/*m_pfilter->m_psys->b_sendsample*/ /*m_pfilter->m_psys->m_msgsock.receive_cb_*/ 1){
		//m_pfilter->m_psys->m_msgsock.send(pSample);
		MyMSG msg;
		msg.bodylen = sizeof(msg.body.sample);
		msg.body.sample.len = pSample->GetActualDataLength();// default 88200 byte, set to 4096
		msg.body.sample.buf[0] = (unsigned char*)malloc(msg.body.sample.len);
		msg.body.sample.iplan = 1;
		msg.code = 0;
		pSample->GetTime(&msg.body.sample.start, &msg.body.sample.end);
		int64_t persist = msg.body.sample.end - msg.body.sample.start;
		TCHAR m[MAX_PATH];
		swprintf(m, TEXT("LEN: %d"), msg.body.sample.len);
		//MessageBox(NULL, m, TEXT("CMyCapVideoInputPin"), MB_OK);
		if (pSample->IsSyncPoint() == S_OK){
			msg.body.sample.sync = 1;
		}
		else{
			msg.body.sample.sync = 0;
		}
		memcpy(msg.body.sample.buf[0], pbData, msg.body.sample.len);
		//if (!pcm){
		//	pcm = fopen("src.pcm", "wb");
		//	if (pcm)
		//		MessageBox(NULL, TEXT("create file!"), TEXT("aac encoder"), MB_OK);
		//	else
		//		MessageBox(NULL, TEXT("create file false!"), TEXT("aac encoder"), MB_OK);
		//}
		//if (pcm){
		//	fwrite(msg.body.sample.buf[0], 1, msg.body.sample.len, pcm);
		//}
		//AVPacket avpkt;
		//int got_packet_ptr = 0;
		//av_init_packet(&avpkt);
		//avpkt.data = outbuf;
		//avpkt.size = outbuf_size;
		//u_size = avcodec_encode_video2(c, &avpkt, m_pYUVFrame, &got_packet_ptr);
		//if (u_size == 0)
		//{
		//	fwrite(avpkt.data, 1, avpkt.size, f);
		//}

		//memcpy(msg.body.sample.buf, pbData, msg.body.sample.len);
		//m_pfilter->m_enc.put_sample(msg.body.sample);
        if (m_pfilter->m_cb)
            m_pfilter->m_cb(&msg.body.sample, m_pfilter->m_userdata);
	}
	return S_OK;
}

//
// EndOfStream
//
STDMETHODIMP CMyCapAudioInputPin::EndOfStream(void)
{
	//CAutoLock lock(m_pReceiveLock);
	if (m_pSwsCtx) sws_freeContext(m_pSwsCtx);
	return CRenderedInputPin::EndOfStream();

} // EndOfStream


//
// NewSegment
//
// Called when we are seeked
//
STDMETHODIMP CMyCapAudioInputPin::NewSegment(REFERENCE_TIME tStart,
	REFERENCE_TIME tStop,
	double dRate)
{
	m_tLast = 0;
	return S_OK;

} // NewSegment


/*=============*/
/* out put pin */
/*=============*/
CPushPinSrc::CPushPinSrc(HRESULT *phr, CSource *pFilter)
: CSourceStream(NAME("Push Source Desktop"), phr, pFilter, L"Out"),
m_FramesWritten(0),
m_bZeroMemory(0),
m_iFrameNumber(0)
{
	uv_mutex_init(&queue_mutex);
	uv_cond_init(&queue_not_empty);
}

CPushPinSrc::~CPushPinSrc()
{
	DbgLog((LOG_TRACE, 3, TEXT("Frames written %d"), m_iFrameNumber));
	uv_mutex_destroy(&queue_mutex);
	uv_cond_destroy(&queue_not_empty);
}

HRESULT CPushPinSrc::SetMediaType(const CMediaType *pMediaType)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	// Pass the call up to my base class
	HRESULT hr = CSourceStream::SetMediaType(pMediaType);

	return hr;

} // SetMediaType


//
// CheckMediaType
//
//
HRESULT CPushPinSrc::CheckMediaType(const CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);

	if ((*(pMediaType->Type()) != MEDIATYPE_Video) )
	{
		return E_INVALIDARG;
	}

	// Check for the subtypes we support
	const GUID *SubType = pMediaType->Subtype();
	if (SubType == NULL)
		return E_INVALIDARG;

	if ((*SubType != MEDIASUBTYPE_H264)
		&& (*SubType != MEDIASUBTYPE_h264)
		&& (*SubType != MEDIASUBTYPE_X264)
		&& (*SubType != MEDIASUBTYPE_x264))
	{
		return E_INVALIDARG;
	}

	//if (!pMediaType->bTemporalCompression){
	//	return E_INVALIDARG;
	//}


	return S_OK;  // This format is acceptable.

} // CheckMediaType

//
// GetMediaType
//
HRESULT CPushPinSrc::GetMediaType(CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);
	CAutoLock cAutoLock(m_pFilter->pStateLock());


	pmt->SetType(&MEDIATYPE_Video);
	pmt->SetSubtype(&MEDIASUBTYPE_x264);
	pmt->SetSampleSize(1);
	pmt->SetTemporalCompression(TRUE);
	//pmt->SetFormatType(&FORMAT_MPEG2Video);
	//pmt->SetTemporalCompression(FALSE);
	
	//pmt->cbFormat = sizeof(MPEG2VIDEOINFO);
	//pmt->pbFormat = (BYTE*)CoTaskMemAlloc(pmt->cbFormat);
	//if (pmt->pbFormat == NULL)
	//{
		// Out of memory; return an error code.
	//}
	//ZeroMemory(pmt->pbFormat, pmt->cbFormat);

	//// Cast the buffer pointer to an MPEG2VIDEOINFO struct.
	//MPEG2VIDEOINFO *pMVIH = (MPEG2VIDEOINFO*)pmt->pbFormat;

	//RECT rcSrc = { 0, 480, 0, 640 };        // Source rectangle.
	//pMVIH->hdr.rcSource = rcSrc;
	//pMVIH->hdr.dwBitRate = 4000000;       // Bit rate.
	//pMVIH->hdr.AvgTimePerFrame = 333667;  // 29.97 fps.
	//pMVIH->hdr.dwPictAspectRatioX = 4;    // 4:3 aspect ratio.
	//pMVIH->hdr.dwPictAspectRatioY = 3;

	//// BITMAPINFOHEADER information.
	//pMVIH->hdr.bmiHeader.biSize = 40;
	//pMVIH->hdr.bmiHeader.biWidth = 640;
	//pMVIH->hdr.bmiHeader.biHeight = 480;

	//pMVIH->dwLevel = AM_MPEG2Profile_Main;  // MPEG-2 profile. 
	//pMVIH->dwProfile = AM_MPEG2Level_Main;  // MPEG-2 level.

	return NOERROR;

} // GetMediaType


//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
//
HRESULT CPushPinSrc::DecideBufferSize(IMemAllocator *pAlloc,
	ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pProperties, E_POINTER);

	CAutoLock cAutoLock(m_pFilter->pStateLock());
	HRESULT hr = NOERROR;

	//VIDEOINFO *pvi = (VIDEOINFO *)m_mt.Format();
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 64*1024;

	ASSERT(pProperties->cbBuffer);

	// Ask the allocator to reserve us some sample memory. NOTE: the function
	// can succeed (return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted.
	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr))
	{
		return hr;
	}

	// Is this allocator unsuitable?
	if (Actual.cbBuffer < pProperties->cbBuffer)
	{
		return E_FAIL;
	}

	// Make sure that we have only 1 buffer (we erase the ball in the
	// old buffer to save having to zero a 200k+ buffer every time
	// we draw a frame)
	ASSERT(Actual.cBuffers == 1);
	return NOERROR;

} // DecideBufferSize


// This is where we insert the DIB bits into the video stream.
// FillBuffer is called once for every sample in the stream.
HRESULT CPushPinSrc::FillBuffer(IMediaSample *pSample)
{
	BYTE *pData;
	long cbData;
	HRESULT hr;

	CheckPointer(pSample, E_POINTER);
	//CAutoLock cAutoLockShared(&m_cSharedState);
	uv_mutex_lock(&queue_mutex);
	if (buf_deque.size() == 0){
		//uv_mutex_unlock(&queue_mutex);
		int ret = uv_cond_timedwait(&queue_not_empty, &queue_mutex, (uint64_t)(5000 * 1e6));
		if (ret == UV_ETIMEDOUT){
			uv_mutex_unlock(&queue_mutex);
			MessageBox(NULL, TEXT("BUFFER EMPTY!"), TEXT("remote preview"), MB_OK);
			return E_FAIL;
		}
	}

	hr = pSample->GetPointer(&pData);
	cbData = pSample->GetSize();

	cc_src_sample_t buf = buf_deque.front();
	memcpy(pData, buf.buf[0], buf.len);
	hr = pSample->SetActualDataLength(buf.len);
	hr = pSample->SetTime(&buf.start, &buf.end);
	hr = pSample->SetSyncPoint(buf.sync);
	
	buf_deque.pop_front();
	free(buf.buf);
	uv_mutex_unlock(&queue_mutex);

	/*
	CAutoLock cAutoLockShared(&m_cSharedState);
	// Access the sample's data buffer
	pSample->GetPointer(&pData);
	cbData = pSample->GetSize();

	// Check that we're still using video
	ASSERT(m_mt.formattype == FORMAT_VideoInfo);

	VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)m_mt.pbFormat;

	// Copy the DIB bits over into our filter's output buffer.
	// Since sample size may be larger than the image size, bound the copy size.
	int nSize = min(pVih->bmiHeader.biSizeImage, (DWORD)cbData);
	//HDIB hDib = CopyScreenToBitmap(&m_rScreen, pData, (BITMAPINFO *)&(pVih->bmiHeader));

	//if (hDib)
	//	DeleteObject(hDib);

	// Set the timestamps that will govern playback frame rate.
	// If this file is getting written out as an AVI,
	// then you'll also need to configure the AVI Mux filter to 
	// set the Average Time Per Frame for the AVI Header.
	// The current time is the sample's start.
	REFERENCE_TIME rtStart = m_iFrameNumber * m_rtFrameLength;
	REFERENCE_TIME rtStop = rtStart + m_rtFrameLength;

	pSample->SetTime(&rtStart, &rtStop);
	m_iFrameNumber++;

	// Set TRUE on every sample for uncompressed frames
	pSample->SetSyncPoint(TRUE);
	*/
	return S_OK;
}

HRESULT CPushPinSrc::PutBuf(cc_src_sample_t buf)
{
	//CAutoLock cAutoLockShared(&m_cSharedState);
	uv_mutex_lock(&queue_mutex);
	if (buf_deque.size() > 50){
		uv_mutex_unlock(&queue_mutex);
		free(buf.buf);
		return S_OK;
	}
	buf_deque.push_back(buf);
	uv_mutex_unlock(&queue_mutex);
	uv_cond_signal(&queue_not_empty);
	return S_OK;
}



/**********************************************
*
*  CPushSourceDesktop Class
*
**********************************************/

CPushSourceSrc::CPushSourceSrc(IUnknown *pUnk, HRESULT *phr)
: CSource(NAME("PushSourceDesktop"), pUnk, CLSID_PushSourceFilter)
{
	// The pin magically adds itself to our pin array.
	m_pPin = new CPushPinSrc(phr, this);

	if (phr)
	{
		if (m_pPin == NULL)
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}
}


CPushSourceSrc::~CPushSourceSrc()
{
	delete m_pPin;
}

HRESULT CPushSourceSrc::PutBuf(cc_src_sample_t buf)
{
	m_pPin->PutBuf(buf);
	return S_OK;
}

CBasePin* CPushSourceSrc::GetPin(void)
{
	return m_pPin;
}


CUnknown * WINAPI CPushSourceSrc::CreateInstance(IUnknown *pUnk, HRESULT *phr)
{
	CPushSourceSrc *pNewFilter = new CPushSourceSrc(pUnk, phr);

	if (phr)
	{
		if (pNewFilter == NULL)
			*phr = E_OUTOFMEMORY;
		else {
			pNewFilter->AddRef();
			*phr = S_OK;
		}
	}
	return pNewFilter;

}

class CFactoryTemplate * g_Templates = NULL;
int g_cTemplates = 0;