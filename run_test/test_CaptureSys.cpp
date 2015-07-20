#include "css.h"
#include "css_test.h"
#include "CaptureSys.h"

#define CHECKGOTO(X) if((X) < 0){goto _clean;}

static void do_nothing(uv_work_t* req)
{
	::Sleep(3000);
}

static void reconn(uv_work_t* req, int status)
{
	CMsgSocket* s = (CMsgSocket*)req->data;
	printf("recconn s stat: %d\n", s->get_stat());
	int ret = s->connect_sever("127.0.0.1", 5566);
	printf("reconn return %d\n", ret);
	free(req);
}

static void do_newuser(MyMSG* msg, CMsgSocket* s)
{
	printf("received new user: id: %d; room: %d\n", msg->body.userinfo.userid, msg->body.userinfo.roomid);
	s->dis_connect();
	uv_work_t* req = (uv_work_t*)malloc(sizeof(uv_work_t));
	req->data = s;
	int ret = uv_queue_work(uv_default_loop(), req, do_nothing, reconn);
	printf("queue reconn return %d\n", ret);
}

static void do_userlogout(MyMSG* msg, CMsgSocket* s)
{
	printf("received user %d logout\n", msg->body.userinfo.userid);
}

static void do_socket_error(MyMSG* msg, CMsgSocket* s)
{
	printf("received socket error\n");
}

static void on_msg(MyMSG* msg, void* userdata)
{
	CMsgSocket* s = (CMsgSocket*)userdata;
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

	userinfo2.port = 5566;
	userinfo2.roomid = 24;
	userinfo2.userid = 104;

	CHECKGOTO(ret = css_server_init(NULL, 0));
	CHECKGOTO(ret = css_server_start());

	CCaptureSys* s = new CCaptureSys(loop);
	s->SetUserInfo(&userinfo);
	s->SetReceivedMsgCb(s, on_msg);
	//s->on_received_frame(s, on_frame);

	CCaptureSys* s2 = new CCaptureSys(loop);
	s2->SetUserInfo(&userinfo2);
	s2->SetReceivedMsgCb(s2, on_msg);
	//s2->on_received_frame(s2, on_frame);

	ret = s->ConnectToServer("127.0.0.1", 5566);
	printf("connect_sever return %d\n", ret);
	ret = s2->ConnectToServer("127.0.0.1", 5566);
	printf("connect_sever return %d\n", ret);
	//printf("s stat: %d\n", s->get_stat());
	//printf("s2 stat: %d\n", s2->get_stat());
	uv_run(loop, UV_RUN_DEFAULT);
	printf("stop!\n");
_clean:
	css_server_stop();
	css_server_clean();
	delete s;
	return;
}