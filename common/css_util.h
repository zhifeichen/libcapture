#ifndef __CSS_UTIL_H__
#define __CSS_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <fcntl.h>
#include <stdio.h>

#ifdef _WIN32
# include <direct.h>
# include <io.h>
# ifndef S_IRUSR
#  define S_IRUSR _S_IREAD
# endif
# ifndef S_IWUSR
#  define S_IWUSR _S_IWRITE
# endif
# define unlink _unlink
# define rmdir _rmdir
//# define stat _stati64
//# define open _open
# define write _write
# define lseek _lseek
# define close _close
# define snprintf _snprintf
# define vsnprintf _vsnprintf
#else
# include <unistd.h> /* unlink, rmdir, etc. */
# define Sleep(T) sleep((T)/1000)
# ifndef O_BINARY
#  define O_BINARY 0
# endif
#endif

#ifndef MAX_PATH
# define MAX_PATH 256
#endif

#ifdef _WIN32
#include <time.h>
int gettimeofday(struct timeval *tp, void* /*NULL*/);
#define GETLASTERROR GetLastError()
#define MKDIR(path) _mkdir(path)
#define ITOA(x,y,z) _itoa((x),(y),(z))
#else
#include <sys/time.h>
#define MKDIR(path) mkdir((path),0755)
#define GETLASTERROR errno
char* itoa(int value, char *string, int radix);
#define ITOA(x,y,z) itoa((x),(y),(z))
#endif

#define FREE(x)										\
	if ( (x) != NULL){								\
		free((x));									\
		(x) = NULL;									\
	}

#define CHECK_CONFIG(x)								\
	if ( (x) == NULL){								\
		printf(#x  " is NULL,check it! \n");		\
		return -1;									\
	}

#define COPY_STR(to,from)							\
	if((from) !=NULL){								\
		(to) = (char*) malloc(strlen((from)) + 1);	\
		strcpy( (to), (from));						\
	}

#define CHECK_INIT(x,y)								\
	if ( ((x) != NULL) && ((x)->init_flag == 1)){	\
		printf(#y" was inited!!! \n");				\
		return -1;									\
	}

#define CHECK_NULL_TO_RETURN(x, ret) if((x)==NULL){return ret;}

#define CHECK_FALSE_TO_BREAK(x) if((x)!=0) break

#ifdef _WIN32
#define ATOLL(x) _atoi64(x)
#define ACCESS(x,y) _access((x),(y))
#else
#define ATOLL(x) atoll(x)
#define ACCESS(x,y) access((x),(y))
#endif // _WIN32

typedef struct sv_time_s
{
	uint16_t year; /*xxxx*/
	uint8_t month; // 1~12
	uint8_t day; // 1~31
	uint32_t milliSeconds;
}sv_time_t;

int ensure_dir(const char *path);

int str_ltrim(char* str);
int str_rtrim(char* str);
int str_trim(char* str);

int get_split_str(char* str, char* sep, int lorr, char** outStr);
int get_split_strs(char* str, char* sep, char** outLStr, char** outRStr);

int is_newline(char c);
int is_end(char c);
int is_space(char c);

int readLine(FILE* file, char** outStr);

/* diff timeval struct to milliseconds tv0: beginning time, tv1: ending time */
long difftimeval(struct timeval* tv1, struct timeval* tv0);

int timeval_to_svtime(sv_time_t* sv, const struct timeval* tp);

/* file name format: "yyyymmddhhmmssmmm" */
int timeval_to_filename(const struct timeval* tp, char* filename, int len);

/* string format: "yyyy-mm-dd hh:mm:ss.mmm" -> 24 byte*/
int timeval_to_svtime_string(const struct timeval* tp, char* str, int len);

int svtime2tm(struct tm* t, sv_time_t* sv);

/* sv1 - sv0 => ms */
long diffsvtime(sv_time_t* sv1, sv_time_t* sv0);

/* sv + ms => status:0:succ; else failse */
int svtimeaddms(sv_time_t* sv, long ms);

/* sv - ms => status:0:succ; else failse */
int svtimesubms(sv_time_t* sv, long ms);

char* lltoa(int64_t value, char *string, int radix);

/**
 *	Checks if one string contains another string
 */
int str_contains(const char *haystack, const char *needle);
/**
 *	Gets the offset of one string in another string
 */
int str_index_of(const char *a, char *b);
/**
 * change string to upper.
 */
void str_to_upper(char *str);
/**
 * change string to lower.
 */
void str_to_lower(char *str);

#ifdef __cplusplus
}
#endif

#endif
