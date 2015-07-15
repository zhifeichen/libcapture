/*
 * css_logger.c
 *
 *  Created on: pm1:44:05
 *      Author: ss
 */

#include "uv/uv.h"
#include "css_logger.h"
#include "css_util.h"
//#include "css.h"
#include "css_ini_file.h"

static css_logger_t css_logger = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
DEFAULT_LOG_HEADER_FMT };
static uv_mutex_t css_logger_mutex;
static uv_once_t init_css_logger_guard_ = UV_ONCE_INIT;
static char buffers[MAX_LOGGER_BUFFER_LEN][DEFAULT_LOG_BUF_SIZE];
static int bindex = 0;
static char *buffer;

static const int64_t dump_to_file_size = DEFAULT_LOG_BUF_SIZE * 80 / 100;
static int64_t offset = 0;

#define LOGGER_UV_MUTEX_LOCK(m) uv_mutex_lock((m));//printf("%s lock.\n",__FUNCTION__)
#define LOGGER_UV_MUTEX_UNLOCK(mutex) uv_mutex_unlock((mutex))
//#define LOGGER_UV_MUTEX_UNLOCK(mutex) printf("%s unlock.\n",__FUNCTION__);uv_mutex_unlock((mutex))

//priv
static int css_logger_new_file();

int css_logger_set_level(int level)
{
	css_logger.level_priority = level;
	return 0;
}

int css_logger_show_console(int console)
{
	css_logger.no_console = console;
	return 0;
}

int css_logger_write_file(int file)
{
	css_logger.no_file = file;
	return 0;
}

void css_logger_init_mutex_cb(void)
{
	uv_mutex_init(&css_logger_mutex);
}

void css_logger_set_config()
{
	char *flag = NULL;
	if (css_logger.pattern && strcmp(css_logger.pattern, DEFAULT_LOG_HEADER_FMT) != 0) {
		FREE(css_logger.pattern);
	}
	css_get_env(CSS_LOGGER_MODULE_NAME, "format", DEFAULT_LOG_HEADER_FMT, &css_logger.pattern);
	css_get_env(CSS_LOGGER_MODULE_NAME, "no_console", "0", &flag);
	if (flag != NULL)
		css_logger.no_console = atoi(flag);
	flag = NULL;
	css_get_env(CSS_LOGGER_MODULE_NAME, "level", "0", &flag);
	if (flag != NULL)
		css_logger.level_priority = atoi(flag);
	FREE(flag);
}

int css_logger_init(uv_loop_t *loop)
{
	if (loop == NULL) {
		printf("init logger error ,loop is null!\n");
		return -1;
	}
	uv_once(&init_css_logger_guard_, css_logger_init_mutex_cb);

	css_logger_set_config();
	css_logger.loop = loop;
	return 0;
}

size_t cl_index_of_basename(char *file)
{
	char *s;
	size_t len = strlen(file);
	if (len == 0)
		return 0;
	s = file + len;
	for (; len > 0; len--, s--) {
		if (*s == '/' || *s == '\\')
			break;
	}
	return len + 1;

}

static void css_log_time(char *time_str, size_t time_str_size)
{
	time_t tt;
	struct tm *local_time;
	struct timeval tv;
	time(&tt);
	local_time = localtime(&tt);

	gettimeofday(&tv, NULL);
	sprintf(time_str, DEFAULT_LOG_TIME_FMT, local_time->tm_mon + 1, local_time->tm_mday, local_time->tm_hour,
			local_time->tm_min, local_time->tm_sec, tv.tv_usec / 1000);
}

#define GEN_CSS_LOGGER_NAME_CASE(level,name) case level: return #name;

char* GET_CSS_LOGGER_LEVEL_NAME(int level)
{
	switch (level) {
	CSS_LOGGER_LEVELS(GEN_CSS_LOGGER_NAME_CASE)
	default:
		return "INFO";
	}
}

