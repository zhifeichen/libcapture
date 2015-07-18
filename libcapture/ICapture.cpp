#include "stdafx.h"
#include "ICapture.h"
#include <stdio.h>


//
// Helper Macros (Jump-If-Failed, Return-If-Failed)
//
#define RELEASE(i) {if (i) i->Release(); i = NULL;}

#define JIF(x) if (FAILED(hr=(x))) \
{Msg(TEXT("FAILED(hr=0x%x) in ") TEXT(#x) TEXT("\n\0"), hr); goto CLEANUP; }

#define RIF(x) if (FAILED(hr=(x))) \
{Msg(TEXT("FAILED(hr=0x%x) in ") TEXT(#x) TEXT("\n\0"), hr); return hr; }


access_sys_t::access_sys_t(uv_loop_t* loop):
m_msgsock(NULL), p_loop(loop),
p_graph(NULL), p_capture_graph_builder2(NULL), p_control(NULL),
p_video_window(NULL), p_media_event(NULL), p_VSC(NULL),
h_wnd(NULL),
i_streams(0), i_current_stream(0),
i_width(0), i_height(0), 
i_chroma(false), b_getInterfaces(false), b_buildPreview(false), b_buildCapture(false),
b_savefile(false), b_sendsample(false),
e_psCurrent(Stopped),
m_remote(NULL)
{
	memset(&m_userinfo, 0, sizeof(cc_userinfo_t));
	memset(&p_streams, 0, sizeof(dshow_stream_t)* 2);
}

void access_sys_t::Msg(TCHAR *szFormat, ...)
{
	TCHAR str[MAX_PATH];
	va_list args;
	va_start(args, szFormat);
	vswprintf(str, szFormat, args);
	va_end(args);
	MessageBox(NULL, str, TEXT("CAPTURE"), MB_OK);
}

HRESULT access_sys_t::GetInterfaces(void)
{
	HRESULT hr = S_OK;
	IBaseFilter *pSrcFilter = NULL;

	if (b_getInterfaces){
		return hr;
	}

	// Create the filter graph
	if (!p_graph){
		hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
			IID_IGraphBuilder, (void **)&p_graph);
		if (FAILED(hr))
			return hr;
	}

	// Create the capture graph builder
	if (!p_capture_graph_builder2){
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC,
			IID_ICaptureGraphBuilder2, (void **)&p_capture_graph_builder2);
		if (FAILED(hr))
			return hr;
	}

	// Attach the filter graph to the capture graph
	hr = p_capture_graph_builder2->SetFiltergraph(p_graph);
	if (FAILED(hr))
	{
		Msg(TEXT("Failed to set capture filter graph!  hr=0x%x"), hr);
		return hr;
	}

	// Obtain interfaces for media control and Video Window
	hr = p_graph->QueryInterface(IID_IMediaControl, (LPVOID *)&p_control);
	if (FAILED(hr))
		return hr;

	hr = p_graph->QueryInterface(IID_IVideoWindow, (LPVOID *)&p_video_window);
	if (FAILED(hr))
		return hr;

	//hr = p_graph->QueryInterface(IID_IMediaEventEx, (LPVOID *)&p_media_event);
	//if (FAILED(hr))
	//	return hr;


	// Set the window handle used to process graph events
	//hr = p_media_event->SetNotifyWindow((OAHWND)ghApp, WM_GRAPHNOTIFY, 0);

	// Use the system device enumerator and class enumerator to find
	// a video capture/preview device, such as a desktop USB video camera.
	hr = FindCaptureDevice();
	if (FAILED(hr))
	{
		// Don't display a message because FindCaptureDevice will handle it
		return hr;
	}

	pSrcFilter = p_streams[0].p_device_filter;
	// Add Capture filter to our graph.
	hr = p_graph->AddFilter(pSrcFilter, L"Video Source");
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't add the capture filter to the graph!  hr=0x%x\r\n\r\n")
			TEXT("If you have a working video capture device, please make sure\r\n")
			TEXT("that it is connected and is not being used by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
		pSrcFilter->Release();
		return hr;
	}
	ULONG ref;
	ref = pSrcFilter->Release();

	pSrcFilter = p_streams[1].p_device_filter;
	// Add Capture filter to our graph.
	hr = p_graph->AddFilter(pSrcFilter, L"Audio Source");
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't add the capture filter to the graph!  hr=0x%x\r\n\r\n")
			TEXT("If you have a working video capture device, please make sure\r\n")
			TEXT("that it is connected and is not being used by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
		pSrcFilter->Release();
		return hr;
	}
	ref = pSrcFilter->Release();

	b_getInterfaces = true;
	return hr;
}

void access_sys_t::CloseInterfaces(void)
{
	// Stop previewing data
	if (p_control)
		p_control->StopWhenReady();

	e_psCurrent = Stopped;

	// Stop receiving events
	//if (p_media_event)
	//	p_media_event->SetNotifyWindow(NULL, WM_GRAPHNOTIFY, 0);

	// Relinquish ownership (IMPORTANT!) of the video window.
	// Failing to call put_Owner can lead to assert failures within
	// the video renderer, as it still assumes that it has a valid
	// parent window.
	if (p_video_window)
	{
		p_video_window->put_Visible(OAFALSE);
		p_video_window->put_Owner(NULL);
	}

#ifdef REGISTER_FILTERGRAPH
	// Remove filter graph from the running object table   
	if (g_dwGraphRegister)
		RemoveGraphFromRot(g_dwGraphRegister);
#endif

	// Release DirectShow interfaces
	SAFE_RELEASE(p_control);
	SAFE_RELEASE(p_media_event);
	SAFE_RELEASE(p_video_window);
	SAFE_RELEASE(p_graph);
	SAFE_RELEASE(p_capture_graph_builder2);

	b_buildPreview = false;
	b_getInterfaces = false;
}

HRESULT access_sys_t::FindCaptureDevice(void)
{
	HRESULT hr = S_OK;
	IBaseFilter * pSrc = NULL;
	IMoniker* pMoniker = NULL;
	ICreateDevEnum *pDevEnum = NULL;
	IEnumMoniker *pClassEnum = NULL;


	// Create the system device enumerator
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void **)&pDevEnum);
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't create system enumerator!  hr=0x%x"), hr);
	}

	// Create an enumerator for the video capture devices

	if (SUCCEEDED(hr))
	{
		hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
		if (FAILED(hr))
		{
			Msg(TEXT("Couldn't create class enumerator!  hr=0x%x"), hr);
		}
	}

	if (SUCCEEDED(hr))
	{
		// If there are no enumerators for the requested type, then 
		// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
		if (pClassEnum == NULL)
		{
			MessageBox(NULL, TEXT("No video capture device was detected.\r\n\r\n")
				TEXT("This sample requires a video capture device, such as a USB WebCam,\r\n")
				TEXT("to be installed and working properly.  The sample will now close."),
				TEXT("No Video Capture Hardware"), MB_OK | MB_ICONINFORMATION);
			hr = E_FAIL;
		}
	}

	// Use the first video capture device on the device list.
	// Note that if the Next() call succeeds but there are no monikers,
	// it will return S_FALSE (which is not a failure).  Therefore, we
	// check that the return code is S_OK instead of using SUCCEEDED() macro.

	if (SUCCEEDED(hr))
	{
		hr = pClassEnum->Next(1, &pMoniker, NULL);
		if (hr == S_FALSE)
		{
			Msg(TEXT("Unable to access video capture device!"));
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
	{
		// Bind Moniker to a filter object
		hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);
		if (FAILED(hr))
		{
			Msg(TEXT("Couldn't bind moniker to filter object!  hr=0x%x"), hr);
		}
	}

	// Copy the found filter pointer to the output parameter.
	ULONG ref;
	if (SUCCEEDED(hr))
	{
		p_streams[0].p_device_filter = pSrc;
		ref = p_streams[0].p_device_filter->AddRef();
	}

	hr = p_capture_graph_builder2->FindInterface(&PIN_CATEGORY_CAPTURE,
		&MEDIATYPE_Video, pSrc,
		IID_IAMStreamConfig, (void **)&p_VSC);
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't find IAMStreamConfig!  hr=0x%x"), hr);
	}
	else {
		AM_MEDIA_TYPE *pmt;
		//VIDEO_STREAM_CONFIG_CAPS scc;
		BYTE* scc = NULL;
		int piCount, piSize;

		hr = p_VSC->GetNumberOfCapabilities(&piCount, &piSize);
		if (hr == S_OK){
			for (int i = 0; i < piCount; i++){
				scc = new BYTE[piSize];
				hr = p_VSC->GetStreamCaps(i, &pmt, scc/*reinterpret_cast<BYTE*>(&scc)*/);
				//hr = p_VSC->GetFormat(&pmt);

				double FrameRate = 15.0;
				if (hr == NOERROR)
				{
					if (pmt->subtype == MEDIASUBTYPE_RGB24 ||
						pmt->subtype == MEDIASUBTYPE_I420 ||
						pmt->subtype == MEDIASUBTYPE_YUY2){
						if (pmt->formattype == FORMAT_VideoInfo)
						{
							//pmt->subtype = MEDIASUBTYPE_RGB24;
							VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
							if (pvi->bmiHeader.biHeight == 240 && pvi->bmiHeader.biWidth == 320){
								pvi->AvgTimePerFrame = (LONGLONG)(10000000 / FrameRate);
								//pvi->bmiHeader.biHeight = 240;
								//pvi->bmiHeader.biWidth = 320;
								hr = p_VSC->SetFormat(pmt);
								if (FAILED(hr)){
									Msg(TEXT("couldn't set video format! hr = 0x%x"), hr);
								}
								DeleteMediaType(pmt);
								delete[] scc;
								break;
							}
						}
					}
					DeleteMediaType(pmt);
				}
				delete[] scc;
			}
		}
		ref = pSrc->Release();
	}
	SAFE_RELEASE(pSrc);
	SAFE_RELEASE(pMoniker);
	SAFE_RELEASE(pClassEnum);

	// Create an enumerator for the audio capture devices

	if (SUCCEEDED(hr))
	{
		hr = pDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pClassEnum, 0);
		if (FAILED(hr))
		{
			Msg(TEXT("Couldn't create class enumerator!  hr=0x%x"), hr);
		}
	}

	if (SUCCEEDED(hr))
	{
		// If there are no enumerators for the requested type, then 
		// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
		if (pClassEnum == NULL)
		{
			MessageBox(NULL, TEXT("No audio capture device was detected.\r\n\r\n")
				TEXT("This sample requires a audio capture device\r\n")
				TEXT("to be installed and working properly.  The sample will now close."),
				TEXT("No Audio Capture Hardware"), MB_OK | MB_ICONINFORMATION);
			hr = E_FAIL;
		}
	}

	// Use the first video capture device on the device list.
	// Note that if the Next() call succeeds but there are no monikers,
	// it will return S_FALSE (which is not a failure).  Therefore, we
	// check that the return code is S_OK instead of using SUCCEEDED() macro.

	if (SUCCEEDED(hr))
	{
		hr = pClassEnum->Next(1, &pMoniker, NULL);
		if (hr == S_FALSE)
		{
			Msg(TEXT("Unable to access audio capture device!"));
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
	{
		// Bind Moniker to a filter object
		hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);
		if (FAILED(hr))
		{
			Msg(TEXT("Couldn't bind moniker to filter object!  hr=0x%x"), hr);
		}
	}

	// Copy the found filter pointer to the output parameter.
	if (SUCCEEDED(hr))
	{
		p_streams[1].p_device_filter = pSrc;
		ref = p_streams[1].p_device_filter->AddRef();
	}

	SAFE_RELEASE(pSrc);
	SAFE_RELEASE(pMoniker);
	SAFE_RELEASE(pClassEnum);

	SAFE_RELEASE(pDevEnum);

	return hr;
}

HRESULT access_sys_t::BuildPreview(void)
{
	HRESULT hr;
	IBaseFilter *pSrcFilter = NULL;

	if (b_buildPreview){
		return S_OK;
	}

	// Get DirectShow interfaces
	hr = GetInterfaces();
	if (FAILED(hr))
	{
		Msg(TEXT("Failed to get video interfaces!  hr=0x%x"), hr);
		return hr;
	}


	pSrcFilter = p_streams[0].p_device_filter;
	
	hr = p_capture_graph_builder2->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
		pSrcFilter, NULL, NULL);
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't render the video capture stream.  hr=0x%x\r\n")
			TEXT("The capture device may already be in use by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
		pSrcFilter->Release();
		return hr;
	}

	//{
	//	IEnumPins *ep;
	//	IPin *inputpin = NULL;
	//	IPin *voutputpin = NULL;
	//	IPin *aoutputpin = NULL;
	//	IPin *pin = NULL;
	//	bool bFindI420 = false;
	//	bool bFindPCM = false;

	//	pSrcFilter = p_streams[0].p_device_filter;

	//	pSrcFilter->EnumPins(&ep);
	//	if (SUCCEEDED(hr)){
	//		ep->Reset();
	//		while (SUCCEEDED(hr = ep->Next(1, &pin, 0)) && hr != S_FALSE){
	//			PIN_DIRECTION pinDir;
	//			pin->QueryDirection(&pinDir);
	//			if (pinDir == PINDIR_OUTPUT){
	//				AM_MEDIA_TYPE *pmt;
	//				IEnumMediaTypes *emt;
	//				pin->EnumMediaTypes(&emt);
	//				while (hr = emt->Next(1, &pmt, NULL), hr != S_FALSE){
	//					if (pmt->majortype == MEDIATYPE_Video){
	//						if (pmt->subtype == MEDIASUBTYPE_RGB24){
	//							//Msg(TEXT("MEDIASUBTYPE_RGB24"));
	//						}
	//						else if (pmt->subtype == MEDIASUBTYPE_I420){
	//							//Msg(TEXT("MEDIASUBTYPE_I420"));
	//							bFindI420 = true;
	//						}
	//						else if (pmt->subtype == MEDIASUBTYPE_YUY2){}
	//					}
	//					TCHAR buf[64] = { 0 };
	//					swprintf(buf, TEXT("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
	//						pmt->subtype.Data1, pmt->subtype.Data2, pmt->subtype.Data3,
	//						pmt->subtype.Data4[0], pmt->subtype.Data4[1],
	//						pmt->subtype.Data4[2], pmt->subtype.Data4[3],
	//						pmt->subtype.Data4[4], pmt->subtype.Data4[5],
	//						pmt->subtype.Data4[6], pmt->subtype.Data4[7]);
	//					//Msg(buf);
	//					DeleteMediaType(pmt);
	//				}
	//				emt->Release();
	//			}
	//			pin->Release();
	//			pin = NULL;
	//		}
	//	}
	//	RELEASE(ep);
	//}

	//pSrcFilter = p_streams[1].p_device_filter;
	//
	//hr = p_capture_graph_builder2->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio,
	//	pSrcFilter, NULL, NULL);
	//if (FAILED(hr))
	//{
	//	Msg(TEXT("Couldn't render the video capture stream.  hr=0x%x\r\n")
	//		TEXT("The capture device may already be in use by another application.\r\n\r\n")
	//		TEXT("The sample will now close."), hr);
	//	pSrcFilter->Release();
	//	return hr;
	//}

	//{
	//	IEnumPins *ep;
	//	IPin *inputpin = NULL;
	//	IPin *voutputpin = NULL;
	//	IPin *aoutputpin = NULL;
	//	IPin *pin = NULL;
	//	bool bFindI420 = false;
	//	bool bFindPCM = false;

	//	//pSrcFilter = p_streams[0].p_device_filter;

	//	pSrcFilter->EnumPins(&ep);
	//	if (SUCCEEDED(hr)){
	//		ep->Reset();
	//		while (SUCCEEDED(hr = ep->Next(1, &pin, 0)) && hr != S_FALSE){
	//			PIN_DIRECTION pinDir;
	//			pin->QueryDirection(&pinDir);
	//			if (pinDir == PINDIR_OUTPUT){
	//				AM_MEDIA_TYPE *pmt;
	//				IEnumMediaTypes *emt;
	//				pin->EnumMediaTypes(&emt);
	//				while (hr = emt->Next(1, &pmt, NULL), hr != S_FALSE){
	//					if (pmt->majortype == MEDIATYPE_Audio){
	//						if (pmt->subtype == MEDIASUBTYPE_PCM){
	//							//Msg(TEXT("MEDIASUBTYPE_PCM"));
	//						}
	//						else if (pmt->subtype == MEDIASUBTYPE_I420){
	//							//Msg(TEXT("MEDIASUBTYPE_I420"));
	//							bFindI420 = true;
	//						}
	//						else{
	//							bFindI420 = true;
	//						}
	//					}
	//					TCHAR buf[64] = { 0 };
	//					swprintf(buf, TEXT("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
	//						pmt->subtype.Data1, pmt->subtype.Data2, pmt->subtype.Data3,
	//						pmt->subtype.Data4[0], pmt->subtype.Data4[1],
	//						pmt->subtype.Data4[2], pmt->subtype.Data4[3],
	//						pmt->subtype.Data4[4], pmt->subtype.Data4[5],
	//						pmt->subtype.Data4[6], pmt->subtype.Data4[7]);
	//					//Msg(buf);
	//					DeleteMediaType(pmt);
	//				}
	//				emt->Release();
	//			}
	//			pin->Release();
	//			pin = NULL;
	//		}
	//	}
	//	RELEASE(ep);
	//}

	b_buildPreview = true;
	return hr;
}

