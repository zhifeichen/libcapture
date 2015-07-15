#ifndef __CSS_STREAM_H__
#define __CSS_STREAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "uv/uv.h"

typedef struct css_stream_s css_stream_t;
typedef struct css_write_req_s css_write_req_t;

typedef void (*css_on_data_cb)(css_stream_t* stream, char* package, ssize_t status);
typedef void (*css_on_connect_cb)(css_stream_t* stream, int status);
typedef void (*css_on_close_cb)(css_stream_t* stream);
typedef void (*css_on_write_cb)(css_write_req_t* req, int status);

typedef void (*css_alloc_stream_cb)(css_stream_t** client);
typedef void (*css_on_connection_cb)(css_stream_t* client, int status);

struct css_write_req_s
{
	css_stream_t* stream;
	css_on_write_cb cb;
	uv_buf_t buf;
	uv_buf_t* bufs;
	int buf_cnt;
};

typedef enum
{
	CSS_STREAM_READ_HEAD = 0,
	CSS_STREAM_READ_BODY
} css_stream_read_flag;

struct css_stream_s
{
	void* data;
	uv_tcp_t stream;
	uv_loop_t* loop;
	char* buf;
	uint32_t position;
	uint32_t package_len;
	css_stream_read_flag get_length;
	css_on_data_cb on_data;
	css_on_connect_cb on_connect;
	css_on_close_cb on_close;
	css_on_write_cb on_write;

	css_alloc_stream_cb alloc_cb;
	css_on_connection_cb on_connection;
};

/*
 * function:
 */
int css_stream_init(uv_loop_t* loop, css_stream_t* stream);
int css_stream_connect(css_stream_t* stream, const struct sockaddr* addr,
		css_on_connect_cb cb);
int css_stream_read_start(css_stream_t* stream, css_on_data_cb cb);
int css_stream_read_stop(css_stream_t* stream);
int css_stream_write(css_stream_t* stream, css_write_req_t* req, uv_buf_t buf,
		css_on_write_cb cb);
int css_stream_write_bufs(css_stream_t* stream, css_write_req_t* req, 
		uv_buf_t buf[], int nbufs, css_on_write_cb cb);
int css_stream_close(css_stream_t* stream, css_on_close_cb cb);

int css_stream_bind(css_stream_t* stream, const char* ip, uint16_t port);
int css_stream_listen(css_stream_t* stream, css_alloc_stream_cb alloc_cb,
		css_on_connection_cb connection_cb);

#ifdef __cplusplus
}
#endif

#endif /* __CSS_STREAM_H__ */
