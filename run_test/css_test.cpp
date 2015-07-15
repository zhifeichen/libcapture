#include "css_test.h"

css_test_case_t suite[] = {
	CSS_TEST_CASES(CTEST_ENTITY) { 0, 0, 0 } };

void css_do_test_by(char *name)
{
	int i = 0, ignore = 0, active = 0;
	struct timeval st, et;
	long cost, total_cost = 0;
	PRINTF_CSS_DO_TEST_HEADER()
		;
	while (suite[i].name != NULL) {
		if (strcmp(name, suite[i].name) == 0) {
			gettimeofday(&st, NULL);
			PRINTF_CSS_TEST_CAST_HEADER(suite[i].name);
			suite[i].main();
			gettimeofday(&et, NULL);
			cost = difftimeval(&et, &st);
			total_cost += cost;
			PRINTF_CSS_TEST_CASE_TAIL(suite[i].name, cost);
			active++;
			break;
		}
		i++;
	}
	PRINTF_CSS_DO_TEST_TAIL(1, active, 1 - active, total_cost);
}

void css_do_test_all()
{
	int i = 0, ignore = 0, active = 0;
	struct timeval st, et;
	long cost, total_cost = 0;
	PRINTF_CSS_DO_TEST_HEADER()
		;
	while (suite[i].main != NULL) {
        if (suite[i].active) {
            gettimeofday(&st, NULL);
            PRINTF_CSS_TEST_CAST_HEADER(suite[i].name);
            suite[i].main();
            gettimeofday(&et, NULL);
            cost = difftimeval(&et, &st);
            total_cost += cost;
            PRINTF_CSS_TEST_CASE_TAIL(suite[i].name, cost);
            active++;
        } else {
            ignore++;
        }
		i++;
	}
	PRINTF_CSS_DO_TEST_TAIL(i, active, ignore, total_cost);
}

void test_test_case(void)
{
	printf("start test case test body.\n");
	printf("start test case test body.\n");
	printf("start test case test body.\n");
	printf("start test case test body.\n");
}