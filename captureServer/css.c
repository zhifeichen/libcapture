// css.cpp : 
//
#include "css.h"
#include "css_stream.h"
#include "css_protocol_package.h"
#include "css_cmd_route.h"


typedef struct css_server_s css_server_t;

struct css_server_s
{
	int index;
	uv_loop_t * loop;
	css_stream_t stream_server;
};

static css_config_t server_config_;
static css_server_t theServer_;
css_sms_info_t default_sms_ = { 0 };
uv_once_t init_sms_guard_ = UV_ONCE_INIT;

static uv_thread_t tid_;

static int css_server_init_server(css_server_t* s, int index);
static void css_server_fini_server(css_server_t* s);
static int css_server_init_config(css_config_t* c);
static void css_server_fini_config(css_config_t* c);


/* the service thread */
static void service_thread(void* arg);

static void on_data_cb(css_stream_t* stream, char* package, ssize_t status);
static void on_connection(css_stream_t* client, int status);
static void on_new_stream(css_stream_t** client);

static int css_server_init_server(css_server_t* s, int index)
{
	int ret = 0;

	s->loop = (uv_loop_t*) malloc(sizeof(uv_loop_t));
	if (!s->loop) {
		return UV_EAI_MEMORY;
	}
	s->index = index;
	ret = uv_loop_init(s->loop);
	ret = css_stream_init(s->loop, &s->stream_server);
	return ret;
}

static void css_server_fini_server(css_server_t* s)
{
	if (s->loop) {
		uv_loop_close(s->loop);
		free(s->loop);
		s->loop = NULL;
	}
}

static int css_server_init_config(css_config_t* c)
{
	int ret = 0;
	c->listen_port = 5566;
	//char* port = NULL;
	//css_get_env(CSS_DEFAULT_CONFIG_NAME, "port", CSS_DEFAULT_PORT_STR, &port);
	//c->listen_port = (uint16_t) atoi(port);
	//FREE(port);
	return ret;
}

static void css_server_fini_config(css_config_t* c)
{
}

static void on_new_stream(css_stream_t** client)
{
	*client = (css_stream_t*) malloc(sizeof(css_stream_t));
	if(*client){
		memset(*client, 0, sizeof(css_stream_t));
	}
}

static void on_connection(css_stream_t* client, int status)
{
	if (status < 0) {
		CL_ERROR("on_connection received error:%d\n", status);
		if (client) {
			free(client);
		}
		return;
	}
	CL_INFO("received conncet\n");
	css_stream_read_start(client, on_data_cb);
}

static void on_data_cb(css_stream_t* stream, char* package, ssize_t status)
{
	
	int ret;
	ret = css_cmd_dispatch(stream, package, status);
	if(ret < 0){
		CL_ERROR("css_cmd_dispatch return error!error == %d\n", ret);
	}
}

static void service_thread(void* arg)
{
	css_server_t* s = (css_server_t*) arg;
	int ret;

	ret = css_stream_bind(&s->stream_server, "0.0.0.0",
			server_config_.listen_port + s->index);
	CL_INFO("start css listen on %d.\n", server_config_.listen_port);
	ret = css_stream_listen(&s->stream_server, on_new_stream, on_connection);

	uv_run(s->loop, UV_RUN_DEFAULT);
}

int css_server_init(const char* conf_path, int index)
{
	int ret = 0;
	do {
		//if (conf_path) {
		//	ret = css_load_ini_file(conf_path);
		//} else {
		//	ret = css_load_ini_file(DEFAULT_CONFIG_FILE);
		//}
		//if (ret < 0) {
		//	CL_ERROR("css_load_ini_file service error:%d.\n", ret);
		//	break;
		//}

		ret = css_server_init_config(&server_config_);
		if (ret < 0) {
			CL_ERROR("css_server_init_config service error:%d.\n", ret);
			break;
		}

		ret = css_server_init_server(&theServer_, index);
		if (ret < 0) {
			CL_ERROR("css_server_init_server service error:%d.\n", ret);
			break;
		}

	} while (0);
	if (ret == 0) {
		CL_INFO("init all services successful.\n");
	}
	return ret;
}

int css_server_start(void)
{

	int ret = 0;
	CL_INFO("start css server.\n");

	if (ret < 0) {

		return ret;
	}

	ret = uv_thread_create(&tid_, service_thread, &theServer_);

	return ret;
}

static void walk_cb(uv_handle_t* h, void* arg)
{
	printf("active handle type: %d\n", h->type);
}

int css_server_stop(void)
{

	int ret = 0;
	CL_INFO("stop css server.\n");

	ret = css_stream_close(&theServer_.stream_server, NULL);

	uv_walk(theServer_.loop, walk_cb, NULL);
	uv_thread_join(&tid_);
	return ret;
}

void css_server_clean(void)
{
	CL_INFO("clean css server.\n");

	css_server_fini_server(&theServer_);
	css_server_fini_config(&server_config_);


}

static void css_sms_init_once(void)
{
	uv_ip4_addr("192.168.8.217", 65002, &default_sms_.smsAddr);
}

css_sms_info_t* css_server_get_default_sms(void)
{
	uv_once(&init_sms_guard_, css_sms_init_once);
	return &default_sms_;
}