HRESULT access_sys_t::StartPreview(HWND h)
{
	HRESULT hr;
	IBaseFilter *pSrcFilter = NULL;

	if (e_psCurrent == Running){
		StopPreview();
	}

	hr = BuildPreview();
	if (FAILED(hr)){
		return hr;
	}
	if (h){
		h_wnd = h;
	}
	//// Get DirectShow interfaces
	//hr = GetInterfaces();
	//if (FAILED(hr))
	//{
	//	Msg(TEXT("Failed to get video interfaces!  hr=0x%x"), hr);
	//	return hr;
	//}

	//// Attach the filter graph to the capture graph
	//hr = p_capture_graph_builder2->SetFiltergraph(p_graph);
	//if (FAILED(hr))
	//{
	//	Msg(TEXT("Failed to set capture filter graph!  hr=0x%x"), hr);
	//	return hr;
	//}

	//// Use the system device enumerator and class enumerator to find
	//// a video capture/preview device, such as a desktop USB video camera.
	//hr = FindCaptureDevice();
	//if (FAILED(hr))
	//{
	//	// Don't display a message because FindCaptureDevice will handle it
	//	return hr;
	//}

	//pSrcFilter = p_streams[0].p_device_filter;
	//// Add Capture filter to our graph.
	//hr = p_graph->AddFilter(pSrcFilter, L"Video Source");
	//if (FAILED(hr))
	//{
	//	Msg(TEXT("Couldn't add the capture filter to the graph!  hr=0x%x\r\n\r\n")
	//		TEXT("If you have a working video capture device, please make sure\r\n")
	//		TEXT("that it is connected and is not being used by another application.\r\n\r\n")
	//		TEXT("The sample will now close."), hr);
	//	pSrcFilter->Release();
	//	return hr;
	//}

	// Render the preview pin on the video capture filter
	// Use this instead of g_pGraph->RenderFile
	//pSrcFilter = p_streams[0].p_device_filter;
	//hr = p_capture_graph_builder2->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
	//	pSrcFilter, NULL, NULL);
	//if (FAILED(hr))
	//{
	//	Msg(TEXT("Couldn't render the video capture stream.  hr=0x%x\r\n")
	//		TEXT("The capture device may already be in use by another application.\r\n\r\n")
	//		TEXT("The sample will now close."), hr);
	//	pSrcFilter->Release();
	//	return hr;
	//}

	//// Now that the filter has been added to the graph and we have
	//// rendered its stream, we can release this reference to the filter.
	//pSrcFilter->Release();

	//pSrcFilter = p_streams[1].p_device_filter;
	//// Add Capture filter to our graph.
	//hr = p_graph->AddFilter(pSrcFilter, L"Audio Source");
	//if (FAILED(hr))
	//{
	//	Msg(TEXT("Couldn't add the capture filter to the graph!  hr=0x%x\r\n\r\n")
	//		TEXT("If you have a working video capture device, please make sure\r\n")
	//		TEXT("that it is connected and is not being used by another application.\r\n\r\n")
	//		TEXT("The sample will now close."), hr);
	//	pSrcFilter->Release();
	//	return hr;
	//}

	// Render the preview pin on the audio capture filter
	// Use this instead of g_pGraph->RenderFile
	//pSrcFilter = p_streams[1].p_device_filter;
	//hr = p_capture_graph_builder2->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio,
	//	pSrcFilter, NULL, NULL);
	//if (FAILED(hr))
	//{
	//	Msg(TEXT("Couldn't render the video capture stream.  hr=0x%x\r\n")
	//		TEXT("The capture device may already be in use by another application.\r\n\r\n")
	//		TEXT("The sample will now close."), hr);
	//	pSrcFilter->Release();
	//	return hr;
	//}

	//// Now that the filter has been added to the graph and we have
	//// rendered its stream, we can release this reference to the filter.
	//pSrcFilter->Release();

	// Set video window style and position
	hr = SetupVideoWindow(h_wnd);
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't initialize video window!  hr=0x%x"), hr);
		return hr;
	}