void after_roll_logger_cb(uv_fs_t* req)
{
//	printf("close file %s.\n", writer->path);
	FREE(req->data);	// css_logger_writer_t *writer;
	uv_fs_req_cleanup(req);
	FREE(req);
}
void after_write_logger_and_close_cb(uv_fs_t* req)
{
	int r;
	uv_fs_t* close_req;
	css_logger_write_req_t *wreq = (css_logger_write_req_t*) req->data;
	css_logger_writer_t *writer = wreq->writer;
	if (req->result < 0) {
		printf("writer last data to log %s error:%s.\n", wreq->writer->path, uv_strerror((int) req->result));
	}
	uv_fs_req_cleanup(req);
	FREE(wreq);
	FREE(req);

	close_req = (uv_fs_t*) malloc(sizeof(uv_fs_t));
	close_req->data = writer;
	r = uv_fs_close(css_logger.loop, close_req, writer->fd, after_roll_logger_cb);
	if (r < 0) {
		printf("close file %s error:%s \n", writer->path, uv_strerror((int) req->result));
		FREE(writer);
		FREE(close_req);
	}
}
void after_write_logger_cb(uv_fs_t* req)
{
	css_logger_write_req_t *wreq = (css_logger_write_req_t*) req->data;
	if (req->result < 0) {
		printf("writer log %s error:%s.\n", wreq->writer->path, uv_strerror((int) req->result));
	}
	uv_fs_req_cleanup(req);
	FREE(wreq);
	FREE(req);
}
void css_logger_dump_to_file(int is_lock, int should_close)
{
	char *tbuffer;
	int64_t len = offset;
	uv_fs_t *req;
	css_logger_write_req_t *wreq;
	int ret;
	int should_roll_log = 0;
	if (!css_logger.writer) {
		printf("writer is null.\n");
		offset = 0;
		return;
	}
	if (!is_lock)
		LOGGER_UV_MUTEX_LOCK(&css_logger_mutex);
	if (offset == 0) {
		if (should_close) {
			req = (uv_fs_t*) malloc(sizeof(uv_fs_t));
			req->data = css_logger.writer;
			ret = uv_fs_close(css_logger.loop, req, css_logger.writer->fd, after_roll_logger_cb);
			if (ret < 0) {
				printf("close file:%s error by reason:%s\n", css_logger.writer->path, uv_strerror(ret));
				FREE(req);
				FREE(css_logger.writer);
			}
		}
		if (!is_lock)
			LOGGER_UV_MUTEX_UNLOCK(&css_logger_mutex);
		return;
	}
	tbuffer = buffer;
	if (bindex > (MAX_LOGGER_BUFFER_LEN - 1))
		bindex = 0;

	buffer = buffers[bindex];
	bindex++;
	offset = 0;
	*buffer = '\0';
	css_logger.writer_offset += len;

	wreq = (css_logger_write_req_t*) malloc(sizeof(css_logger_write_req_t));
	wreq->buf.base = tbuffer;
	wreq->buf.len = len;
	wreq->writer = css_logger.writer;
	req = (uv_fs_t*) malloc(sizeof(uv_fs_t));
	req->data = wreq;
//	printf("dump %lld.%lldb to file:%s.\n", css_logger.writer_offset / 1024,
//			css_logger.writer_offset % 1024, css_logger.writer->path);
	should_roll_log = css_logger.writer_offset > CSS_LOGGER_MAX_FILE_SIZE ? 1 : 0;
	if (should_roll_log)
		should_close = 1;
	if (should_close) {
		// close log.
		ret = uv_fs_write(css_logger.loop, req, css_logger.writer->fd, &wreq->buf, 1, -1,
				after_write_logger_and_close_cb);
		if (ret < 0) {
			req->data = wreq->writer;
			ret = uv_fs_close(css_logger.loop, req, wreq->writer->fd, after_roll_logger_cb);
			FREE(wreq);
		}
		css_logger.writer = NULL;
		css_logger.writer_offset = 0;
		if (should_roll_log) {
			css_logger_new_file();
		}
	} else {
		ret = uv_fs_write(css_logger.loop, req, css_logger.writer->fd, &wreq->buf, 1, -1, after_write_logger_cb);
	}
	if (!is_lock)
		LOGGER_UV_MUTEX_UNLOCK(&css_logger_mutex);
}

void css_logger_dump_to_file_cb(uv_timer_t* handle)
{
//	printf("dump %lld,%lldB to file timeout.\n", offset / 1024,offset % 1024);
	css_logger_dump_to_file(0, 0);
}
/**
 * start logger writer timer.
 */
void css_logger_start_timer()
{
// start timer
	css_logger.timer = (uv_timer_t*) malloc(sizeof(uv_timer_t));
	uv_timer_init(css_logger.loop, css_logger.timer);
	uv_timer_start(css_logger.timer, css_logger_dump_to_file_cb, css_logger.timeout, css_logger.timeout);

}

