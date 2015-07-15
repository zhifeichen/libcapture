#include "css_protocol_package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INT_ENCODE(buf, v, size)   							\
	do{														\
		int i = 0;											\
		for(i = 0; i < (size); i++){						\
			buf[i] = (char)(0xff & (v>>((size)-i-1)*8));		\
		}													\
	}while(0)

#define INT_DECODE(buf, v, size) 							\
	do{														\
		int i = 0;											\
		*v = 0;												\
		for(i = 0; i <= (size)-1; i++) {					\
			*v = ((*v<<8) | ( (unsigned char)(buf[i])) );	\
		}													\
	}while(0)

int css_int8_encode(char* buf, int8_t v)
{
	INT_ENCODE(buf, v, 1);
	return 1;
}
int css_uint8_encode(char* buf, uint8_t v)
{
	INT_ENCODE(buf, v, 1);
	return 1;
}
int css_int16_encode(char* buf, int16_t v)
{
	INT_ENCODE(buf, v, 2);
	return 2;
}
int css_uint16_encode(char* buf, uint16_t v)
{
	INT_ENCODE(buf, v, 2);
	return 2;
}
int css_int32_encode(char* buf, int32_t v)
{
	INT_ENCODE(buf, v, 4);
	return 4;
}
int css_uint32_encode(char* buf, uint32_t v)
{
	INT_ENCODE(buf, v, 4);
	return 4;
}
int css_int64_encode(char* buf, int64_t v)
{
	INT_ENCODE(buf, v, 8);
	return 8;
}
int css_uint64_encode(char* buf, uint64_t v)
{
	INT_ENCODE(buf, v, 8);
	return 8;
}

int css_int8_decode(char* buf, int8_t* v)
{
	INT_DECODE(buf, v, 1);
	return 1;
}
int css_uint8_decode(char* buf, uint8_t* v)
{
	INT_DECODE(buf, v, 1);
	return 1;
}
int css_int16_decode(char* buf, int16_t* v)
{
	INT_DECODE(buf, v, 2);
	return 2;
}
int css_uint16_decode(char* buf, uint16_t* v)
{
	INT_DECODE(buf, v, 2);
	return 2;
}
int css_int32_decode(char* buf, int32_t* v)
{
	INT_DECODE(buf, v, 4);
	return 4;
}
int css_uint32_decode(char* buf, uint32_t* v)
{
	INT_DECODE(buf, v, 4);
	return 4;
}
int css_int64_decode(char* buf, int64_t* v)
{
	INT_DECODE(buf, v, 8);
	return 8;
}
int css_uint64_decode(char* buf, uint64_t* v)
{
	INT_DECODE(buf, v, 8);
	return 8;
}

static int css_FixedString_encode(char* buf, uv_buf_t str, size_t len);
static int css_OctetString_encode(char* buf, uv_buf_t str, size_t maxlen);
static int css_ByteArray_encode(char* buf, uv_buf_t str);
static int css_SkipBlock_encode(char* buf, size_t len);

static int css_FixedString_decode(char* buf, uv_buf_t* str, size_t len);
static int css_OctetString_decode(char* buf, uv_buf_t* str, size_t maxlen);
static int css_ByteArray_decode(char* buf, uv_buf_t* str, size_t len);
static int css_SkipBlock_decode(char* buf, size_t len);

static int css_FixedString_encode(char* buf, uv_buf_t str, size_t len)
{
	size_t i;
	i = str.len < len ? str.len : len;
	memset(buf, 0, len);
	if (str.base) {
		memcpy(buf, str.base, i);
	}
	return i;
}

static int css_OctetString_encode(char* buf, uv_buf_t str, size_t maxlen)
{
	size_t i;
	i = str.len < maxlen ? str.len : maxlen;
	memset(buf, 0, i);
	memcpy(buf, str.base, i);
	buf[i - 1] = '\0';
	return i;
}

