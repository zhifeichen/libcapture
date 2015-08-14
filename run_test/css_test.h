/*
 * css_test.h
 *
 *  Created on: 上午11:04:31
 *      Author: ss
 */

#ifndef CSS_TEST_H_
#define CSS_TEST_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "css_util.h"
#include "uv/uv.h"


typedef void (*test_case)(void);

typedef enum
{
	TCASE_INACTIVE = 0, TCASE_ACTIVE
} test_case_active_t;

/**
 * add test func here!   func name must start with 'test_'
 * XX(name,active) like XX(name,0) or XX(name,TCASE_ACTIVE)
 */
#define CSS_TEST_CASES(XX)  				\
	XX(msgsock, TCASE_ACTIVE)				\
	XX(CaptureSysMsg, TCASE_ACTIVE)		    \
    XX(AudioDecoder, TCASE_ACTIVE)          \
    XX(VideoDecoder2, TCASE_ACTIVE)         \

/**
 * test imple
 */
#define TEST_IMPL(name)					\
	void test_##name(void);				\
	void test_##name(void)

/**
 * private
 */
typedef struct css_test_case_s
{
	char *name;
	test_case main;
	int active;
} css_test_case_t;

/**
 * declare all test case func.
 */
#define CTEST_DECLARE(name,_) \
	void test_##name(void);
CSS_TEST_CASES(CTEST_DECLARE)

/**
 * add test case to suite.
 */
#define CTEST_ENTITY(name,active) \
	{ #name, &test_##name, active },



#define PRINTF_CSS_TEST_CAST_HEADER(name)	\
	printf("Running test_case: test_%s\n---------------------------------------------------\n",(name))

#define PRINTF_CSS_TEST_CASE_TAIL(name,cost)		\
		printf("Test end of test case:%s , cost:%ldms.\n---------------------------------------------------\n\n",(name),(cost))

#define PRINTF_CSS_DO_TEST_HEADER()				 \
do {																	\
	printf("---------------------------------------------------\n");	\
	printf("   R U N N I N G    T E S T S \n");							\
	printf("---------------------------------------------------\n\n");	\
}while(0)

#define PRINTF_CSS_DO_TEST_TAIL(t,active,ignore,total)					\
do{																			\
	printf("\nTotal %d test case, active:%d, skipped:%d, total cost:%ldms.\n", (t), (active), (ignore), (total));  \
}while(0)

void css_do_test_by(char *name);

void css_do_test_all();

#endif /* CSS_TEST_H_ */
