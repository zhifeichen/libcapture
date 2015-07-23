#include "css.h"
#include "css_test.h"
#include "CaptureSys.h"

#define CHECKGOTO(X) if((X) < 0){goto _clean;}

static int cnt = 0;
static CCaptureSys *s1 = NULL, *s2 = NULL;

static void walk_cb(uv_handle_t* h, void* arg)
{
	printf("active handle type: %d\n", h->type);
}

static void do_nothing(uv_work_t* req)
{
	::Sleep(1000);
}

static void reconn(uv_work_t* req, int status)
{
	CCaptureSys* s = (CCaptureSys*)req->data;
	//printf("recconn s stat: %d\n", s->get_stat());
	int ret = s->ConnectToServer("127.0.0.1", 5566);
	printf("reconn return %d\n", ret);
	free(req);
}

static void do_newuser(MyMSG* msg, CCaptureSys* s)
{
	printf("received new user: id: %d; room: %d\n", msg->body.userinfo.userid, msg->body.userinfo.roomid);
	s->DisconnectToServer();
	cnt++;
	if (cnt >= 2){
		uv_walk(uv_default_loop(), walk_cb, NULL);
		printf("alive handle: %d\n", uv_loop_alive(uv_default_loop()));
		s1->DisconnectToServer();
		s2->DisconnectToServer();
		return;
	}
	uv_work_t* req = (uv_work_t*)malloc(sizeof(uv_work_t));
	req->data = s;
	int ret = uv_queue_work(uv_default_loop(), req, do_nothing, reconn);
	printf("queue reconn return %d\n", ret);
}

static void do_userlogout(MyMSG* msg, CCaptureSys* s)
{
	printf("received user %d logout\n", msg->body.userinfo.userid);
	printf("alive handle: %d\n", uv_loop_alive(uv_default_loop()));
}

static void do_socket_error(MyMSG* msg, CCaptureSys* s)
{
	printf("received socket error\n");
}

static void on_msg(MyMSG* msg, void* userdata)
{
	CCaptureSys* s = (CCaptureSys*)userdata;
	switch (msg->code){
	case CODE_NEWUSER:
		do_newuser(msg, s);
		break;
	case CODE_USERLOGOUT:
		do_userlogout(msg, s);
		break;
	case CODE_SOCKETERROR:
		do_socket_error(msg, s);
		break;
	default:
		break;
	}
}

static void on_frame(int userid, cc_src_sample_t* frame, void* userdata)
{}

TEST_IMPL(CaptureSysMsg)
{
	int ret = -1;
	uv_loop_t* loop = uv_default_loop();
	if (!loop) return;
	cc_userinfo_t userinfo = { 0 };
	cc_userinfo_t userinfo2 = { 0 };

	userinfo.port = 5566;
	userinfo.roomid = 24;
	userinfo.userid = 103;
	memcpy(userinfo.ip, "127.0.0.1", 16);

	userinfo2.port = 5566;
	userinfo2.roomid = 24;
	userinfo2.userid = 104;
	memcpy(userinfo2.ip, "127.0.0.1", 16);

	CHECKGOTO(ret = css_server_init(NULL, 0));
	CHECKGOTO(ret = css_server_start());

	s1 = new CCaptureSys(loop);
	s1->SetUserInfo(&userinfo);
	s1->SetReceivedMsgCb(s1, on_msg);
	//s->on_received_frame(s, on_frame);

	s2 = new CCaptureSys(loop);
	s2->SetUserInfo(&userinfo2);
	s2->SetReceivedMsgCb(s2, on_msg);
	//s2->on_received_frame(s2, on_frame);

	ret = s1->ConnectToServer("127.0.0.1", 5566);
	printf("connect_sever return %d\n", ret);
	ret = s2->ConnectToServer("127.0.0.1", 5566);
	printf("connect_sever return %d\n", ret);
	//printf("s stat: %d\n", s->get_stat());
	//printf("s2 stat: %d\n", s2->get_stat());
	uv_run(loop, UV_RUN_DEFAULT);
	printf("stop!\n");
	printf("Resource count: %d\n", CResourcePool::GetInstance().GetResourceCount());
_clean:
	css_server_stop();
	css_server_clean();
	delete s1;
	delete s2;
	return;
}