#ifdef REGISTER_FILTERGRAPH
	// Add our graph to the running object table, which will allow
	// the GraphEdit application to "spy" on our graph
	hr = AddGraphToRot(g_pGraph, &g_dwGraphRegister);
	if (FAILED(hr))
	{
		Msg(TEXT("Failed to register filter graph with ROT!  hr=0x%x"), hr);
		g_dwGraphRegister = 0;
	}
#endif

	// Start previewing video & audio data
	hr = p_control->Run();
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't run the graph!  hr=0x%x"), hr);
		return hr;
	}
	DWORD err = GetLastError();

	// Remember current state
	e_psCurrent = Running;
	ConnectToServer(m_userinfo.ip, m_userinfo.port);
	return S_OK;
}

HRESULT access_sys_t::StopPreview(void)
{
	HRESULT hr = S_OK;
	// Stop previewing data
	if (p_control){
		hr = p_control->StopWhenReady();
		if (hr == S_FALSE){}
	}

	e_psCurrent = Stopped;

	// Stop receiving events
	//if (p_media_event)
	//	p_media_event->SetNotifyWindow(NULL, WM_GRAPHNOTIFY, 0);

	// Relinquish ownership (IMPORTANT!) of the video window.
	// Failing to call put_Owner can lead to assert failures within
	// the video renderer, as it still assumes that it has a valid
	// parent window.
	if (p_video_window)
	{
		hr = p_video_window->put_Visible(OAFALSE);
		hr = p_video_window->put_Owner(NULL);
	}
	//h_wnd = NULL;
	return hr;
}