static int css_ByteArray_encode(char* buf, uv_buf_t str)
{
	if (str.base)
		memcpy(buf, str.base, str.len);
	return str.len;
}

static int css_SkipBlock_encode(char* buf, size_t len)
{
	return len;
}

static int css_FixedString_decode(char* buf, uv_buf_t* str, size_t len)
{
	str->base = buf;
	str->len = len;
	return len;
}

static int css_OctetString_decode(char* buf, uv_buf_t* str, size_t maxlen)
{
	str->base = buf;
	str->len = strlen(str->base) + 1;
	return str->len;
}

static int css_ByteArray_decode(char* buf, uv_buf_t* str, size_t len)
{
	str->base = buf;
	str->len = len;
	return len;
}

static int css_SkipBlock_decode(char* buf, size_t len)
{
	return len;
}

#define Byte_(name)							buflen += 1;
#define UInt16_(name)						buflen += 2;
#define UInt32_(name)						buflen += 4;
#define Int32_(name)						buflen += 4;
#define UInt64_(name)						buflen += 8;
#define FixedString_(name,len)				buflen += (len);
#define OctetString_(name,maxlen)			buflen += This->name.len;
#define SkipBlock_(blockLen)				buflen += (blockLen);
#define ByteArray_(name,length)				buflen += This->name.len;
#define PDU_(classname,name,cmdid,fields) 			\
	static int classname##_len(classname* This)		\
{												\
	int buflen = 0;								\
	fields										\
	return buflen;								\
}
#include "css_protocol_package_DEF.h"

#define Byte_(name)							This->name = 0;
#define UInt16_(name)						This->name = 0;
#define UInt32_(name)						This->name = 0;
#define Int32_(name)						This->name = 0;
#define UInt64_(name)						This->name = 0ull;
#define FixedString_(name,length)			This->name.base = NULL;This->name.len = (length);
#define OctetString_(name,maxlen)			This->name.base = "";This->name.len = 1;
#define SkipBlock_(blockLen)
#define ByteArray_(name,length)				This->name.base = NULL;This->name.len = 0;
#define PDU_(classname,name,cmdid,fields) 				\
	void classname##_init(classname* This)				\
	{													\
		fields											\
		This->I_CmdId = cmdid;							\
	}
#include "css_protocol_package_DEF.h"

#define Byte_(name)							if(p+1>buf_len)return UV_EAI_MEMORY;r = css_uint8_encode(pb, This->name);p += r; pb = &buf[p];
#define UInt16_(name)						if(p+2>buf_len)return UV_EAI_MEMORY;r = css_uint16_encode(pb, This->name);p += r; pb = &buf[p];
#define UInt32_(name)						if(p+4>buf_len)return UV_EAI_MEMORY;r = css_uint32_encode(pb, This->name);p += r; pb = &buf[p];
#define Int32_(name)						if(p+4>buf_len)return UV_EAI_MEMORY;r = css_int32_encode(pb, This->name);p += r; pb = &buf[p];
#define UInt64_(name)						if(p+8>buf_len)return UV_EAI_MEMORY;r = css_uint64_encode(pb, This->name);p += r; pb = &buf[p];
#define FixedString_(name,len)				if(p+(len)>buf_len)return UV_EAI_MEMORY;r = css_FixedString_encode(pb, This->name, (len));p += r; pb = &buf[p];
#define OctetString_(name,maxlen)			if(p+This->name.len>buf_len)return UV_EAI_MEMORY;r = css_OctetString_encode(pb, This->name, (maxlen));p += r; pb = &buf[p];
#define SkipBlock_(blockLen)				if(p+(blockLen)>buf_len)return UV_EAI_MEMORY;r = css_SkipBlock_encode(pb, (blockLen));p += r; pb = &buf[p];
#define ByteArray_(name,length)				if(p+This->name.len>buf_len)return UV_EAI_MEMORY;r = css_ByteArray_encode(pb, This->name);p += r; pb = &buf[p];
#define PDU_(classname,name,cmdid,fields) 						\
	static int classname##_encode(char* buf, ssize_t buf_len, classname* This)	\
	{															\
		int p = 0, r = 0;										\
		char* pb = buf;											\
		fields													\
		css_uint32_encode(buf, p);								\
		return p;												\
	}
