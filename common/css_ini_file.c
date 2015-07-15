/*
 * css_ini_file.c
 *
 *  Created on: 2014?7?29?
 *      Author: zj14
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "css_ini_file.h"
#include "css_util.h"
#include "assert.h"
#include "css_logger.h"

#define MAX_FILE_SIZE 4096
#define LEFT_BRACE '['
#define RIGHT_BRACE ']'
#define ANNOTATE_SEMICOLON ';'
#define ANNOTATE_NUMBER_SIG '#'
#define MAX_SECMENT_LEN 99
#define MAX_KEY_VALUE_OF_SECMENT 99

static css_config* m_css_config;
static int css_config_count = 0;

void css_destory_ini_file()
{
	int i = 0, j = 0;
	CL_INFO("destory config file.\n");
	for (; i < css_config_count; i++) {
//		printf("%d section : %s , keys : %d \n", i,
//				m_css_config[i].setctionName, m_css_config[i].keyCount);
		for (j = 0; j < m_css_config[i].keyCount; j++) {
//			printf("%d.%d %s : %s \n", i, j, m_css_config[i].config[j].keyName,
//					m_css_config[i].config[j].value);
			FREE(m_css_config[i].config[j].keyName);
			FREE(m_css_config[i].config[j].value);
		}
		FREE(m_css_config[i].setctionName);
		FREE(m_css_config[i].config);
	}
	FREE(m_css_config);
}

static int is_left_barce(char c)
{
	return LEFT_BRACE == c ? 1 : 0;
}
//static int is_right_brace(char c)
//{
//	return RIGHT_BRACE == c ? 1 : 0;
//}
/**
 * already ignore space character.
 */
static int is_annotate_line(const char* str)
{
	return (*str == ANNOTATE_SEMICOLON || *str == ANNOTATE_NUMBER_SIG) ? 1 : 0;
}
/**
 * already ignore space character.
 */
static int is_keyvalue_line(const char* str)
{

	return (strstr(str, "=") && ((*str) != '=')) ? 1 : 0;
}
/**
 * already ignore space character.
 */
static int is_segment_line(const char* str)
{
	if (strstr(str, "]") == NULL) {
		return 0;
	}
	return (is_left_barce(*str) && strlen(str) > 2) ? 1 : 0; //must "^[.]$"
}
/**
 * already ignore space character.
 */
static int get_key_value(char* buf, char** key, char** value)
{
	get_split_strs(buf, "=", key, value);
	str_trim(*key);
	str_trim(*value);
	return 0;
}

static int get_section_name(char* buf, char** secName)
{
	char* s = buf;
	char* name = NULL;
	int len = 0;
	while (is_space(*s) || is_left_barce(*s)) {
		s++;
	}
	len = (int)(strlen(s) - strlen(strstr(s, "]")));
	name = (char*) malloc(len + 1);
	memcpy(name, s, len);
	name[len] = '\0';
	(*secName) = name;
	return 0;
}

int css_load_ini_file(const char* file_path)
{
	FILE* config_file = NULL;
	int i = 0, secCount = -1, keyCount = 0;
	char* line = NULL;
	int sec_tag = 0;
	css_config_env* m_env;
	CL_INFO("start loading config file : %s \n", file_path);
	if ((config_file = fopen(file_path, "r")) == NULL) {
		CL_ERROR("config file : %s not found!! \n", file_path);
		return -1;
	}

	m_css_config = (css_config*) malloc(MAX_SECMENT_LEN * sizeof(css_config));
	memset(m_css_config, 0, MAX_SECMENT_LEN * sizeof(css_config));
	while (EOF != readLine(config_file, &line)) {
		str_trim(line);
		if (line != NULL && strlen(line) > 0) {
			if (is_annotate_line(line)) {
				//nothing to do,ignore.
			} else if (is_segment_line(line)) {
				if (sec_tag == 1) {
//					printf("%d.%d\n", secCount, keyCount);
//					realloc(m_css_config[secCount].config,
//							keyCount * sizeof(css_config_env));
//					assert(m_css_config[secCount].config!=NULL);
					m_css_config[secCount].keyCount = keyCount;
					sec_tag = 0;
				}
				secCount++;
				get_section_name(line, &(m_css_config[secCount].setctionName));
//				printf("%d.%s\n",secCount,m_css_config[secCount].setctionName);
				m_css_config[secCount].config = (css_config_env*) malloc(
				MAX_KEY_VALUE_OF_SECMENT * sizeof(css_config_env));
				memset(m_css_config[secCount].config, 0,
						MAX_KEY_VALUE_OF_SECMENT * sizeof(css_config_env));
				sec_tag = 1;
				keyCount = 0;
			} else if (is_keyvalue_line(line)) {
				m_env = &(m_css_config[secCount].config[keyCount]);
				get_key_value(line, &(m_env->keyName), &(m_env->value));
//				printf("%d.%d %s:%s\n",secCount,keyCount,m_env->keyName,m_env->value);
				keyCount++;
			} else {
				CL_WARN("read unkown line : %s\n", line);
			}
		}
		FREE(line);
	}
	if (sec_tag == 1) {
//		printf("%d.%d\n", secCount, keyCount);
//		realloc(m_css_config[secCount].config,
//				keyCount * sizeof(css_config_env));
		m_css_config[secCount].keyCount = keyCount;
		sec_tag = 0;
	}
	secCount++;
	css_config_count = secCount;
	CL_DEBUG("total read %d sections of config file : %s.\n", secCount,
			file_path);
	return 0;
}

