// run_test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "css_test.h"


#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "winmm.lib")
#ifdef _DEBUG
#pragma comment(lib, "strmbasd.lib")
#else
#pragma comment(lib, "strmbase.lib")
#endif
#pragma comment(lib, "libx264.dll.a")
#pragma comment(lib, "uv\\libuv.lib")
#pragma comment(lib, "ffmpeg\\avutil.lib")
#pragma comment(lib, "ffmpeg\\avformat.lib")
#pragma comment(lib, "ffmpeg\\avcodec.lib")
#pragma comment(lib, "ffmpeg\\swscale.lib")
#pragma comment(lib, "ffmpeg\\swresample.lib")
#pragma comment(lib, "sdl\\x86\\sdl2.lib")

int _tmain(int argc, _TCHAR* argv[])
{
	css_do_test_all();
    printf("test end, press any key exit...");
    getchar();
	return 0;
}

