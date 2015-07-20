#ifndef __IFILTER_H__
#define __IFILTER_H__

#include <dshow.h>
#include "Streams.h"
#include <deque>
#include <Dvdmedia.h>

extern "C" {
#include "uv/uv.h"
#include <libswscale/swscale.h>
}

#include "x264encode.h"
#include "aacencode.h"

// {D06A31CB-3518-4054-A167-65CE67D7A931}
DEFINE_GUID(CLSID_MyCapVideoFilter,
	0xd06a31cb, 0x3518, 0x4054, 0xa1, 0x67, 0x65, 0xce, 0x67, 0xd7, 0xa9, 0x31);

// {25A7F004-9B2D-4B77-AEFC-08B0084F2D4F}
DEFINE_GUID(CLSID_CMyCapAudioFilter,
	0x25a7f004, 0x9b2d, 0x4b77, 0xae, 0xfc, 0x08, 0xb0, 0x08, 0x4f, 0x2d, 0x4f);


// {7D732DC9-5770-4D20-9090-14429900992C}
DEFINE_GUID(CLSID_PushSourceFilter,
	0x7d732dc9, 0x5770, 0x4d20, 0x90, 0x90, 0x14, 0x42, 0x99, 0x0, 0x99, 0x2c);

typedef void(*RawFrameCallBack)(cc_src_sample_t*, void* userdata);

class CAccessSys;

class CMyCapVideoFilter;

//  Pin object
class CMyCapVideoInputPin : public CRenderedInputPin
{
	//CDump    * const m_pDump;           // Main renderer object
	//CCritSec * const m_pReceiveLock;    // Sample critical section
	REFERENCE_TIME m_tLast;             // Last sample receive time
	CMyCapVideoFilter* m_pfilter;
	CMediaType m_mt;
	enum AVPixelFormat m_pix_fmt;
	int m_width, m_height;
	SwsContext *m_pSwsCtx;

	int ConvertFmt(const CMediaType *);
	
public:

	CMyCapVideoInputPin(CMyCapVideoFilter* pFilter);

	// Do something with this media sample
	STDMETHODIMP Receive(IMediaSample *pSample);
	STDMETHODIMP EndOfStream(void);
	STDMETHODIMP ReceiveCanBlock();

	// Write detailed information about this sample to a file
	HRESULT WriteStringInfo(IMediaSample *pSample);

	// Check if the pin can support this specific proposed type and format
	HRESULT CheckMediaType(const CMediaType *);

	void ConvertRGB(uint8_t* dst, uint8_t* src);

	// Break connection
	HRESULT BreakConnect();

	// Track NewSegment
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart,
		REFERENCE_TIME tStop,
		double dRate);
};

class CMyCapVideoFilter : public CBaseFilter
{
	friend class CMyCapVideoInputPin;
	CMyCapVideoInputPin* m_pPin;
	CCritSec *m_pLock;
	HRESULT m_hr;
    RawFrameCallBack m_cb;
    void* m_userdata;

public:

	// Constructor
	CMyCapVideoFilter(CAccessSys *psys,
		LPUNKNOWN pUnk,
		CCritSec *pLock,
		HRESULT *phr);

	// Pin enumeration
	CBasePin * GetPin(int n);
	int GetPinCount();

	// Open and close the file as necessary
	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

    void SetRawFrameCallBack(RawFrameCallBack cb, void* mdata);

private:
	//HANDLE m_hFile;
	// Open and write to the file
	//HRESULT OpenFile();
	//HRESULT CloseFile();
	//HRESULT Write(PBYTE pbData, LONG lDataLength);

	// x264 encode call back
	static void enc_cb(cc_src_sample_t smple, void* data);
	void enc_cb(cc_src_sample_t smple);
};

//  Pin object
class CMyCapAudioFilter;

class CMyCapAudioInputPin : public CRenderedInputPin
{
	//CDump    * const m_pDump;           // Main renderer object
	//CCritSec * const m_pReceiveLock;    // Sample critical section
	REFERENCE_TIME m_tLast;             // Last sample receive time
	CMyCapAudioFilter* m_pfilter;
	CMediaType m_mt;
	enum AVPixelFormat m_pix_fmt;
	int m_width, m_height;
	SwsContext *m_pSwsCtx;

