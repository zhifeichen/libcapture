/*
 * css_ini_file.h
 *
 *  Created on: 2014?7?29?
 *      Author: zj14
 */

#ifndef CSS_INI_FILE_H_
#define CSS_INI_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif


#define SECTION_DB "db"
#define SECTION_DISK "disk"

#define DEFAULT_CONFIG_FILE "../config.ini"
#define TEST_CONFIG_FILE "../test/test_config.ini"

#define CHECK_LOAD_INI_FILE() if(m_css_config == NULL)	\
								{	\
									printf("please load the config file!! \n");	\
									return -1;	\
								}

typedef struct {
	char* keyName;
	char* value;
} css_config_env;

typedef struct {
	char* setctionName;
	int   keyCount;
	css_config_env* config;
} css_config;


int css_load_ini_file(const char* file_path);
void css_destory_ini_file();
int css_get_env(char* secName,char* keyName,char* defaultValue,char** value);

#ifdef __cplusplus
}
#endif

#endif /* CSS_INI_FILE_H_ */