#include "css_protocol_package_DEF.h"

#define Byte_(name)							if(p+1>buf_len)return UV_EAI_MEMORY;r = css_uint8_decode(pb, &This->name);p += r; pb = &buf[p];
#define UInt16_(name)						if(p+2>buf_len)return UV_EAI_MEMORY;r = css_uint16_decode(pb, &This->name);p += r; pb = &buf[p];
#define UInt32_(name)						if(p+4>buf_len)return UV_EAI_MEMORY;r = css_uint32_decode(pb, &This->name);p += r; pb = &buf[p];
#define Int32_(name)						if(p+4>buf_len)return UV_EAI_MEMORY;r = css_int32_decode(pb, &This->name);p += r; pb = &buf[p];
#define UInt64_(name)						if(p+8>buf_len)return UV_EAI_MEMORY;r = css_uint64_decode(pb, &This->name);p += r; pb = &buf[p];
#define FixedString_(name,len)				if(p+(len)>buf_len)return UV_EAI_MEMORY;r = css_FixedString_decode(pb, &This->name, (len));p += r; pb = &buf[p];
#define OctetString_(name,maxlen)			if(p+(1)>buf_len)return UV_EAI_MEMORY;r = css_OctetString_decode(pb, &This->name, (maxlen));p += r; pb = &buf[p];
#define SkipBlock_(blockLen)				if(p+(blockLen)>buf_len)return UV_EAI_MEMORY;r = css_SkipBlock_decode(pb, (blockLen));p += r; pb = &buf[p];
#define ByteArray_(name,len)				if(p+(len)>buf_len)return UV_EAI_MEMORY;r = css_ByteArray_decode(pb, &This->name, (len));p += r; pb = &buf[p];
#define PDU_(classname,name,cmdid,fields) 						\
	static int classname##_decode(char* buf, ssize_t buf_len, classname* This)	\
	{															\
		int p = 0, r = 0;										\
		char* pb = buf;											\
		fields													\
		return p;												\
	}
#include "css_protocol_package_DEF.h"

int css_proto_package_encode(char* buf, ssize_t buf_len, JNetCmd_Header* package)
{
	switch (package->I_CmdId) {
#define Byte_(name)	
#define UInt16_(name)
#define UInt32_(name)
#define Int32_(name)
#define UInt64_(name)
#define FixedString_(name,len)
#define OctetString_(name,maxlen)
#define SkipBlock_(blockLen)
#define ByteArray_(name,len)
#define PDU_(classname,name,cmdid,fields) 							\
	case cmdid:														\
	{																\
		return classname##_encode(buf, buf_len, (classname *)package);		\
	}
#include "css_protocol_package_DEF.h"
	default:
		return 0;
		break;
	}
}

int css_proto_package_decode(char* buf, ssize_t buf_len, JNetCmd_Header* package)
{
	uint32_t cmdId;
	css_uint32_decode(&buf[4], &cmdId);
	switch (cmdId) {
#define Byte_(name)	
#define UInt16_(name)
#define UInt32_(name)
#define Int32_(name)
#define UInt64_(name)
#define FixedString_(name,len)
#define OctetString_(name,maxlen)
#define SkipBlock_(blockLen)
#define ByteArray_(name,len)
#define PDU_(classname,name,cmdid,fields) 							\
	case cmdid:														\
	{																\
		return classname##_decode(buf, buf_len, (classname *)package);		\
	}
#include "css_protocol_package_DEF.h"
	default:
		return 0;
		break;
	}
}