	int ConvertFmt(const CMediaType *);

public:

	CMyCapAudioInputPin(CMyCapAudioFilter* pFilter);

	// Do something with this media sample
	STDMETHODIMP Receive(IMediaSample *pSample);
	STDMETHODIMP EndOfStream(void);
	STDMETHODIMP ReceiveCanBlock();

	const CMediaType * GetMediaType(void);
	// Write detailed information about this sample to a file
	HRESULT WriteStringInfo(IMediaSample *pSample);

	// Check if the pin can support this specific proposed type and format
	HRESULT CheckMediaType(const CMediaType *);

	// Break connection
	HRESULT BreakConnect();

	// Track NewSegment
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart,
		REFERENCE_TIME tStop,
		double dRate);
};

class CMyCapAudioFilter : public CBaseFilter
{
	friend class CMyCapAudioInputPin;
	CMyCapAudioInputPin* m_pPin;
	CCritSec *m_pLock;
	HRESULT m_hr;
    RawFrameCallBack m_cb;
    void* m_userdata;

public:

	// Constructor
	CMyCapAudioFilter(CAccessSys *psys,
		LPUNKNOWN pUnk,
		CCritSec *pLock,
		HRESULT *phr);

	// Pin enumeration
	CBasePin * GetPin(int n);
	int GetPinCount();

	// Open and close the file as necessary
	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

    void SetRawFrameCallBack(RawFrameCallBack cb, void* data);

private:

	// aac encode call back
	static void enc_cb(cc_src_sample_t smple, void* data);
	void enc_cb(cc_src_sample_t smple);
};


/* out put pin */
class CPushPinSrc : public CSourceStream
{
protected:

	int m_FramesWritten;				// To track where we are in the file
	BOOL m_bZeroMemory;                 // Do we need to clear the buffer?
	CRefTime m_rtSampleTime;	        // The time stamp for each sample

	BITMAPINFO *m_pBmi;                 // Pointer to the bitmap header
	DWORD       m_cbBitmapInfo;         // Size of the bitmap header

	// File opening variables 
	HANDLE m_hFile;                     // Handle returned from CreateFile
	BYTE * m_pFile;                     // Points to beginning of file buffer
	BYTE * m_pImage;                    // Points to pixel bits       

	int m_iImageHeight;                 // The current image height
	int m_iImageWidth;                  // And current image width

	int m_iFrameNumber;
	//const REFERENCE_TIME m_rtFrameLength;

	CCritSec m_cSharedState;            // Protects our internal state
	std::deque<cc_src_sample_t> buf_deque;

	uv_mutex_t queue_mutex;
	uv_cond_t queue_not_empty;

public:

	CPushPinSrc(HRESULT *phr, CSource *pFilter);
	~CPushPinSrc();

	// Override the version that offers exactly one media type
	HRESULT SetMediaType(const CMediaType *pMediaType);
	HRESULT CheckMediaType(const CMediaType *pMediaType);
	HRESULT GetMediaType(CMediaType *pMediaType);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
	HRESULT FillBuffer(IMediaSample *pSample);

	HRESULT PutBuf(cc_src_sample_t buf);
	// Quality control
	// Not implemented because we aren't going in real time.
	// If the file-writing filter slows the graph down, we just do nothing, which means
	// wait until we're unblocked. No frames are ever dropped.
	STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
	{
		return E_FAIL;
	}

};

/* out put filter */
class CPushSourceSrc : public CSource
{

private:
	// Constructor is private because you have to use CreateInstance
	CPushSourceSrc(IUnknown *pUnk, HRESULT *phr);
	~CPushSourceSrc();

	CPushPinSrc *m_pPin;

public:
	static CUnknown * WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);

public:
	HRESULT PutBuf(cc_src_sample_t buf);
	CBasePin* GetPin(void);

};
#endif //__IFILTER_H__