HRESULT access_sys_t::BuildCapture(void)
{
	HRESULT hr;
	IBaseFilter *pSrcFilter = NULL;
	IBaseFilter *pMpeg2Filter = NULL;

	if (b_buildCapture){
		return S_OK;
	}

	// Get DirectShow interfaces
	hr = GetInterfaces();
	if (FAILED(hr))
	{
		Msg(TEXT("Failed to get video interfaces!  hr=0x%x"), hr);
		return hr;
	}


	pSrcFilter = p_streams[0].p_device_filter;

	// Render the preview pin on the video capture filter
	// Use this instead of g_pGraph->RenderFile
	MyCapVideoFilter *myfilter = new MyCapVideoFilter(this, NULL, &m_lock, NULL);
	hr = p_graph->AddFilter(myfilter, L"video filter");
	hr = p_capture_graph_builder2->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
		pSrcFilter, NULL, myfilter);
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't render the video capture stream to my filter.  hr=0x%x\r\n")
			TEXT("The capture device may already be in use by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
		pSrcFilter->Release();
		return hr;
	}

	pSrcFilter = p_streams[1].p_device_filter;

	CMyCapAudioFilter *myAudiofilter = new CMyCapAudioFilter(this, NULL, &m_alock, NULL);
	hr = p_graph->AddFilter(myAudiofilter, L"audio filter");
	hr = p_capture_graph_builder2->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio,
		pSrcFilter, NULL, myAudiofilter);
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't render the audio capture stream to my audio filter.  hr=0x%x\r\n")
			TEXT("The capture device may already be in use by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
		pSrcFilter->Release();
		return hr;
	}
	return S_OK;
}


