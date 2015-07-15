// libcapture.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "libcapture.h"
#include "ICapture.h"
#include "uv/uv.h"
#include "msgsock.h"
#include "css_logger.h"

static uv_loop_t g_loop;
static uv_timer_t g_timer;
static uv_thread_t g_loop_thread;
static uv_async_t g_async;

static access_sys_t* g_sys = NULL;
//static remote_sys_t* g_remote;
//static msgsock g_sock(&g_loop);

static void loop_thread(void* arg)
{
	uv_run(&g_loop, UV_RUN_DEFAULT);
}

static void timer_fn(uv_timer_t* timer)
{}

static int uv_init(void)
{
	int ret = 0;
	ret = uv_loop_init(&g_loop);
	ret = uv_timer_init(&g_loop, &g_timer);
	//uv_ref((uv_handle_t*)&g_timer);
	ret = uv_timer_start(&g_timer, timer_fn, 10, 10);
	ret = uv_thread_create(&g_loop_thread, loop_thread, NULL);
	css_logger_init(&g_loop);
	g_sys = new access_sys_t(&g_loop);
	//g_remote = new remote_sys_t(&g_loop);
	//g_sys->m_msgsock.set_loop(&g_loop);
	return ret;
}

static void uv_finit(void)
{
	//uv_unref((uv_handle_t*)&g_timer);
	uv_timer_stop(&g_timer);
	uv_stop(&g_loop);
	uv_async_init(&g_loop, &g_async, NULL);
	uv_async_send(&g_async);
	uv_thread_join(&g_loop_thread);
	if (g_sys) delete g_sys;
	//if (g_remote) delete g_remote;
}

LIBCAPTURE_API	int init(void)
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	uv_init();
	return 0;
}

LIBCAPTURE_API  int finit(void)
{
	uv_finit();
	CoUninitialize();
	return 0;
}

LIBCAPTURE_API  int startpreview(HWND hShow, int volume, cc_userinfo_t* info, void* userdata, FNCOMMCALLBACK cb)
{
	int ret;
	g_sys->m_userinfo.userid = info->userid;
	g_sys->m_userinfo.roomid = info->roomid;
	g_sys->m_userinfo.port = info->port;
	memcpy(g_sys->m_userinfo.ip, info->ip, 16);
	//strcpy(g_sys->m_userinfo.ip, info->ip);
	g_sys->StartPreview(hShow);
	//ret = g_sys->ConnectToServer(info->ip, info->port);
	return 0;
}

LIBCAPTURE_API  int resizewindow(void)
{
	g_sys->ResizeVideoWindow(NULL);
	return 0;
}

LIBCAPTURE_API  int stoppreview(void* userdata, FNCOMMCALLBACK cb)
{
	g_sys->StopPreview();
	g_sys->CloseInterfaces();
	return 0;
}

LIBCAPTURE_API  int startremotepreview(int userid, HWND hShow, void* userdata, FNCOMMCALLBACK cb)
{
	//g_sys.m_msgsock.connect_sever("120.199.202.36", 5566);
	//g_sys.m_msgsock.connect_sever("120.199.202.36", 5566);
	g_sys->b_sendsample = true;

	g_sys->StartRemotePreview(userid, hShow);
	return 0;
}

LIBCAPTURE_API int resizeremotepreview(int userid, HWND hShow, void* userdata, FNCOMMCALLBACK cb)
{
	if (g_sys->m_remote)
		g_sys->m_remote->ResizeVideoWindow(hShow);
	return 0;
}

LIBCAPTURE_API  int stopremotepreview(int userid, void* userdata, FNCOMMCALLBACK cb)
{
	g_sys->b_sendsample = false;
	g_sys->StopRemotePreview();
	g_sys->DisconnectToServer();
	return 0;
}

LIBCAPTURE_API  int startsendsample(void* userdata, FNCOMMCALLBACK cb)
{
	g_sys->b_sendsample = true;
	return 0;
}

LIBCAPTURE_API  int stopsendsample(void* userdata, FNCOMMCALLBACK cb)
{
	g_sys->b_sendsample = false;
	return 0;
}

LIBCAPTURE_API  int startcapture(char* filename, void* userdata, FNCOMMCALLBACK cb)
{
	g_sys->StartCapture();
	return 0;
}

LIBCAPTURE_API  int stopcaptrue(void* userdata, FNCOMMCALLBACK cb)
{
	g_sys->StopCapture();
	return 0;
}

LIBCAPTURE_API  int setmsgcallback(void* userdata, FNMSGCALLBACK cb)
{
	if (g_sys->m_msgsock)
		g_sys->m_msgsock->on_received(userdata, cb);
	return 0;
}