int css_logger_rename_old_logs()
{
	// TODO: REIMPLEMENT
//	uv_fs_t req, rreq;
//	int r;
//	r = uv_fs_readdir(css_logger.loop, &req, css_logger.root_dir, 0,
//	NULL);
//	if (r > 0) {
//		size_t len, pos;
//		int i, lindex, ibasename[MAX_LOGGER_FILES_LEN] = { 0 };
//		char *fix;
//		char *filename = req.ptr;
//		char old_path[MAX_PATH], new_path[MAX_PATH];
//		for (i = 0; i < req.result; i++) {
////			printf("filename:%s\n",filename);
//			fix = strstr(filename, DEFAULT_LOG_FILE_FIX);
//			len = strlen(filename);
//			if (fix != NULL) {
//				pos = fix - filename;
//				sprintf(old_path, "%s%s", css_logger.root_dir, filename);
//				filename[pos] = '\0';
//				lindex = atoi(filename);
//				if (lindex < MAX_LOGGER_FILES_LEN) {
//					ibasename[lindex] = 1;
//				}
//			}
//			filename += len + 1;
//		}
//		for (lindex = MAX_LOGGER_FILES_LEN - 1; lindex >= 0; lindex--) {
//			if (ibasename[lindex]) {
//				sprintf(old_path, "%s%d%s", css_logger.root_dir, lindex, DEFAULT_LOG_FILE_FIX);
//				if (lindex + 1 < MAX_LOGGER_FILES_LEN) {
//					sprintf(new_path, "%s%d%s", css_logger.root_dir, lindex + 1, DEFAULT_LOG_FILE_FIX);
//					r = uv_fs_rename(css_logger.loop, &rreq, old_path, new_path,
//					NULL);
////					printf("rename %s to %s.\n", old_path, new_path);
//					if (r != 0) {
//						printf("rename %s to %s error:%s.\n", old_path, new_path, uv_strerror(r));
//						return r;
//					}
//				} else {
//					r = uv_fs_unlink(css_logger.loop, &rreq, old_path, NULL);
//					if (r != 0) {
//						printf("unlink file %s error:%s.\n", old_path, uv_strerror(r));
//						return r;
//					}
//				}
//				uv_fs_req_cleanup(&rreq);
//			}
//		}
//	}
//	uv_fs_req_cleanup(&req);
	return 0;
}

/**
 * open new file, filename: $ROOT_DIR/yyyyMMddhhmmss.log
 */
int css_logger_new_file()
{
	int r;
	uv_fs_t req;
	r = css_logger_rename_old_logs();
	if (r < 0)
		return r;
	css_logger.writer = (css_logger_writer_t*) malloc(sizeof(css_logger_writer_t));
	sprintf(css_logger.writer->path, "%s%d%s", css_logger.root_dir, 0, DEFAULT_LOG_FILE_FIX);
	css_logger.writer->loop = css_logger.loop;
	css_logger.writer_offset = 0;
	r = uv_fs_open(css_logger.loop, &req, css_logger.writer->path,
	O_RDWR | O_CREAT, S_IWUSR | S_IRUSR, NULL);

	if (r < 0) {
		printf("open logger %s faild.\n", css_logger.writer->path);
		uv_fs_req_cleanup(&req);
	} else {
		// set fd to r, start timer and reset r=0 when r >0
//		printf("open logger %s successful.\n", css_logger.writer->path);
		css_logger.writer->fd = (uv_file) req.result;
		uv_fs_req_cleanup(&req);
		r = 0;
	}
	return r;
}
/**
 * reopen last log file.
 */
int css_logger_reopen_file(char *filenames, int len)
{
	int r;
	char *logger_name_t = NULL;
	char *filename = filenames; //readdir_req.ptr;
	uv_fs_t req;
	int i;
	for (i = 0; i < len; i++) {
		if (strstr(filename, DEFAULT_LOG_FILE_FIX) != NULL) {
			if (logger_name_t == NULL || strcmp(filename, logger_name_t) < 0) {
				logger_name_t = filename;
			}
		}
		filename += strlen(filename) + 1;
	}
	if (logger_name_t == NULL) {
		r = css_logger_new_file();
	} else {
		css_logger.writer = (css_logger_writer_t*) malloc(sizeof(css_logger_writer_t));
		sprintf(css_logger.writer->path, "%s%s", css_logger.root_dir, logger_name_t);
		css_logger.writer->loop = css_logger.loop;
		r = uv_fs_open(css_logger.loop, &req, css_logger.writer->path,
		O_RDWR | O_APPEND, S_IWUSR | S_IRUSR, NULL);
		if (r < 0 || req.result < 0) {
			printf("reopen logger %s faild.\n", css_logger.writer->path);
			uv_fs_req_cleanup(&req);
		} else {
			// set fd to r, start timer and reset r=0 when r >0
			css_logger.writer->fd = (uv_file) req.result;
			uv_fs_req_cleanup(&req);
			r = uv_fs_stat(css_logger.loop, &req, css_logger.writer->path,
			NULL);
			if (r == 0 && req.result == 0) {
				uv_stat_t s = req.statbuf;
				css_logger.writer_offset = s.st_size;
				printf("reopen logger %s,offset:%lld successful.\n", css_logger.writer->path, css_logger.writer_offset);

			}
			uv_fs_req_cleanup(&req);
		}
	}
	return r;
}