//HRESULT access_sys_t::BuildCapture(void)
//{
//	HRESULT hr;
//	IBaseFilter *pSrcFilter = NULL;
//	IBaseFilter *pMpeg2Filter = NULL;
//
//	if (b_buildCapture){
//		return S_OK;
//	}
//
//	// Get DirectShow interfaces
//	hr = GetInterfaces();
//	if (FAILED(hr))
//	{
//		Msg(TEXT("Failed to get video interfaces!  hr=0x%x"), hr);
//		return hr;
//	}
//
//	// Create the mpeg2 encode
//	hr = CoCreateInstance(CLSID_CMPEG2EncoderDS, NULL, CLSCTX_INPROC,
//		IID_IBaseFilter, (void **)&pMpeg2Filter);
//	if (FAILED(hr))
//	{
//		Msg(TEXT("Couldn't create system enumerator!  hr=0x%x"), hr);
//		if (pMpeg2Filter)
//			pMpeg2Filter->Release();
//		return hr;
//	}
//
//	hr = p_graph->AddFilter(pMpeg2Filter, L"mpeg2 filter");
//	if (FAILED(hr))
//	{
//		Msg(TEXT("Couldn't create system enumerator!  hr=0x%x"), hr);
//		pMpeg2Filter->Release();
//		return hr;
//	}
//
//	{
//		IEnumPins *ep;
//		IPin *inputpin = NULL;
//		IPin *voutputpin = NULL;
//		IPin *aoutputpin = NULL;
//		IPin *pin = NULL;
//		bool bFindI420 = false;
//		bool bFindPCM = false;
//
//		pSrcFilter = p_streams[0].p_device_filter;
//
//		pSrcFilter->EnumPins(&ep);
//		if (SUCCEEDED(hr)){
//			ep->Reset();
//			while (SUCCEEDED(hr = ep->Next(1, &pin, 0)) && hr != S_FALSE){
//				PIN_DIRECTION pinDir;
//				pin->QueryDirection(&pinDir);
//				if (pinDir == PINDIR_OUTPUT){
//					AM_MEDIA_TYPE *pmt;
//					IEnumMediaTypes *emt;
//					pin->EnumMediaTypes(&emt);
//					while (hr = emt->Next(1, &pmt, NULL), hr != S_FALSE){
//						if (pmt->majortype == MEDIATYPE_Video){
//							if (pmt->subtype == MEDIASUBTYPE_RGB24){
//								Msg(TEXT("MEDIASUBTYPE_RGB24"));
//							}
//							else if (pmt->subtype == MEDIASUBTYPE_I420){
//								Msg(TEXT("MEDIASUBTYPE_I420"));
//								bFindI420 = true;
//							}
//						}
//						DeleteMediaType(pmt);
//					}
//					emt->Release();
//				}
//				pin->Release();
//				pin = NULL;
//			}
//		}
//		RELEASE(ep);
//
//		pSrcFilter = p_streams[1].p_device_filter;
//
//		pSrcFilter->EnumPins(&ep);
//		if (SUCCEEDED(hr)){
//			ep->Reset();
//			while (SUCCEEDED(hr = ep->Next(1, &pin, 0)) && hr != S_FALSE){
//				PIN_DIRECTION pinDir;
//				pin->QueryDirection(&pinDir);
//				if (pinDir == PINDIR_OUTPUT){
//					AM_MEDIA_TYPE *pmt;
//					IEnumMediaTypes *emt;
//					pin->EnumMediaTypes(&emt);
//					while (hr = emt->Next(1, &pmt, NULL), hr != S_FALSE){
//						if (pmt->majortype == MEDIATYPE_Audio && pmt->subtype == MEDIASUBTYPE_PCM){
//							bFindPCM =  true;
//						}
//						DeleteMediaType(pmt);
//					}
//					emt->Release();
//				}
//				pin->Release();
//				pin = NULL;
//			}
//		}
//		RELEASE(ep);
//	}
//
//
//	pSrcFilter = p_streams[0].p_device_filter;
//
//	// Render the preview pin on the video capture filter
//	// Use this instead of g_pGraph->RenderFile
//	hr = p_capture_graph_builder2->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
//		pSrcFilter, NULL, pMpeg2Filter);
//	if (FAILED(hr))
//	{
//		Msg(TEXT("Couldn't render the video capture stream.  hr=0x%x\r\n")
//			TEXT("The capture device may already be in use by another application.\r\n\r\n")
//			TEXT("The sample will now close."), hr);
//		pSrcFilter->Release();
//		return hr;
//	}
//
//
//	pSrcFilter = p_streams[1].p_device_filter;
//
//	// Render the preview pin on the audio capture filter
//	// Use this instead of g_pGraph->RenderFile
//	hr = p_capture_graph_builder2->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio,
//		pSrcFilter, NULL, pMpeg2Filter);
//	if (FAILED(hr))
//	{
//		Msg(TEXT("Couldn't render the video capture stream.  hr=0x%x\r\n")
//			TEXT("The capture device may already be in use by another application.\r\n\r\n")
//			TEXT("The sample will now close."), hr);
//		pSrcFilter->Release();
//		return hr;
//	}
//
//	MyCapVideoFilter *myfilter = new MyCapVideoFilter(this, NULL, &m_lock, NULL);
//	hr = p_graph->AddFilter(myfilter, L"file filter");
//
//	// Render the preview pin on the audio capture filter
//	// Use this instead of g_pGraph->RenderFile
//	hr = p_capture_graph_builder2->RenderStream(NULL, NULL,
//		pMpeg2Filter, NULL, myfilter);
//	if (FAILED(hr))
//	{
//		Msg(TEXT("Couldn't render the video capture stream.  hr=0x%x\r\n")
//			TEXT("The capture device may already be in use by another application.\r\n\r\n")
//			TEXT("The sample will now close."), hr);
//		myfilter->Release();
//		return hr;
//	}
//
//	/* check out put media type */
//	/*
//	{
//		HRESULT hr = S_FALSE;
//		IEnumPins *ep;
//		IPin *pin;
//		AM_MEDIA_TYPE mt;
//		hr = pMpeg2Filter->EnumPins(&ep);
//		if (SUCCEEDED(hr)){
//			ep->Reset();
//			while (SUCCEEDED(hr = ep->Next(1, &pin, 0)) && hr != S_FALSE){
//				PIN_DIRECTION pinDir;
//				pin->QueryDirection(&pinDir);
//				if (pinDir == PINDIR_OUTPUT){
//					pin->ConnectionMediaType(&mt);
//					if (mt.majortype == MEDIATYPE_Stream)
//						hr = S_OK;
//					if (mt.subtype == MEDIASUBTYPE_MPEG2_PROGRAM)
//						hr = S_OK;
//					if (mt.formattype == FORMAT_None)
//						hr = S_OK;
//					FreeMediaType(mt);
//				}
//				pin->Release();
//			}
//		}
//	}
//	*/
//
//	b_buildCapture = true;
//	return hr;
//}


