#include "stdafx.h"
#include "MsgSocket.h"
#include "css_protocol_package.h"
#include "css_logger.h"
#include "DshowCapture.h"

void CMsgSocket::conn_cb(css_stream_t* stream, int status)
{
    CMsgSocket* s = (CMsgSocket*)stream->data;
    printf("conn_cb status: %d\n", status);
    s->conn_cb(status);
}

void CMsgSocket::conn_cb(int status)
{
	if (status < 0){
        stat = CMsgSocket::closed;
		MyMSG msg;
		msg.code = CODE_SOCKETERROR;
		msg.bodylen = 0;
		if (receive_cb_)
			receive_cb_(&msg, msgcb_userdata_);
		return;
	}
	else{
        stat = CMsgSocket::connected;
		int ret = send_user_info();
		if (ret < 0){
			MyMSG msg;
			msg.code = CODE_SOCKETERROR;
			msg.bodylen = 0;
			if (receive_cb_)
				receive_cb_(&msg, msgcb_userdata_);
			return;
		}
		css_stream_read_start(stream_, read_cb);
	}
}

void CMsgSocket::close_cb(css_stream_t* stream)
{
	CMsgSocket* s = (CMsgSocket*)stream->data;
    s->stream_ = NULL;
	s->stat = closed;
	s->Release();
}

void CMsgSocket::read_cb(css_stream_t* stream, char* package, ssize_t status)
{
	CMsgSocket* s = (CMsgSocket*)stream->data;
	JNetCmd_Header header;
	if (status < 0){
		if (package) free(package);
		return;
	}

	css_proto_package_header_decode(package, status, &header);

	switch (header.I_CmdId){
	case cc_user_login_info:
		s->do_user_login(stream, package, status);
		break;
	case cc_media_sample:
		s->do_preview_frame(stream, package, status);
		break;
	case cc_start_send_frame:
		s->do_start_send_frame(stream, package, status);
		break;
	case cc_user_logout_info:
		s->do_user_logout(stream, package, status);
		break;
	default:
		free(package);
		break;
	}
}

//void CMsgSocket::alloc_cb(uv_handle_t* h, size_t suggested_size, uv_buf_t* buf)
//{
//	CMsgSocket* s = (CMsgSocket*)h->data;
//	buf->base = (char*)malloc(1024 * 100);
//	buf->len = 1024 * 100;
//}

//void CMsgSocket::sendsample_cb(css_write_req_t* req, int status)
//{
//	IMediaSample* s = (IMediaSample*)req->data;
//	s->Release();
//	free(req);
//}

void CMsgSocket::send_cb(css_write_req_t* req, int status)
{
	FREE(req->buf.base)
	FREE(req)
}

CMsgSocket::CMsgSocket(uv_loop_t* loop) :
loop_(loop),
receive_cb_(NULL),
msgcb_userdata_(NULL),
received_info_(false), sended_start_(false),
local_user_({ 0 }),
stream_(NULL),
CResource(e_rsc_msgsocket)
{
	if (loop_){
        stream_ = (css_stream_t*)malloc(sizeof(css_stream_t));
		css_stream_init(loop_, stream_);
		stream_->data = this;
		stat = init;
	}
	else{
		stat = uninit;
	}
}

CMsgSocket::~CMsgSocket(void)
{}

int CMsgSocket::connect_sever(char* ip, uint16_t port)
{
	int ret = 0;
	if (stat == connected){
		return ret;
	}
	else if (stat == uninit){
		return -1;
	}
	ret = uv_ip4_addr(ip, port, &server_addr_);
    if (stat != init){
        stream_ = (css_stream_t*)malloc(sizeof(css_stream_t));
        if (!stream_) return -1;
        css_stream_init(loop_, stream_);
        stream_->data = this;
        stat = init;
    }
	ret = css_stream_connect(stream_, (const sockaddr*)&server_addr_, conn_cb);;
	return ret;
}

int CMsgSocket::dis_connect(void)
{
	int ret = 0;
	if (stat == uninit){
		Release();
		return 0;
	}else {
		css_stream_close(stream_, close_cb);
	}
	return ret;
}

int CMsgSocket::send(uint8_t* buf, uint32_t len)
{
	int ret = 0;
	css_write_req_t* req_wr = (css_write_req_t*)malloc(sizeof(css_write_req_t));
	if (!req_wr)
		return -1;
	req_wr->stream = stream_;
	uv_buf_t bb;
	bb.len = len;
	bb.base = (char*)buf;
	memcpy(bb.base, buf, len);
	ret = css_stream_write(stream_, req_wr, bb, send_cb);
	if (ret < 0){
		free(bb.base);
		free(req_wr);
	}
	return ret;
}

//int CMsgSocket::send(IMediaSample* sample)
//{
//	int ret = 0;
//	uv_write_t* req_wr = (uv_write_t*)malloc(sizeof(uv_write_t));
//	if (!req_wr)
//		return -1;
//	req_wr->data = sample;
//	long len = sample->GetActualDataLength();
//	uv_buf_t buf;
//	buf.len = len;
//	sample->GetPointer((uint8_t**)&buf.base);
//	ret = uv_write(req_wr, (uv_stream_t*)&tcp_, &buf, 1, sendsample_cb);
//	if (ret >= 0){
//		sample->AddRef();
//	}
//	return ret;
//}

int CMsgSocket::on_received(void* userdata, void(*cb)(MyMSG*, void*))
{
	int ret = 0;
	receive_cb_ = cb;
	msgcb_userdata_ = userdata;
	return ret;
}

int CMsgSocket::on_received_frame(void* userdata, void(*cb)(int, cc_src_sample_t*, void* userdata))
{
    int ret = 0;
    receive_frame_cb_ = cb;
    framecb_userdata_ = userdata;
    return ret;
}

