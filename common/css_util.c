#include "uv/uv.h"
#include <string.h>
#include "css_util.h"
#include <stdio.h>
#include <ctype.h>
#include "css_logger.h"

int ensure_dir(const char *sPathName)
{
	char DirName[MAX_PATH];
	int i, len = 0;
	strcpy(DirName, sPathName);
	len = (int)strlen(DirName);
	if (DirName[len - 1] != '/')
		strcat(DirName, "/");

	len = (int)strlen(DirName);

	for (i = 1; i < len; i++) {
		if (DirName[i] == '/' || DirName[i] == '\\') {
			DirName[i] = 0;
//			printf("%s\n", DirName);
			if (ACCESS(DirName, 0) != 0) {
				if (MKDIR(DirName) != 0) {
					printf("mkdir %s error %d\n", DirName, GETLASTERROR);
					return -1;
				}
			}
			DirName[i] = '/';
		}
	}

	return 0;
}

#ifdef _WIN32

int gettimeofday(struct timeval *tp, void* notuse/*NULL*/)
{
	uint64_t intervals;
	FILETIME ft;

	GetSystemTimeAsFileTime(&ft);

	/*
	 * A file time is a 64-bit value that represents the number
	 * of 100-nanosecond intervals that have elapsed since
	 * January 1, 1601 12:00 A.M. UTC.
	 *
	 * Between January 1, 1970 (Epoch) and January 1, 1601 there were
	 * 134744 days,
	 * 11644473600 seconds or
	 * 11644473600,000,000,0 100-nanosecond intervals.
	 *
	 * See also MSKB Q167296.
	 */

	intervals = ((uint64_t) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	intervals -= 116444736000000000;

	tp->tv_sec = (long) (intervals / 10000000);
	tp->tv_usec = (long) ((intervals % 10000000) / 10);
	return 0;
}
#else
char* itoa(int value, char* result, int base)
{
	if (base < 2 || base > 36) {
		*result = '\0';
		return result;
	}
	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;
	do {
		tmp_value = value;
		value /= base;
		*ptr++ =
				"zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35
						+ (tmp_value - value * base)];
	} while (value);
	if (tmp_value < 0)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}
#endif

char* lltoa(int64_t value, char *string, int radix)
{
	int64_t tmp_value;
	char* ptr = string, *ptr1 = string, tmp_char;
	if (radix < 2 || radix > 36) {
		*string = '\0';
		return string;
	}
	do {
		tmp_value = value;
		value /= radix;
		*ptr++ =
				"zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35
						+ (tmp_value - value * radix)];
	} while (value);
	if (tmp_value < 0)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return string;
}

long difftimeval(struct timeval* tv1, struct timeval* tv0)
{
	int64_t t0, t1;
	t0 = tv0->tv_sec * 1000 + tv0->tv_usec / 1000;
	t1 = tv1->tv_sec * 1000 + tv1->tv_usec / 1000;
	return (long) (0xffffffff & (t1 - t0));
}

int timeval_to_svtime(sv_time_t* sv, const struct timeval* tp)
{
	int ret = 0;
	time_t time32 = tp->tv_sec;
	struct tm *t;

	t = localtime(&time32);

	sv->year = t->tm_year + 1900;
	sv->month = t->tm_mon + 1;
	sv->day = t->tm_mday;
	sv->milliSeconds = (t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec) * 1000
			+ tp->tv_usec / 1000;
	return ret;
}

/* file name format: "yyyymmddhhmmssmmm" */
int timeval_to_filename(const struct timeval* tp, char* filename, int len)
{
	time_t time32 = tp->tv_sec;
	struct tm *t;

	if (len < 18) {
		return UV_EAI_MEMORY;
	}

	t = localtime(&time32);
	sprintf(filename, "%04d%02d%02d%02d%02d%02d%03d", t->tm_year + 1900,
			t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,
			tp->tv_usec / 1000);
	return 0;
}

int timeval_to_svtime_string(const struct timeval* tp, char* str, int len)
{
	time_t time32 = tp->tv_sec;
	struct tm *t;

	if (len < 24) {
		return UV_EAI_MEMORY;
	}

	t = localtime(&time32);
	sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d.%03d", t->tm_year + 1900,
			t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,
			tp->tv_usec / 1000);
	return 0;
}

int svtime2tm(struct tm* t, sv_time_t* sv)
{
	memset(t, 0, sizeof(struct tm));
	t->tm_year = sv->year - 1900;
	t->tm_mon = sv->month -1;
	t->tm_mday = sv->day;
	t->tm_hour = sv->milliSeconds / 1000 / 3600;
	t->tm_min = ((sv->milliSeconds / 1000) % 3600) / 60;
	t->tm_sec = (sv->milliSeconds / 1000) % 60;
	return 0;
}