HRESULT access_sys_t::StartCapture(void)
{
	HRESULT hr;
	IBaseFilter *pSrcFilter = NULL;
	
 	IFilterGraph*pGraph = NULL;

	if (e_psCurrent == Running){
		StopPreview();
	}

	hr = BuildCapture();
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't BuildCapture !  hr=0x%x"), hr);
		return hr;
	}

	// Start previewing video & audio data
	if (h_wnd){
		hr = SetupVideoWindow(h_wnd);
		if (FAILED(hr))
		{
			Msg(TEXT("Couldn't initialize video window!  hr=0x%x"), hr);
			return hr;
		}
	}

	hr = p_control->Run();
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't run the graph!  hr=0x%x"), hr);
		return hr;
	}

	// Remember current state
	e_psCurrent = Running;
	m_remote = new remote_sys_t(p_loop);
	//Msg(TEXT("StartCapture success"));
	return S_OK;
}

HRESULT access_sys_t::StopCapture(void)
{
	HRESULT hr;
	// Stop previewing data
	if (p_control)
		hr = p_control->StopWhenReady();

	e_psCurrent = Stopped;

	// Stop receiving events
	//if (p_media_event)
	//	p_media_event->SetNotifyWindow(NULL, WM_GRAPHNOTIFY, 0);

	// Relinquish ownership (IMPORTANT!) of the video window.
	// Failing to call put_Owner can lead to assert failures within
	// the video renderer, as it still assumes that it has a valid
	// parent window.
	if (p_video_window)
	{
		hr = p_video_window->put_Visible(OAFALSE);
		hr = p_video_window->put_Owner(NULL);
	}
	return hr;
}