int CMsgSocket::set_local_user(cc_userinfo_t* user)
{
    local_user_.roomid = user->roomid;
    local_user_.userid = user->userid;
    local_user_.port = user->port;
    memcpy(local_user_.ip, user->ip, 16);
    return 0;
}

int CMsgSocket::set_loop(uv_loop_t* loop)
{
	loop_ = loop;
	if (loop_){ 
		css_stream_init(loop_, stream_);
		stream_->data = this;
		stat = init; 
	}
	return 0;
}

void CMsgSocket::do_preview_frame(css_stream_t* stream, char* package, ssize_t status)
{
	MyMSG msg;
	int ret = 0;
	JMedia_sample resp;

	ret = css_proto_package_decode(package, status, (JNetCmd_Header*)&resp);

	if (ret <= 0){
		//CL_ERROR("decode preview response packet error:%d.\n", ret);
		return;
	}
	msg.body.sample.buf[0] = (uint8_t*)resp.FrameData.base;
	msg.body.sample.len = resp.FrameData.len;
    msg.body.sample.sampletype = resp.FrameType;
	msg.code = resp.FrameType;

    if (receive_frame_cb_){
        receive_frame_cb_(resp.UserId, &msg.body.sample, framecb_userdata_);
    }
	free(package); /* can free anyway */
}

void CMsgSocket::do_start_send_frame(css_stream_t* stream, char* package, ssize_t status)
{
	MyMSG msg;
	int ret = 0;
	JStart_Send_Frame resp;

	ret = css_proto_package_decode(package, status, (JNetCmd_Header*)&resp);

	if (ret <= 0){
		//CL_ERROR("decode preview response packet error:%d.\n", ret);
		return;
	}
	msg.body.userinfo.userid = resp.UserId;
	msg.body.userinfo.roomid = resp.RoomId;
	msg.code = CODE_NEWUSER;

	free(package); /* can free anyway */
	if (!received_info_){
		if (receive_cb_)
			receive_cb_(&msg, msgcb_userdata_);
		received_info_ = true;
	}
	//accs_->StartCapture();
	//accs_->b_sendsample = true;
}

void CMsgSocket::do_user_login(css_stream_t* stream, char* package, ssize_t status)
{
	MyMSG msg;
	int ret = 0;
	JUser_Login_Info resp;

	ret = css_proto_package_decode(package, status, (JNetCmd_Header*)&resp);

	if (ret <= 0){
		//CL_ERROR("decode preview response packet error:%d.\n", ret);
		return;
	}
	msg.body.userinfo.userid = resp.UserId;
	msg.body.userinfo.roomid = resp.RoomId;
	msg.code = CODE_NEWUSER;
	if (receive_cb_)
		receive_cb_(&msg, msgcb_userdata_);
	free(package); /* can free anyway */
	if (!received_info_){
		//send_user_info();
		received_info_ = true;
	}
	if (!sended_start_){
		if (send_start_frame() == 0){
			sended_start_ = true;
		}
	}
}

void CMsgSocket::do_user_logout(css_stream_t* stream, char* package, ssize_t status)
{
	MyMSG msg;
	int ret = 0;
	JUser_Logout_Info resp;

	ret = css_proto_package_decode(package, status, (JNetCmd_Header*)&resp);

	if (ret <= 0){
		//CL_ERROR("decode preview response packet error:%d.\n", ret);
		return;
	}

	msg.code = CODE_USERLOGOUT;
	msg.body.userinfo.userid = resp.UserId;
	msg.body.userinfo.roomid = resp.RoomId;
	if (receive_cb_)
		receive_cb_(&msg, msgcb_userdata_);
}

int CMsgSocket::send_user_info(void)
{
	JUser_Login_Info info;
	JUser_Login_Info_init(&info);

    info.UserId = local_user_.userid;
    info.RoomId = local_user_.roomid;

	uv_buf_t buf;
	ssize_t len = css_proto_package_calculate_buf_len((JNetCmd_Header *)&info);

	buf.base = (char*)malloc(len);
	if (!buf.base){
		//capture_session_stop_record(session);
		return -1;
	}
	buf.len = len;

	css_proto_package_encode(buf.base, len, (JNetCmd_Header *)&info);
	css_write_req_t* req = (css_write_req_t*)malloc(sizeof(css_write_req_t));
	if (!req){
		free(buf.base);
		return -1;
	}

	int ret = css_stream_write(stream_, req, buf, send_cb);
	if (ret < 0){
		// CL_ERROR("write to stream error:%d(%s).\n", ret, uv_strerror(ret));
		free(buf.base);
		free(req);
		//capture_session_stop_record(session);
		return -1;
	}
	return 0;
}

int CMsgSocket::send_start_frame(void)
{
	JStart_Send_Frame ssf;
	JStart_Send_Frame_init(&ssf);

    ssf.UserId = local_user_.userid;
    ssf.RoomId = local_user_.roomid;

	uv_buf_t buf;
	ssize_t len = css_proto_package_calculate_buf_len((JNetCmd_Header *)&ssf);

	buf.base = (char*)malloc(len);
	if (!buf.base){
		//capture_session_stop_record(session);
		return -1;
	}
	buf.len = len;

	css_proto_package_encode(buf.base, len, (JNetCmd_Header *)&ssf);
	css_write_req_t* req = (css_write_req_t*)malloc(sizeof(css_write_req_t));
	if (!req){
		free(buf.base);
		return -1;
	}

	int ret = css_stream_write(stream_, req, buf, send_cb);
	if (ret < 0){
		// CL_ERROR("write to stream error:%d(%s).\n", ret, uv_strerror(ret));
		free(buf.base);
		free(req);
		//capture_session_stop_record(session);
		return -1;
	}
	return 0;
}