int open_logger_file()
{
	int r;

	// open new logger file.
	printf("open new logger file....\n");
	r = css_logger_new_file();
	if (r == 0) {
		css_logger_start_timer();
	}

	// TODO: REIMPLEMENT
	//uv_fs_t readdir_req;
	//r = uv_fs_readdir(css_logger.loop, &readdir_req, css_logger.root_dir, 0,
	//NULL);

	//if (r < 0) {
	//	printf("list logger dir:%s failed.\n", css_logger.root_dir);
	//} else {
	//	if (r == 0) {
	//		// open new logger file.
	//		printf("open new logger file....\n");
	//		r = css_logger_new_file();
	//		if (r == 0) {
	//			css_logger_start_timer();
	//		}
	//	} else {
	//		// reopen last logger file.
	//		printf("reopen logger file....\n");
	//		r = css_logger_reopen_file(readdir_req.ptr, (int) readdir_req.result);
	//		if (r == 0) {
	//			css_logger_start_timer();
	//		}
	//	}
	//}
	//uv_fs_req_cleanup(&readdir_req);
	return r;

}

int css_logger_start()
{
	int r = 0;
	if (css_logger.loop == NULL)
		return -1;
	if (css_logger.status == CSS_LOGGER_UNINIT) {
		LOGGER_UV_MUTEX_LOCK(&css_logger_mutex);
		css_logger.no_file = 0;
		css_logger.root_dir = DEFAULT_LOG_DIR;
		css_logger.timeout = DEFAULT_LOG_TIME_TIMEOUT;
		ensure_dir(css_logger.root_dir);

//#ifdef CSS_TEST
		css_logger.level_priority = LL_DEBUG;
//#else
//		css_logger.level_priority = LL_INFO;
//#endif
		if (!css_logger.no_file) {
			r = open_logger_file();
			buffer = buffers[bindex++];
			if (r == 0) {
				css_logger.status = CSS_LOGGER_INIT;
			}
		}
		LOGGER_UV_MUTEX_UNLOCK(&css_logger_mutex);
	} else {
		printf("already init logger.\n");
	}
	return r;
}

void css_logger_dump_to_file_wcb(uv_work_t* req)
{
//	printf("dump out of buffer %lld,%lldB data to log.\n", offset / 1024,offset % 1024);
	css_logger_dump_to_file(0, 0);
}
void css_logger_after_dump_to_file_cb(uv_work_t* req, int status)
{
	FREE(req);
}

int css_logger_fmt_replace_enter(char **ffmt, char *fmt)
{
	size_t len = strlen(fmt);
	char *tmp, *dest_str, *from_str = fmt;
	int pos = 0;
	if ((tmp = strstr(fmt, "\n")) == NULL)
		return -1;
	*ffmt = dest_str = malloc(len + 32);
	do {
		pos = tmp - from_str;
		strncpy(dest_str, from_str, pos);
		dest_str = dest_str + pos;
		*dest_str++ = '\r';
		*dest_str++ = '\n';
		tmp++;
		from_str = tmp;
	} while ((tmp = strstr(tmp, "\n")) != NULL);
	*dest_str = '\0';
	return 0;

}

