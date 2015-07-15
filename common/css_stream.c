#include "css_stream.h"
#include "css_protocol_package.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void connect_cb(uv_connect_t* req, int status);
static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
static void close_cb(uv_handle_t* handle);
static void write_cb(uv_write_t* req, int status);
static void write_bufs_cb(uv_write_t* req, int status);

int css_stream_init(uv_loop_t* loop, css_stream_t* stream)
{
	int ret;

	ret = uv_tcp_init(loop, &stream->stream);
	stream->loop = loop;
	stream->stream.data = stream;
	stream->buf = NULL;
	stream->position = stream->package_len = stream->get_length = 0;
	stream->on_data = NULL;
	stream->on_connect = NULL;
	stream->on_close = NULL;

	return ret;
}

int css_stream_connect(css_stream_t* stream, const struct sockaddr* addr, css_on_connect_cb cb)
{
	int ret = 0;
	uv_connect_t *connect_req;

	connect_req = (uv_connect_t*) malloc(sizeof(uv_connect_t));
	if(!connect_req){
		return UV_EAI_MEMORY;
	}
	connect_req->data = stream;
	stream->on_connect = cb;
	ret = uv_tcp_connect(connect_req, &stream->stream, addr, connect_cb);
	if(ret){
		free(connect_req);
	}
	return ret;
}

int css_stream_read_start(css_stream_t* stream, css_on_data_cb cb)
{
	int ret = 0;

	stream->on_data = cb;
	ret = uv_read_start((uv_stream_t*)&stream->stream, alloc_cb, read_cb);
	return ret;
}

int css_stream_read_stop(css_stream_t* stream)
{
	int ret = 0;
	ret = uv_read_stop((uv_stream_t*)&stream->stream);
	return ret;
}

int css_stream_write(css_stream_t* stream, css_write_req_t* req, uv_buf_t buf, css_on_write_cb cb)
{
	int ret;

	uv_write_t* write_req;

	req->stream = stream;
	req->buf.base = buf.base;
	req->buf.len = buf.len;
	req->cb = cb;

	write_req = (uv_write_t*)malloc(sizeof(uv_write_t));
	if(write_req == NULL){
		return UV_EAI_MEMORY;
	}

	req->cb = cb;
	write_req->data = req;
	ret = uv_write(write_req, (uv_stream_t*)&req->stream->stream, &req->buf, 1, write_cb);
	if(ret){
		free(write_req);
	}
	return ret;
}

int css_stream_write_bufs(css_stream_t* stream, css_write_req_t* req,
	uv_buf_t buf[], int nbufs, css_on_write_cb cb)
{
	int ret;

	uv_write_t* write_req;

	req->stream = stream;
	req->bufs = buf;
	req->buf_cnt = nbufs;
	req->cb = cb;

	write_req = (uv_write_t*)malloc(sizeof(uv_write_t));
	if (write_req == NULL){
		return UV_EAI_MEMORY;
	}

	write_req->data = req;
	ret = uv_write(write_req, (uv_stream_t*)&req->stream->stream, req->bufs, req->buf_cnt, write_bufs_cb);
	if (ret){
		free(write_req);
	}
	return ret;
}

int css_stream_close(css_stream_t* stream, css_on_close_cb cb)
{
	int ret = 0;
	stream->on_close = cb;
	uv_close((uv_handle_t*)&stream->stream, close_cb);
	return ret;
}

static void connect_cb(uv_connect_t* req, int status)
{
	css_stream_t* stream = (css_stream_t*)req->data;
	stream->on_connect(stream, status);
	free(req);
}

static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	css_stream_t* s = (css_stream_t*)handle->data;
	if(s->get_length == CSS_STREAM_READ_HEAD){
		buf->base = ((char*)&s->package_len) + s->position;
		buf->len = 4 - s->position;
	}
	else {

		buf->base = &s->buf[s->position];
		buf->len = s->package_len - s->position;

	}
}

static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
	css_stream_t* s = (css_stream_t*)stream->data;
	if(nread < 0){
		s->on_data(s, s->buf, nread);
		s->buf = NULL;
		s->position = s->package_len = 0;
		s->get_length = CSS_STREAM_READ_HEAD;
		return ;
	}
	if(nread == 0) return ;

	if(s->buf == NULL){
		// read first 4 bytes. also (s->get_length == CSS_STREAM_READ_HEAD).
		uint32_t len;

		if(nread < 4){
			s->position += nread;
			if(s->position < 4){
				return;
			}
		}
		assert(s->position == 4 || nread == 4);		// read first 4 bytes completed.
		css_uint32_decode((char*)&s->package_len, &len);
		if(len == 0){
//			printf("fuck ! get length == 0!\n");
			//CL_ERROR("invalide packet length(0).\n");
			// error when package len == 0
			s->on_data(s, NULL, UV_EAI_MEMORY);
			s->get_length = CSS_STREAM_READ_HEAD;
			return;
		}
		s->package_len = len;
		s->get_length = CSS_STREAM_READ_BODY;

		s->buf = (char*)malloc(s->package_len);
		if(s->buf){
			css_uint32_encode(s->buf, s->package_len);
			s->position = 4;
		}
		else{
			s->on_data(s, NULL, UV_EAI_MEMORY);
			s->get_length = CSS_STREAM_READ_HEAD;
		}
	}
	else{
		s->position += nread;
		if(s->position == s->package_len){
			s->on_data(s, s->buf, s->package_len);
			s->buf = NULL;
			s->position = s->package_len = 0;
			s->get_length = CSS_STREAM_READ_HEAD;
		}
	}
}