/* sv1 - sv0 => ms */
long diffsvtime(sv_time_t* sv1, sv_time_t* sv0)
{
	long result, ms;
	struct tm t1, t0;
	time_t time32_1, time32_0;

	svtime2tm(&t1, sv1);
	svtime2tm(&t0, sv0);

	ms = (sv1->milliSeconds % 1000) - (sv0->milliSeconds % 1000);
	time32_1 = mktime(&t1);
	time32_0 = mktime(&t0);
	result = (time32_1 - time32_0) * 1000;
	result += ms;

	return result;
}

/* sv + ms => status:0:succ; else failse */
int svtimeaddms(sv_time_t* sv, long ms)
{
	struct tm t, *rt;
	time_t time;
	long a = 0, r;

	r = (sv->milliSeconds % 1000) + (ms % 1000);
	a = r / 1000;
	r %= 1000;
	svtime2tm(&t, sv);
	t.tm_sec = t.tm_sec + (ms / 1000) + a;
	time = mktime(&t);
	rt = localtime(&time);

	sv->year = rt->tm_year + 1900;
	sv->month = rt->tm_mon + 1;
	sv->day = rt->tm_mday;
	sv->milliSeconds = (rt->tm_hour * 3600 + rt->tm_min * 60 + rt->tm_sec) * 1000 + r;
	return 0;
}

/* sv - ms => status:0:succ; else failse */
int svtimesubms(sv_time_t* sv, long ms)
{
	struct tm t, *rt;
	time_t time;
	long a = 0, r;

	r = (sv->milliSeconds % 1000) - (ms % 1000);
	if(r < 0){
		a = -1;
		r += 1000;
	}
	svtime2tm(&t, sv);
	t.tm_sec = t.tm_sec + (ms / 1000) + a;
	time = mktime(&t);
	rt = localtime(&time);

	sv->year = rt->tm_year + 1900;
	sv->month = rt->tm_mon + 1;
	sv->day = rt->tm_mday;
	sv->milliSeconds = (rt->tm_hour * 3600 + rt->tm_min * 60 + rt->tm_sec) * 1000 + r;
	return 0;
}

/**
 * string util
 */
int str_contains(const char *haystack, const char *needle)
{
	return (char*) strstr(haystack, needle) ? 1 : 0;
}

/*
 Gets the offset of one string in another string
 */
int str_index_of(const char *a, char *b)
{
	char *offset = (char*) strstr(a, b);
	return offset ? (int)(offset - a) : -1;
}
/**
 * change string to upper.
 */
void str_to_upper(char *str)
{
	char *s = str;
	while (*str)
		*s++ = toupper(*str++);
}
/**
 * change string to lower.
 */
void str_to_lower(char *str)
{
	char *s = str;
	while (*str)
		*s++ = tolower(*str++);
}

int str_ltrim(char* str)
{
	char* s = str;
	int len = 0;
	CHECK_NULL_TO_RETURN(str, -1);
	while (is_space(*s)) {
		s++;
	}
	if (*s) {
		len = (int)strlen(s);
		if (s != str) {
			memmove(str, s, len);
			str[len] = '\0';
		}
	} else
		str[0] = '\0';
	return 0;
}
int str_rtrim(char* str)
{
	int size = (int)strlen(str);
	if (size > 0) {
		while (is_space(str[size - 1]) && size > 0) {
			size--;
		}
	}
	str[size] = '\0';
	return 0;
}

int str_trim(char* str)
{
	str_ltrim(str);
	str_rtrim(str);
	return 0;
}

//lorr : Left 0;Right 1
//sep : separative sign
int get_split_str(char* str, char* sep, int lorr, char** outStr)
{
	char* s = NULL;
	int s_start = 0, s_end = 0;
	if (strstr(str, sep) == NULL) {
		(*outStr) = s;
		return -1;
	}
	if (lorr == 1) {
		s_start = (int)(strlen(str) - strlen(strstr(str, sep)) + strlen(sep));
		s_end = (int)strlen(str);
	} else {
		s_end = (int)(strlen(str) - strlen(strstr(str, sep)));
	}
	s = (char*) malloc(s_end - s_start + 1);
	strncpy(s, str + s_start, s_end - s_start);
	s[s_end - s_start] = '\0';
	str_trim(s);
	(*outStr) = s;
	return 0;
}

//get sep left and right str,remove sep
int get_split_strs(char* str, char* sep, char** outLStr, char** outRStr)
{
	get_split_str(str, sep, 0, outLStr);
	get_split_str(str, sep, 1, outRStr);
	return 0;
}

int is_newline(char c)
{
	return ('\n' == c || '\r' == c) ? 1 : 0;
}
int is_end(char c)
{
	return '\0' == c ? 1 : 0;
}
int is_eof(char c)
{
	return EOF == c ? 1 : 0;
}
int is_space(char c)
{
	return ' ' == c ? 1 : 0;
}