int css_logger_gen_format(char *fmt, char *pattern, char *time_str, char *file, const long line, const char *func,
		int pid, char* level_str, char *msg)
{
	char *p = pattern;
	char *f = fmt;
	while (*p != 0) {
		if (*p == '%') {
			p++;
			if (*p != 0) {
				//"%d [%l][%P] <%F:%l:%f> - %m"
				switch (*p) {
				case 'd':	// time like "mm-dd hh:MM:ss"
					CL_COPY_STR(f, time_str);
					break;
				case 'p':	// level
					CL_COPY_STR(f, level_str);
					break;
				case 'P':	// pid :  getpid()
					itoa(pid, f, 10);
					f += strlen(f);
					break;
				case 'F':	// filename :  __FILE__
					CL_COPY_STR(f, file);
					break;
				case 'l':	// line : __LINE__
					itoa(line, f, 10);
					f += strlen(f);
					break;
				case 'f':	// func : __FUNCTION__
					CL_COPY_STR(f, func);
					break;
				case 'm':	// msg
					CL_COPY_STR(f, msg);
					break;
				case '%':
					*f++ = '%';
					*f++ = '%';
					break;
				default:
					break;
				}
			}
		} else {
			*f = *p;
			f++;
		}
		p++;
	}
	*f = '\0';
	return 0;
}

int css_logger_log_inner(char *file, const long line, const char *func, long pid, css_logger_level level, char *fmt,
		...)
{
//ignore when lower priority or no console,no file
	if ((css_logger.no_console && css_logger.no_file) || level < css_logger.level_priority) {
		return 0;
	} else {
		char time_str[24 + 1];
		char *real_fmt = malloc(BUFSIZ);
		size_t basename_index;
		int ret;
		char *ffmt = NULL;
		va_list args;
		css_log_time(time_str, sizeof(time_str));
		basename_index = cl_index_of_basename(file);
#ifdef _WIN32
		if (css_logger_fmt_replace_enter(&ffmt, fmt) == 0) {
			css_logger_gen_format(real_fmt, css_logger.pattern, time_str,
					file + basename_index, line, func, pid,
					GET_CSS_LOGGER_LEVEL_NAME(level), ffmt);
			FREE(ffmt);
		} else {
			css_logger_gen_format(real_fmt, css_logger.pattern, time_str,
					file + basename_index, line, func, pid,
					GET_CSS_LOGGER_LEVEL_NAME(level), fmt);
		}
#else
		css_logger_gen_format(real_fmt, css_logger.pattern, time_str, file + basename_index, line, func, pid,
				GET_CSS_LOGGER_LEVEL_NAME(level), fmt);
#endif
		if (!css_logger.no_console) {
			// printf to console.
			va_start(args, fmt);
			vprintf(real_fmt, args);
		}
		if (css_logger.status == CSS_LOGGER_INIT && !css_logger.no_file && css_logger.writer) {
			// write to buffer first, dump to file latter.
			char *tbuffer;
			va_start(args, fmt);
			LOGGER_UV_MUTEX_LOCK(&css_logger_mutex);
			tbuffer = buffer + offset;
			ret = vsnprintf(tbuffer, DEFAULT_LOG_BUF_SIZE - offset, real_fmt, args);

			if (ret >= DEFAULT_LOG_BUF_SIZE - offset) {
				// if log is too log to buffer.
				// 1. dump to file sync.
				css_logger_dump_to_file(1, 0);
				// 2. retry.
				ret = vsnprintf(tbuffer, DEFAULT_LOG_BUF_SIZE - offset, real_fmt, args);
				if (ret >= DEFAULT_LOG_BUF_SIZE - offset) {
					//3. ignore log when has no buffer also!!
					printf("out of logger buffer,ignore.\n");
				}
			} else {
				offset += strlen(tbuffer);
				if (offset > dump_to_file_size) {
					// dump to file sync.
					css_logger_dump_to_file(1, 0);
				}
			}
			FREE(real_fmt);
			LOGGER_UV_MUTEX_UNLOCK(&css_logger_mutex);

		} else {
			FREE(real_fmt);
		}
		va_end(args);
		return 0;
	}
}

void css_logger_close_file(uv_work_t* req, int status)
{
	FREE(req);
//	css_logger.no_console = 1;
//	css_logger.no_file = 1;
	offset = 0;
}

void css_logger_flush_to_file(uv_work_t* req)
{

	printf("dump last %lld,%lldB data to log.\n", offset / 1024, offset % 1024);
	css_logger_dump_to_file(0, 1);
}

static void css_logger_timer_close_cb(uv_handle_t* h)
{
	FREE(h);
}