static void write_cb(uv_write_t* req, int status)
{
	css_write_req_t* write_req = (css_write_req_t*)req->data;
	write_req->cb(write_req, status);
	free(req);
}

static void write_bufs_cb(uv_write_t* req, int status)
{
	css_write_req_t* write_req = (css_write_req_t*)req->data;
	write_req->cb(write_req, status);
	free(req);
}

static void close_cb(uv_handle_t* handle)
{
	css_stream_t* stream = (css_stream_t*)handle->data;
	if(stream->on_close){
		stream->on_close(stream);
	}
}

static void connection_cb(uv_stream_t* server, int status);

static void connection_cb(uv_stream_t* server, int status)
{
	int ret = 0;
	css_stream_t* client = NULL;
	css_stream_t* stream = (css_stream_t*)server->data;
	if(status < 0){
		stream->on_connection(NULL, status);
		return ;
	}
	stream->alloc_cb(&client);
	if(client == NULL){
		stream->on_connection(NULL, UV_EAI_MEMORY);
		return ;
	}
	ret = css_stream_init(stream->loop, client);
	if(ret < 0){
		stream->on_connection(client, ret);
		return ;
	}
	ret = uv_accept(server, (uv_stream_t*)&(client->stream));
	stream->on_connection(client, ret);
}

int css_stream_bind(css_stream_t* stream, const char* ip, uint16_t port)
{
	int ret = 0;
	struct sockaddr_in addr;
	ret = uv_ip4_addr(ip, port, &addr);
	if(ret < 0){
		return ret;
	}
	ret = uv_tcp_bind(&stream->stream, (const struct sockaddr*) &addr, 0);
	if(ret < 0){
		return ret;
	}
	return ret;
}

int css_stream_listen(css_stream_t* stream, css_alloc_stream_cb alloc_cb, css_on_connection_cb cb)
{
	int ret = 0;
	stream->alloc_cb = alloc_cb;
	stream->on_connection = cb;
	ret = uv_listen((uv_stream_t*)&stream->stream, 1024, connection_cb);
	return ret;
}

/*
 * TEST:
 */
#ifdef CSS_TEST

static void on_data_cb(css_stream_t* stream, char* package, ssize_t status);
static void on_connect_cb(css_stream_t* stream, int status);
static void on_close_cb(css_stream_t* stream);
static void on_write_cb(css_write_req_t* req, int status);

static void on_data_cb(css_stream_t* stream, char* package, ssize_t status)
{
	JNetCmd_Header header;

	if(status <= 0){
		if(package) free(package);
		css_stream_read_stop(stream);
		css_stream_close(stream, on_close_cb);
		return ;
	}

	css_proto_package_header_decode(package, status, &header);
	free(package);
	printf("get %u size data\n", header.I_CmdLen);
}

static void on_connect_cb(css_stream_t* stream, int status)
{
	JNetCmd_PreviewConnect_Ex msg;
	uv_buf_t buf;
	size_t len;
	css_write_req_t* req;

	if(status){
		printf("connect error! error no.: %d\n", status);
		css_stream_close(stream, on_close_cb);
		return ;
	}

	JNetCmd_PreviewConnect_Ex_init(&msg);
	msg.EquipId = 1ull;
	msg.ChannelNo = 1;
	msg.CodingFlowType = 0;

	len = css_proto_package_calculate_buf_len((JNetCmd_Header *)&msg);
	buf.base = (char*)malloc(len);
	if(!buf.base){
		css_stream_close(stream, on_close_cb);
		return ;
	}
	buf.len = len;
	css_proto_package_encode(buf.base, len, (JNetCmd_Header *)&msg);
	req = (css_write_req_t*)malloc(sizeof(css_write_req_t));
	if(!req){
		free(buf.base);
		css_stream_close(stream, on_close_cb);
		return ;
	}
	/*
	req->stream = stream;
	req->buf.base = buf;
	req->buf.len = len;
	req->cb = on_write_cb;
	*/
	css_stream_write(stream, req, buf, on_write_cb);
}

static void on_close_cb(css_stream_t* stream)
{
}

static void on_write_cb(css_write_req_t* req, int status)
{
	css_stream_t* stream = req->stream;

	free(req->buf.base);
	free(req);

	if(status){
		css_stream_close(stream, on_close_cb);
		return ;
	}

	css_stream_read_start(stream, on_data_cb);
}

void test_stream(void)
{
	uv_loop_t* loop = uv_default_loop();
	css_stream_t stream;
	struct sockaddr_in addr;

	uv_ip4_addr("127.0.0.1", 65002, &addr);
	css_stream_init(loop, &stream);
	css_stream_connect(&stream, (const struct sockaddr*)&addr, on_connect_cb);
	uv_run(loop, UV_RUN_DEFAULT);
}

css_stream_t stream_client, stream_server;

static void new_stream(css_stream_t** client)
{
	*client = &stream_client;
}

static void on_connection(css_stream_t* client, int status)
{
	if(status < 0){
		printf("conncetion error: %d\n", status);
		return ;
	}
	css_stream_read_start(client, on_data_cb);
}

void test_stream_server(void)
{
	uv_loop_t* loop = uv_default_loop();
	uv_once_t guard  = UV_ONCE_INIT;
	css_stream_init(loop, &stream_server);
	css_stream_bind(&stream_server, "0.0.0.0", 65002);
	css_stream_listen(&stream_server, new_stream, on_connection);

	uv_once(&guard, test_stream);
	uv_run(loop, UV_RUN_DEFAULT);
}

#endif