int css_proto_package_header_decode(char* buf, ssize_t buf_len, JNetCmd_Header* package)
{
	int p = 0;
	int r = 0;

	if(p + 20 > buf_len) {
		return UV_EAI_MEMORY;
	}

	r = css_uint32_decode(&buf[p], &package->I_CmdLen);
	p += r;
	r = css_uint32_decode(&buf[p], &package->I_CmdId);
	p += r;
	r = css_uint32_decode(&buf[p], &package->I_SeqNum);
	p += r;
	r = css_uint32_decode(&buf[p], &package->I_SrcAddr);
	p += r;
	r = css_uint32_decode(&buf[p], &package->I_DestAddr);
	p += r;
	return p;
}

ssize_t css_proto_package_calculate_buf_len(JNetCmd_Header* package)
{
	switch (package->I_CmdId) {
#define Byte_(name)	
#define UInt16_(name)
#define UInt32_(name)
#define Int32_(name)
#define UInt64_(name)
#define FixedString_(name,len)
#define OctetString_(name,maxlen)
#define SkipBlock_(blockLen)
#define ByteArray_(name,len)
#define PDU_(classname,name,cmdid,fields) 							\
	case cmdid:														\
	{																\
		return classname##_len((classname *)package);				\
	}
#include "css_protocol_package_DEF.h"
	default:
		return 0;
		break;
	}
}

/*
 * TEST
 */
#ifdef CSS_TEST

