#ifndef __LIBCAPTURE_H__
#define __LIBCAPTURE_H__

#ifdef LIBCAPTURE_EXPORTS
#define LIBCAPTURE_API __declspec(dllexport)
#else
#define LIBCAPTURE_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <Windows.h>


#define SAMPLEVIDEO 0
#define	SAMPLEAUDIO 1

#define CODE_CONNECTED		1
#define CODE_CLOSED			2
#define CODE_NEWUSER		4
#define CODE_USERLOGOUT		8
#define CODE_SAMPLE_VIDEO	16
#define CODE_SAMPLE_AUDIO	32
#define CODE_SOCKETERROR	64

	typedef struct cc_src_sample_s
	{
        int sampletype;
		LONGLONG start;
		LONGLONG end;
		int sync;
		int len;
		uint8_t* buf[8];
		int line[8];
		int iplan;
	}cc_src_sample_t;

	typedef struct cc_userinfo_s
	{
		int userid;
		int roomid;
		char ip[16];
		unsigned short port;
	}cc_userinfo_t;

	typedef struct MyMSG
	{
		int32_t code;
		int32_t bodylen;
		union{
			cc_src_sample_t sample;
			cc_userinfo_t userinfo;
			unsigned char* buf;
		}body;
	}MyMSG;

	typedef void(*FNMSGCALLBACK)(MyMSG* msg, void* userdata);
	typedef void(*FNCOMMCALLBACK)(int stat, void* userdata);

	LIBCAPTURE_API	int init(void);
	LIBCAPTURE_API  int finit(void);
	LIBCAPTURE_API  int startpreview(HWND hShow, int volume, cc_userinfo_t* info, void* userdata, FNCOMMCALLBACK cb);
	LIBCAPTURE_API  int resizewindow(void);
	LIBCAPTURE_API  int stoppreview(void* userdata, FNCOMMCALLBACK cb);
	LIBCAPTURE_API  int startremotepreview(int userid, HWND hShow, void* userdata, FNCOMMCALLBACK cb);
	LIBCAPTURE_API  int resizeremotepreview(int userid, HWND hShow, void* userdata, FNCOMMCALLBACK cb);
	LIBCAPTURE_API  int stopremotepreview(int userid, void* userdata, FNCOMMCALLBACK cb);
	LIBCAPTURE_API  int startsendsample(void* userdata, FNCOMMCALLBACK cb);
	LIBCAPTURE_API  int stopsendsample(void* userdata, FNCOMMCALLBACK cb);
	LIBCAPTURE_API  int startcapture(char* filename, void* userdata, FNCOMMCALLBACK cb);
	LIBCAPTURE_API  int stopcaptrue(void* userdata, FNCOMMCALLBACK cb);
	LIBCAPTURE_API  int setmsgcallback(void* userdata, FNMSGCALLBACK cb);

#ifdef __cplusplus
}
#endif

#endif //__LIBCAPTURE_H__