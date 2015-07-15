#ifndef __MSGSOCK_H__
#define __MSGSOCK_H__
#include "uv/uv.h"
#include <dshow.h>
#include "libcapture.h"
#include "css_util.h"
#include "css_stream.h"


class msgsock
{
	uv_loop_t		   *loop_;
	css_stream_t       *stream_;
	struct sockaddr_in	server_addr_;
    cc_userinfo_t       local_user_;
	
	bool				received_info_;
	bool				sended_start_;
	bool				delete_when_close_;
	
	void(*receive_cb_)(MyMSG* msg, void* userdata);
	void* msgcb_userdata_;
    void(*receive_frame_cb_)(cc_src_sample_t* frame, void* userdata);
    void* framecb_userdata_;
private:
	static void conn_cb(css_stream_t* stream, int status);
	static void close_cb(css_stream_t* stream);
	static void read_cb(css_stream_t* stream, char* package, ssize_t status);
	//static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	//static void sendsample_cb(css_write_req_t* req, int status);
	static void send_cb(css_write_req_t* req, int status);
	int send_user_info(void);
	int send_start_frame(void);
	void do_preview_frame(css_stream_t* stream, char* package, ssize_t status);
	void do_user_login(css_stream_t* stream, char* package, ssize_t status);
	void do_user_logout(css_stream_t* stream, char* package, ssize_t status);
	void do_start_send_frame(css_stream_t* stream, char* package, ssize_t status);
	enum {uninit, init, connected, close} stat;
public:
	msgsock(uv_loop_t* loop, struct access_sys_t* accs);
	~msgsock(void);

    int get_stat(void){ return stat; }
    int set_local_user(cc_userinfo_t* user);
	int set_loop(uv_loop_t* loop);

	int connect_sever(char* ip, uint16_t port);
	int dis_connect(bool bDelete = false);
	int send(uint8_t* buf, uint32_t len);
	//int send(IMediaSample* sample);
	int on_received(void* userdata, void(*cb)(MyMSG*, void* userdata));
    int on_received_frame(void* userdata, void(*cb)(cc_src_sample_t*, void* userdata));
};

#endif //__MSGSOCK_H__