void test_encode(void)
{
	char buf[8] = {0};
	int8_t i8 = 0x87;
	uint8_t ui8 = 0x87;
	int16_t i16 = 0x1234;
	uint16_t ui16 = 0x4321;
	int32_t i32 = 0x12345678;
	uint32_t ui32 = 0x87654321;
	int64_t i64 = 0x1234567812345678ll;
	uint64_t ui64 = 0x8765432187654321ll;

	css_int8_encode(buf, i8);
	printf("buf: %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	memset(buf, 0, 8);
	css_uint8_encode(buf, ui8);
	printf("buf: %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	memset(buf, 0, 8);
	css_int16_encode(buf, i16);
	printf("buf: %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	memset(buf, 0, 8);
	css_uint16_encode(buf, ui16);
	printf("buf: %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	memset(buf, 0, 8);
	css_int32_encode(buf, i32);
	printf("buf: %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	memset(buf, 0, 8);
	css_uint32_encode(buf, ui32);
	printf("buf: %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	memset(buf, 0, 8);
	css_int64_encode(buf, i64);
	printf("buf: %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	memset(buf, 0, 8);
	css_uint64_encode(buf, ui64);
	printf("buf: %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
}

void test_decode(void)
{
	char buf[] = {0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12};
	int8_t i8 = 0;
	uint8_t ui8 = 0;
	int16_t i16 = 0;
	uint16_t ui16 = 0;
	int32_t i32 = 0;
	uint32_t ui32 = 0;
	int64_t i64 = 0;
	uint64_t ui64 = 0;

	css_int8_decode( buf, &i8);
	printf("i8: %x\n", i8);

	css_uint8_decode( buf, &ui8);
	printf("ui8: %x\n", ui8);

	css_int16_decode( buf, &i16);
	printf("i16: %x\n", i16);

	css_uint16_decode( buf, &ui16);
	printf("ui16: %x\n", ui16);

	css_int32_decode( buf, &i32);
	printf("i32: %x\n", i32);

	css_uint32_decode( buf, &ui32);
	printf("ui32: %x\n", ui32);

	css_int64_decode( buf, &i64);
	printf("i64: %llx\n", i64);

	css_uint64_decode( buf, &ui64);
	printf("ui64: %llx\n", ui64);
}

void test_codec(void)
{
	JNetCmd_AlarmUpMsgServer_Ex msg, msg2;
	uv_buf_t xml;
	uv_buf_t xml2;
	char* buf;
	int len;
	int r = 0;

	xml = uv_buf_init("<?xml >", 8);
	xml2 = uv_buf_init("<?xml2 >", 9);
	msg.I_CmdLen = 0;
	msg.I_CmdId = 0x000B0001;
	msg.I_SeqNum = msg.I_SrcAddr = msg.I_DestAddr = 0;
	msg.AlarmInfoXmlLen = 8;
	msg.AlarmInfoXml = xml;
	msg.RelationInfoXmlLen = 9;
	msg.RelationInfoXml = xml2;

	/*
	 len = JNetCmd_AlarmUpMsgServer_Ex_len(&msg);
	 buf = (char*)malloc(len);

	 r = JNetCmd_AlarmUpMsgServer_Ex_encode(buf, &msg);
	 r = JNetCmd_AlarmUpMsgServer_Ex_decode(buf, &msg2);
	 printf("%d\n", r);
	 free(buf);
	 */

	len = css_proto_package_calculate_buf_len((JNetCmd_Header*)&msg);
	buf = (char*)malloc(len);

	r = css_proto_package_encode(buf, len, (JNetCmd_Header*)&msg);
	//msg2.I_CmdId = 0x000B0001;
	r = css_proto_package_decode(buf, len, (JNetCmd_Header*)&msg2);
	printf("%d\n", r);
	free(buf);

	/* error buf test: */

	JNetCmd_AlarmUpMsgServer_Ex_init(&msg);
	msg.AlarmInfoXmlLen = 8;
	msg.AlarmInfoXml = xml;
	msg.RelationInfoXmlLen = 9;
	msg.RelationInfoXml = xml2;

	len = css_proto_package_calculate_buf_len((JNetCmd_Header*)&msg);
	buf = (char*)malloc(len);

	r = css_proto_package_encode(buf, len - 2, (JNetCmd_Header*)&msg);
	printf("r should be %d, and the r == %d\n", UV_EAI_MEMORY, r);

	r = css_proto_package_encode(buf, len, (JNetCmd_Header*)&msg);
	printf("r should be %d, and the r == %d\n", len, r);

	r = css_proto_package_decode(buf, len - 1, (JNetCmd_Header*)&msg2);
	printf("r should be %d, and the r == %d\n", UV_EAI_MEMORY, r);
	r = css_proto_package_decode(buf, len, (JNetCmd_Header*)&msg2);
	printf("r should be %d, and the r == %d\n", len, r);
	free(buf);
}

void test_uint32(void)
{
	uint32_t i = 0x12345678;
	uint32_t j, j2;

	css_uint32_encode((char*)&j, i);

	css_uint32_decode((char*)&j, &j2);
}

void test_package_init(void)
{
	JNetCmd_PreviewConnect_Ex msg;
	char *buf;
	size_t len;

	JNetCmd_AddCentralizeStorageServer msg2;

	JNetCmd_Header header;

	JNetCmd_PreviewConnect_Ex_init(&msg);
	msg.ChannelNo = 1;
	msg.CodingFlowType = 0;

	len = css_proto_package_calculate_buf_len((JNetCmd_Header *)&msg);
	buf = (char*)malloc(len);
	css_proto_package_encode(buf, len, (JNetCmd_Header *)&msg);
	css_proto_package_header_decode(buf, len, &header);
	free(buf);

	JNetCmd_AddCentralizeStorageServer_init(&msg2);
	msg2.wsUriPrefix.base = "http://128.8.153.99:8080/sc/";
	msg2.wsUriPrefix.len = strlen("http://128.8.153.99:8080/sc/");
	len = css_proto_package_calculate_buf_len((JNetCmd_Header *)&msg2);
	buf = (char*)malloc(len);
	css_proto_package_encode(buf, len, (JNetCmd_Header *)&msg2);
	css_proto_package_header_decode(buf, len, &header);
	free(buf);
}
void test_protocol_packege_suite()
{
	test_encode();
	test_decode();
	test_codec();
	test_uint32();
	test_package_init();
}

#endif
