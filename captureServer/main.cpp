#include "uv/uv.h"

#include <stdio.h>
#include <stdlib.h>

#define ASSERT(x) x

#define container_of(ptr, type, member) \
	((type *)((char *)(ptr)-offsetof(type, member)))

#define TEST_PORT 5566

typedef struct {
	uv_tcp_t handle;
	uv_shutdown_t shutdown_req;
	int volid;
	int need_free;
	int index;
} conn_rec;

static uv_tcp_t tcp_server;

static conn_rec client[2] = { 0 };

static void connection_cb(uv_stream_t* stream, int status);
static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
static void shutdown_cb(uv_shutdown_t* req, int status);
static void close_cb(uv_handle_t* handle);


static void connection_cb(uv_stream_t* stream, int status) {
	conn_rec* conn;
	int r;

	ASSERT(status == 0);
	ASSERT(stream == (uv_stream_t*)&tcp_server);

	printf("received connection\n");
	if (client[0].volid == 0){
		conn = &client[0];
		conn->index = 0;
	}
	else if(client[1].volid == 0){
		conn = &client[1];
		conn->index = 1;
	}
	else{
		printf("already have two client connected!\n");
		conn = (conn_rec*)malloc(sizeof *conn);
		ASSERT(conn != NULL);
		conn->need_free = 1;
	}

	r = uv_tcp_init(stream->loop, &conn->handle);
	ASSERT(r == 0);

	r = uv_accept(stream, (uv_stream_t*)&conn->handle);
	ASSERT(r == 0);

	if (conn != &client[0] && conn != &client[1]){
		r = uv_shutdown(&conn->shutdown_req, (uv_stream_t*)&conn->handle, shutdown_cb);
		return;
	}
	conn->volid = 1;
	r = uv_read_start((uv_stream_t*)&conn->handle, alloc_cb, read_cb);
	ASSERT(r == 0);
}


static void alloc_cb(uv_handle_t* handle,
	size_t suggested_size,
	uv_buf_t* buf) {
	//static char slab[65536];
	buf->base = new char[1024*100];
	buf->len = 1024*100;
}


static void write_cb(uv_write_t* req, int stat)
{
	if (stat < 0){
		printf("write cb error: %d\n", stat);
	}
	delete[] req->data;
}


static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	conn_rec* conn;
	int r;
	if (nread <= 0){
		printf("read cb: %d\n", nread);
	}
	if (nread > 0){
		conn_rec* conn2;
		int index;
		conn = container_of(stream, conn_rec, handle);
		index = (conn->index + 1) & 1;
		conn2 = &client[index];
		if (conn2->volid == 0){
			delete[] buf->base;
			return;
		}

		uv_write_t *req = (uv_write_t *)malloc(sizeof uv_write_t);
		req->data = buf->base;
		uv_buf_t sb;
		sb.len = nread;
		sb.base = buf->base;
		r = uv_write(req, (uv_stream_t*)&conn2->handle, &sb, 1, write_cb);
		if (r < 0){
			printf("write error: %d\n", r);
			delete[] buf->base;
		}
		else{
			return;
		}
	}
	else if (nread == 0){
		return;
	}

	ASSERT(nread == UV_EOF);

	conn = container_of(stream, conn_rec, handle);

	r = uv_shutdown(&conn->shutdown_req, stream, shutdown_cb);
	ASSERT(r == 0);
}


static void shutdown_cb(uv_shutdown_t* req, int status) {
	conn_rec* conn = container_of(req, conn_rec, shutdown_req);
	uv_close((uv_handle_t*)&conn->handle, close_cb);
}


static void close_cb(uv_handle_t* handle) {
	conn_rec* conn = container_of(handle, conn_rec, handle);
	if (conn->need_free)
		free(conn);
	else
		memset(conn, 0, sizeof(conn_rec));
}


int main_old(void) {
	struct sockaddr_in addr;
	uv_loop_t* loop;
	int r;

	loop = uv_default_loop();
	ASSERT(0 == uv_ip4_addr("0.0.0.0", TEST_PORT, &addr));

	r = uv_tcp_init(loop, &tcp_server);
	ASSERT(r == 0);

	r = uv_tcp_bind(&tcp_server, (const struct sockaddr*) &addr, 0);
	ASSERT(r == 0);

	r = uv_listen((uv_stream_t*)&tcp_server, 128, connection_cb);
	ASSERT(r == 0);

	r = uv_run(loop, UV_RUN_DEFAULT);
	ASSERT(0 && "Blackhole server dropped out of event loop.");

	return 0;
}
extern "C" {
#include "css.h"
}
void signal_handler(uv_signal_t *handle, int signum)
{
	printf("Signal received: %d\n", signum);
	uv_signal_stop(handle);
	uv_stop(handle->loop);
}

int main(int argc, char* argv[])
{
	uv_loop_t loop;
	uv_signal_t sig_int;
	int ret = 0;
	char* config_file = NULL;
	int index = 0;

	uv_loop_init(&loop);
	uv_signal_init(&loop, &sig_int);
	uv_signal_start(&sig_int, signal_handler, SIGINT);
	if (argc > 1){
		index = atoi(argv[1]);
	}
	if (argc > 2) {
		config_file = argv[2];
	}
	ret = css_server_init(config_file, index);
	if (ret < 0) {
		return ret;
	}
	ret = css_server_start();
	uv_run(&loop, UV_RUN_DEFAULT);
	css_server_stop();
	css_server_clean();
	return 0;
}