HRESULT access_sys_t::StartRemotePreview(int userid, HWND h)
{
	return m_remote->StartRemotePreview(userid, h);
}

HRESULT access_sys_t::StopRemotePreview(void)
{
	return m_remote ? m_remote->StopRemotePreview() : S_OK;
}

HRESULT access_sys_t::SetupVideoWindow(HWND h)
{
	HRESULT hr;

	// Set the video window to be a child of the main window
	hr = p_video_window->put_Owner((OAHWND)h);
	if (FAILED(hr))
		return hr;

	hr = p_video_window->put_MessageDrain((OAHWND)h);
	if (FAILED(hr))
		return hr;

	// Set video window style
	hr = p_video_window->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
	if (FAILED(hr))
		return hr;

	// Use helper function to position video window in client rect 
	// of main application window
	ResizeVideoWindow(h);

	// Make the video window visible, now that it is properly positioned
	hr = p_video_window->put_Visible(OATRUE);
	if (FAILED(hr))
		return hr;

	return hr;
}

void access_sys_t::ResizeVideoWindow(HWND h)
{
	if (p_video_window)
	{
		RECT rc;

		// Make the preview video fill our window
		GetClientRect(h_wnd, &rc);
		p_video_window->SetWindowPosition(0, 0, rc.right, rc.bottom);
	}
}