int readLine(FILE* file, char** outStr)
{
	char buf[BUFSIZ];
	int i = 0;
	buf[i] = fgetc(file);
	if (is_eof(buf[i])) {
		return -1;
	}
	while (!is_eof(buf[i]) && !is_newline(buf[i])) {
		i++;
		buf[i] = fgetc(file);
	}
	buf[i] = '\0';
	COPY_STR((*outStr), buf);
	return i;
}

/*
 * TEST
 */
#ifdef CSS_TEST
#include <assert.h>
#include "css_ini_file.h"
void test_css_util_time(void)
{
	struct timeval tv0, tv1;

	struct timeval tv;
	sv_time_t sv;
	time_t tt;
	char str[24];

	gettimeofday(&tv0, NULL);
	Sleep(100);
	gettimeofday(&tv1, NULL);

	printf("diff0: %ld\n", difftimeval(&tv1, &tv0));
	tv0.tv_usec -= 999000;
	printf("diff1: %ld\n", difftimeval(&tv1, &tv0));
	tv1.tv_sec--;
	printf("diff2: %ld\n", difftimeval(&tv1, &tv0));
	tv1.tv_sec += 10;
	printf("diff3: %ld\n", difftimeval(&tv1, &tv0));

	gettimeofday(&tv, NULL);
	time(&tt);
	timeval_to_svtime(&sv, &tv);
	timeval_to_svtime_string(&tv, str, 24);
	printf("%hd-%02hd-%02hd %d\n", sv.year, (short)sv.month, (short)sv.day, sv.milliSeconds);
	printf("%s\n", str);

}

void test_css_util_other()
{
	char str[24];
	char test[10];
	char* s = NULL;
	char* t = NULL;
	assert(strcmp("922337203685477587",lltoa(922337203685477587,str,10)) == 0);

	strcpy(test," abc");
	str_trim(test);
	assert( 0 == strcmp(test,"abc"));

	strcpy(test," abc ");
	str_trim(test);
	assert( 0 == strcmp(test,"abc"));

	strcpy(test,"abc ");
	str_trim(test);
	assert( 0 == strcmp(test,"abc"));

	strcpy(test,"  a b c ");
	str_trim(test);
	assert( 0 == strcmp(test,"a b c"));

	strcpy(test,"   ");
	str_trim(test);
	assert( 0 == strcmp(test,""));

	strcpy(test," abc");
	str_ltrim(test);
	assert( 0 == strcmp(test,"abc"));

	strcpy(test," abc ");
	str_rtrim(test);
	assert( 0 == strcmp(test," abc"));

	strcpy(test," abc ");
	str_ltrim(test);
	assert( 0 == strcmp(test,"abc "));

	strcpy(test,"  a b c ");
	str_ltrim(test);
	assert( 0 == strcmp(test,"a b c "));

	strcpy(test,"   ");
	str_ltrim(test);
	assert( 0 == strcmp(test,""));

	strcpy(test,"   ");
	str_rtrim(test);
	assert( 0 == strcmp(test,""));

	memset(test,' ',10);
	strcpy(test,"a=b");
	get_split_str(test,"=",1,&s);
	assert( 0 == strcmp(s,"b"));
	FREE(s);
	get_split_str(test,"=",0,&s);
	assert( 0 == strcmp(s,"a"));
	FREE(s);
	get_split_strs(test,"=",&s,&t);
	assert( 0 == strcmp(s,"a"));
	assert( 0 == strcmp(t,"b"));
	FREE(s);
	FREE(t);

	memset(test,' ',10);
	strcpy(test," a = b ");
	get_split_str(test,"=",1,&s);
	str_trim(s);
	assert( 0 == strcmp(s,"b"));
	FREE(s);

	get_split_str(test,"=",0,&s);
	str_trim(s);
	assert( 0 == strcmp(s,"a"));
	FREE(s);

	get_split_strs(test,"=",&s,&t);
	str_trim(s);
	str_trim(t);
	assert( 0 == strcmp(s,"a"));
	assert( 0 == strcmp(t,"b"));
	FREE(s);
	FREE(t);

	assert(0 == ensure_dir("./test/dir"));
	assert(0 == ensure_dir("./test/dir/"));
}

void test_css_util_stringx()
{
	char *str;
	assert(1 == str_contains("hello word","wor"));
	assert(0 == str_contains("hello word","worw"));
	assert(2 == str_index_of("hello word","ll"));
	assert(-1 == str_index_of("hello word","lldd"));

	COPY_STR(str,"2abc123");
	str_to_upper(str);
	assert(strcmp(str,"2ABC123") == 0);
	str_to_lower(str);
	assert(strcmp(str,"2abc123") == 0);
}

void test_css_util_readFile()
{
	FILE* config_file = NULL;
	char* s = NULL;
	config_file = fopen(TEST_CONFIG_FILE, "r");
	assert(NULL != config_file);
	while(EOF != readLine(config_file,&s)) {
		FREE(s);
	}

}

void test_css_util_suite()
{
	test_css_util_time();
	test_css_util_other();
	test_css_util_stringx();
	test_css_util_readFile();
}

#endif
