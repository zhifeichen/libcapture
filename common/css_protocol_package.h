#ifndef __CSS_PROTOCOL_PACKAGE_H__
#define __CSS_PROTOCOL_PACKAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "uv/uv.h"

enum {
#define Byte_(name)	
#define UInt16_(name)
#define UInt32_(name)
#define Int32_(name)
#define UInt64_(name)
#define FixedString_(name,len)
#define OctetString_(name,maxlen)
#define SkipBlock_(blockLen)
#define ByteArray_(name,len)
#define PDU_(classname,name,cmdid,fields) 	name=cmdid,
#include "css_protocol_package_DEF.h"
	css_dummy_cmd
};


#define Byte_(name)					uint8_t name;
#define UInt16_(name)				uint16_t name;
#define UInt32_(name)				uint32_t name;
#define Int32_(name)				int32_t name;
#define UInt64_(name)				uint64_t name;
#define FixedString_(name,len)		uv_buf_t name;
#define OctetString_(name,maxlen)	uv_buf_t name;
#define ByteArray_(name,len)		uv_buf_t name;
#define SkipBlock_(blockLen)	
#define PDU_(classname,name,cmdid,fields)				\
	typedef struct {									\
		fields											\
	}classname;
#include "css_protocol_package_DEF.h"

#define Byte_(name)	
#define UInt16_(name)
#define UInt32_(name)
#define Int32_(name)
#define UInt64_(name)
#define FixedString_(name,len)
#define OctetString_(name,maxlen)
#define SkipBlock_(blockLen)
#define ByteArray_(name,len)
#define PDU_(classname,name,cmdid,fields) 				\
	void classname##_init(classname* msg);				
#include "css_protocol_package_DEF.h"

int css_int8_encode(char* buf, int8_t v);
int css_uint8_encode(char* buf, uint8_t v);
int css_int16_encode(char* buf, int16_t v);
int css_uint16_encode(char* buf, uint16_t v);
int css_int32_encode(char* buf, int32_t v);
int css_uint32_encode(char* buf, uint32_t v);
int css_int64_encode(char* buf, int64_t v);
int css_uint64_encode(char* buf, uint64_t v);

int	css_int8_decode(char* buf, int8_t* v);
int	css_uint8_decode(char* buf, uint8_t* v);
int	css_int16_decode(char* buf, int16_t* v);
int	css_uint16_decode(char* buf, uint16_t* v);
int	css_int32_decode(char* buf, int32_t* v);
int	css_uint32_decode(char* buf, uint32_t* v);
int	css_int64_decode(char* buf, int64_t* v);
int	css_uint64_decode(char* buf, uint64_t* v);

int css_proto_package_encode(char* buf, ssize_t len, JNetCmd_Header* package);
int css_proto_package_decode(char* buf, ssize_t len, JNetCmd_Header* package);
int css_proto_package_header_decode(char* buf, ssize_t len, JNetCmd_Header* package);
ssize_t css_proto_package_calculate_buf_len(JNetCmd_Header* package);

#ifdef __cplusplus
}
#endif

#endif /* __CSS_PROTOCOL_PACKAGE_H__ */