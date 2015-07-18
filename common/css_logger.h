/*
 * css_logger.h
 *
 *  Created on: PM 1:35:23
 *      Author: ss
 */

#ifndef CSS_LOGGER_H_
#define CSS_LOGGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "uv/uv.h"
//#include "css.h"
#include "css_util.h"
//#include "css_file_writer.h"

//public API

#define CL_DEBUG(...)   CL_LOG(LL_DEBUG, __VA_ARGS__)	//CL_DEBUG(char *fmt,...)
#define CL_INFO(...)  	CL_LOG(LL_INFO, __VA_ARGS__)	//CL_INFO(char *fmt,...)
#define CL_WARN(...)  	CL_LOG(LL_WARN, __VA_ARGS__)	//CL_WARN(char *fmt,...)
#define CL_ERROR(...) 	CL_LOG(LL_ERROR, __VA_ARGS__)	//CL_ERROR(char *fmt,...)

// private...
#define DEFAULT_LOG_DIR "./logs/"
#define DEFAULT_LOG_TIME_TIMEOUT 2*1000
#define CSS_LOGGER_MAX_FILE_SIZE 16*1024*1024
#define DEFAULT_LOG_BUF_SIZE 5*1024*1024
#define MAX_LOGGER_BUFFER_LEN 3
#define MAX_LOGGER_FILES_LEN 20

#define CSS_LOGGER_MODULE_NAME "log"

#define DEFAULT_LOG_FILE_FIX ".log"

#define CL_LOG(level,...) css_logger_log_inner(__FILE__, __LINE__, __FUNCTION__,(long)uv_thread_self(),level, __VA_ARGS__)
/**
 * %d : time like "mm-dd hh:MM:ss"
 * %p : process pid from getpid()
 * %P : level
 * %F : filename from __FILE__
 * %l : line from __LINE__
 * %f : function name from __FUNCTION__
 * %m : msg
 */
#define DEFAULT_LOG_HEADER_FMT "%d [%p][%P] <%F:%l:%f> - %m"
//#define DEFAULT_LOG_HEADER_FMT  "%s [%.5s][%d] <%s:%ld:%s> - %s"   //time [level][pid] <file:line:function> - msg
#define DEFAULT_LOG_TIME_FMT "%02d-%02d %02d:%02d:%02d.%03d"

#define CL_COPY_STR(f,s) while(*(s)) *(f)++=*(s)++

#define CSS_LOGGER_LEVELS(XX) 		\
	XX(0,DEBUG)						\
	XX(1,INFO)						\
	XX(2,WARN)						\
	XX(3,ERROR)						\

#define GEN_CSS_LOGGER_LEVEL(_level,name)  LL_##name,

typedef enum
{
	CSS_LOGGER_LEVELS(GEN_CSS_LOGGER_LEVEL)CL_UNKNOW //LL_DEBUG,LL_INFO,LL_WARN,LL_ERROR
} css_logger_level;

typedef enum
{
	CSS_LOGGER_UNINIT = 0, CSS_LOGGER_INIT
} css_logger_status;

typedef struct css_logger_writer_s
{
	uv_loop_t* loop;
	char path[MAX_PATH]; /* file path; */
	uv_file fd; /* file; */
} css_logger_writer_t;

typedef struct css_logger_write_req_s
{
	uv_buf_t buf;
	css_logger_writer_t *writer;
} css_logger_write_req_t;

typedef struct css_logger_s
{
	css_logger_status status;
	int8_t no_console;			// 1: no console log. 0: printf log to console.
	int8_t no_file;			// 1: no file.        0: fprintf log to file.
	css_logger_level level_priority;// default: 1 (LL_info) but 0 (LL_DEBUG) if _CSS_TES_ is defined.
	char *root_dir;

	css_logger_writer_t *writer;
	uint64_t writer_offset;
	uv_loop_t *loop;

	uv_timer_t *timer;		//write to log file trigger
	int64_t timeout;

	char* pattern;
} css_logger_t;

int css_logger_init(uv_loop_t *loop);	// warnning: must run loop first!.
int css_logger_start();
int css_logger_log_inner(char *file, const long line, const char *func,
		long pid, css_logger_level level, char *fmt, ...);

int css_logger_show_console(int show_console);
int css_logger_write_file(int to_file);
int css_logger_set_level(int level);

int css_logger_destroy();

#ifdef __cplusplus
}
#endif

#endif /* CSS_LOGGER_H_ */