HRESULT access_sys_t::HandleGraphEvent(void)
{
	LONG evCode;
	LONG_PTR evParam1, evParam2;
	HRESULT hr = S_OK;

	if (!p_media_event)
		return E_POINTER;

	while (SUCCEEDED(p_media_event->GetEvent(&evCode, &evParam1, &evParam2, INFINITE)))
	{
		//
		// Free event parameters to prevent memory leaks associated with
		// event parameter data.  While this application is not interested
		// in the received events, applications should always process them.
		//
		hr = p_media_event->FreeEventParams(evCode, evParam1, evParam2);

		// Insert event processing code here, if desired
	}

	return hr;
}

static void work_connect(uv_work_t* req)
{}

static void after_work_connect(uv_work_t* req, int status)
{
	access_sys_t* sys = (access_sys_t*)req->data;
	sys->m_msgsock->connect_sever(sys->m_userinfo.ip, sys->m_userinfo.port);
	delete req;
}

int access_sys_t::ConnectToServer(char* ip, uint16_t port)
{
	if (m_msgsock){
		m_msgsock->dis_connect(true);
		m_msgsock = NULL;
	}
	m_msgsock = new msgsock(p_loop, this);
	if (!m_msgsock){
		return -1;
	}
    m_msgsock->set_local_user(&m_userinfo);
    m_msgsock->on_received_frame(this, do_received_frame);
	uv_work_t* req = new uv_work_t();
	req->data = this;
	int ret = uv_queue_work(p_loop, req, work_connect, after_work_connect);
	return ret;
	return m_msgsock->connect_sever(ip, port);
}

int access_sys_t::DisconnectToServer(void)
{
	if (m_msgsock){
		m_msgsock->dis_connect(true);
		m_msgsock = NULL;
	}
	return 0;
}

void access_sys_t::do_received_frame(cc_src_sample_t* frame, void* userdata)
{
    access_sys_t* sys = (access_sys_t*)userdata;
    if (sys->m_remote){
        sys->m_remote->putframe(frame);
    }
}


/* remote sys */

remote_sys_t::remote_sys_t(uv_loop_t* loop) :
h_wnd(NULL),
b_has_interface(false),
p_loop(loop)
{}

void remote_sys_t::Msg(TCHAR *szFormat, ...)
{
	TCHAR str[MAX_PATH];
	va_list args;
	va_start(args, szFormat);
	vswprintf(str, szFormat, args);
	va_end(args);
	MessageBox(NULL, str, TEXT("CAPTURE"), MB_OK);
}

void remote_sys_t::ResizeVideoWindow(HWND h)
{
	if (h_wnd)
	{
		//RECT rc;

		// Make the preview video fill our window
		//GetClientRect(h, &rc);
		// TODO resize
		vdecoder.resizewindow();
	}
}

HRESULT remote_sys_t::StartRemotePreview(int userid, HWND h)
{
	HRESULT hr = S_OK;
	if (h)
		h_wnd = h;

	vdecoder.init(p_loop, h_wnd);
	adecoder.init(p_loop);

	return hr;
}

HRESULT remote_sys_t::StopRemotePreview(void)
{
	HRESULT hr = S_OK;
	vdecoder.stop();
	adecoder.stop();
	return hr;
}

void remote_sys_t::putframe(cc_src_sample_t* frame)
{
	//decoder.put(msg->body.sample);
    if (frame->sampletype == SAMPLEVIDEO)
        vdecoder.put(frame->buf[0], frame->len);
    else if (frame->sampletype == SAMPLEAUDIO)
        adecoder.put(frame->buf[0], frame->len);
}

remote_map::remote_map() :
m_remote()
{}

remote_map::~remote_map()
{
	remove_all();
}

int remote_map::set(int userid, uv_loop_t* loop)
{
	remote_sys_t* r = get(userid);
	if (r) return 0;
	r = new remote_sys_t(loop);
	if (!r)
		return -1;
	m_remote.insert({ userid, r });
	return 0;
}

remote_sys_t* remote_map::get(int userid)
{
	remote_sys_t* r = NULL;
	rIt it = m_remote.find(userid);
	if (it == m_remote.end()){
		return r;
	}
	else {
		r = it->second;
		return r;
	}
}

int remote_map::remove(int userid)
{
	int ret = -1;
	remote_sys_t* r = NULL;
	rIt it = m_remote.find(userid);
	if (it == m_remote.end()){
		return 0;
	}
	else {
		r = it->second;
		m_remote.erase(it);
		delete r;
		return 0;
	}
}

int remote_map::remove_all(void)
{
	int ret = -1;
	for (rIt it = m_remote.begin(); it != m_remote.end(); ++it){
		delete it->second;
	}
	m_remote.clear();
	return ret;
}