//must after css_load_ini_file
int css_get_env(char* secName, char* keyName, char* defaultValue, char** value)
{
	int i = 0, j = 0;
	char* v = NULL;

	if (strlen(secName) > 0 && strlen(keyName) > 0) {
		for (; i < css_config_count; i++) {
			if (0 == strcmp(secName, m_css_config[i].setctionName)) {
				for (j = 0; j < m_css_config[i].keyCount; j++) {
					if (0
							== strcmp(keyName,
									m_css_config[i].config[j].keyName)) {
						COPY_STR(v, m_css_config[i].config[j].value);
						(*value) = v;
						return 0;
					}
				}
			}
		}
//		printf("not find,section:%s key:%s \n", secName, keyName);
		COPY_STR(v, defaultValue);
		(*value) = v;
		return 1;
	} else {
		printf("secName or keyName error!! \n");
		return -1;
	}

}


/*
 * TEST:
 */
#ifdef CSS_TEST
#include <assert.h>
void test_read_inifile(void)
{
	char ip[99] = {0};
	int i = 0, j = 0;
	char* test = NULL;
	css_db_config myDbConfig;
	css_disk_config myDiskConfig;

	strcpy(ip, ";123");
	assert(1 == is_annotate_line(ip));
	assert(0 == is_keyvalue_line(ip));
	assert(0 == is_segment_line(ip));

	strcpy(ip, "[123 ]");
	assert(0 == is_annotate_line(ip));
	assert(0 == is_keyvalue_line(ip));
	assert(1 == is_segment_line(ip));

	strcpy(ip, "4 =  2");
	assert(0 == is_annotate_line(ip));
	assert(1 == is_keyvalue_line(ip));
	assert(0 == is_segment_line(ip));

	strcpy(ip, "[]");
	assert(0 == is_annotate_line(ip));
	assert(0 == is_keyvalue_line(ip));
	assert(0 == is_segment_line(ip));

	strcpy(ip, "4=");
	assert(0 == is_annotate_line(ip));
	assert(1 == is_keyvalue_line(ip));
	assert(0 == is_segment_line(ip));

	strcpy(ip, "=4");
	assert(0 == is_annotate_line(ip));
	assert(0 == is_keyvalue_line(ip));
	assert(0 == is_segment_line(ip));

	printf("read inifile test start ... \n");

	assert(NULL == m_css_config);
	assert(0 == css_load_ini_file(TEST_CONFIG_FILE));
	assert(NULL != m_css_config);
	assert(0 == css_get_db_config(&myDbConfig));
	assert(0 == strcmp(myDbConfig.dbName, "test"));
	assert(DBTYPE_MYSQL == myDbConfig.dbType);
	assert(0 == strcmp(myDbConfig.host, "192.168.8.217"));
	assert(0 == strcmp(myDbConfig.passwd, "root"));
	assert(3306 == myDbConfig.port);
	assert(0 == strcmp(myDbConfig.user, "root"));
	assert(0 == css_get_disk_config(&myDiskConfig));
	assert(85 == myDiskConfig.max_per);
	assert(0 == strcmp(myDiskConfig.disk_paths, "C:;D:;E:"));

	css_get_env(SECTION_DB, "ttt", "ttt",
			&(test));
	assert(strcmp(test,"ttt") ==0);
	FREE(test);

	css_get_env(SECTION_DB, "host", "ttt",
			&(test));
	assert(strcmp(test,"192.168.8.217") ==0);
	FREE(test);

	css_destory_ini_file();
	assert(NULL == m_css_config);
	printf("read inifile test end! \n");
}

void test_read_inifile_suite()
{
	test_read_inifile();
}

#endif