int css_logger_close()
{
	LOGGER_UV_MUTEX_LOCK(&css_logger_mutex);
	printf("logger close.\n");
	if (css_logger.status == CSS_LOGGER_UNINIT) {
		printf("logger unnited!.\n");
		LOGGER_UV_MUTEX_UNLOCK(&css_logger_mutex);
		return 0;
	}
	if (css_logger.loop) {
		uv_work_t *req = (uv_work_t*) malloc(sizeof(uv_work_t));
		if (css_logger.timer != NULL) {
			uv_timer_stop(css_logger.timer);
			printf("stop logger timer.\n");
			uv_close((uv_handle_t*)css_logger.timer, css_logger_timer_close_cb);
			//FREE(css_logger.timer);
			uv_queue_work(css_logger.loop, req, css_logger_flush_to_file, css_logger_close_file);
		}
	}
	css_logger.level_priority = 0;
	css_logger.no_console = 0;
	css_logger.no_file = 0;
	css_logger.status = CSS_LOGGER_UNINIT;
	LOGGER_UV_MUTEX_UNLOCK(&css_logger_mutex);

	return 0;
}

int css_logger_destroy()
{
	printf("destroy css logger.\n");

	css_logger_close();
	return 0;
}

#ifdef CSS_TEST

#include <assert.h>
static int mutil_test_count = 20;
static int mutil_test_clients = 10;

void test_css_logger_gen_fmt()
{
	char fmt[256];
	css_logger_gen_format(fmt, "%d [%p][%P] <%F:%l:%f> - %m", "time",
			"file", 1, "func",2, "DEBUG","msg");
//	printf("fmt:%s\n",fmt);
	assert(strcmp("time [DEBUG][2] <file:1:func> - msg", fmt) == 0);
	css_logger_gen_format(fmt, "%d [%p][%P] <%F:%l:%f> %%%%- %m", "time",
			"file", 1, "func",2, "DEBUG","msg");
//	printf("fmt:%s\n",fmt);
	assert(strcmp("time [DEBUG][2] <file:1:func> %%%%- msg", fmt) == 0);
	css_logger_gen_format(fmt, "%d [%p][%P] <%F:%l:%f> %u- %m", "time",
			"file", 1, "func",2, "DEBUG","msg");
//	printf("fmt:%s\n",fmt);
	assert(strcmp("time [DEBUG][2] <file:1:func> - msg", fmt) == 0);
}

/**
 * will just printf to console log when uv_loop is not run.
 */
void test_css_logger_console_log()
{
// test log.
	CL_DEBUG("test logger %s %d.\n", __FUNCTION__, 1);
	CL_DEBUG("test logger \n");
	css_logger_set_level(2);
	CL_DEBUG("test logger %s %d.\n", __FUNCTION__, 2);
	CL_INFO("test logger %s %d.\n", __FUNCTION__, 3);
	CL_WARN("test logger \n");
//	printf("%s %d %s.\n",strstr(__FILE__,"src"),__LINE__,__FUNCTION__);

}
void css_logger_log_cb(uv_work_t* req)
{
	time_t timer;
	int i = 0;
	time(&timer);
	for (; i < mutil_test_count; i++) {
		CL_DEBUG(
				"log lager  %lld\n",
				timer);
	}
}

void css_logger_after_log_cb(uv_work_t* req, int status)
{
	free(req);
	mutil_test_clients--;
	if (mutil_test_clients == 0) {
		css_logger_close();

	}
}

/**
 * printf to console and dump to file.
 */
void test_css_logger_mutil()
{
	uv_loop_t *loop = uv_default_loop();
	int i;
	assert(0 == css_logger_init(loop));
	assert(0 == css_logger_init(loop));
	css_logger_start();
	css_logger_start();
	css_logger.no_console = 1;
	for (i = 0; i < mutil_test_clients; i++) {
		uv_work_t *work_req = (uv_work_t*) malloc(sizeof(uv_work_t));
		uv_queue_work(loop, work_req, css_logger_log_cb,
				css_logger_after_log_cb);
	}
	uv_run(loop, UV_RUN_DEFAULT);
	assert(0 == css_logger_destroy());
}

void test_css_logger_fmt_replace_enter()
{
	char *fmt;
	assert(-1 == css_logger_fmt_replace_enter(&fmt,"test."));
	assert(0 == css_logger_fmt_replace_enter(&fmt,"test\n"));
	assert(strcmp(fmt,"test\r\n") == 0);
	assert(0 == css_logger_fmt_replace_enter(&fmt,"te\ns  t\n"));
	assert(strcmp(fmt,"te\r\ns  t\r\n") == 0);
}

void test_css_logger_suite()
{
	test_css_logger_gen_fmt();
	test_css_logger_fmt_replace_enter();
	test_css_logger_console_log();
	test_css_logger_mutil();

}
#endif
