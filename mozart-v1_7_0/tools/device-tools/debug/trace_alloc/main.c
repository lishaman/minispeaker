#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include "trace_alloc.h"

#define DEBUG 0
#if DEBUG
#define pr_debug(fmt, args...)			\
	printf(fmt, ##args)
#else
#define pr_debug(fmt, args...)			\
	do {} while(0)
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define barrier() __asm__ __volatile__("" ::: "memory")

static int check_entry(struct malloc_node *entry, void *ptr,
		       unsigned int ra, enum malloc_t type)
{
	if (!entry || entry->addr != ptr)
		return 0;
#ifndef MIPS
	if (ra && entry->ra != ra)
		return 0;
#endif
	if (entry->type != type)
		return 0;

	return 1;
}

static int check_list_num(unsigned int num)
{
	if (trace_alloc_get_entry(num) != NULL ||
	    trace_alloc_get_free_num() != MALLOC_NUM - num)
		return 0;

	return 1;
}

static int check_one_entry(void *ptr, enum malloc_t type)
{
	struct malloc_node *entry;

	pr_debug("%s...\n", __func__);

	entry = trace_alloc_get_entry(0);
	if (!check_entry(entry, ptr, ~(unsigned int)ptr, type))
		return 0;

	if (!check_list_num(1))
		return 0;

	return 1;
}

struct test_case {
	char name[128];
	void *data[MALLOC_NUM + 1];
	int result;
	void (* run)(struct test_case *);
	/* return 1, or 0 if an error occurred. */
	int  (* check)(struct test_case *);
	void (* error)(struct test_case *);
	void (* cleanup)(struct test_case *);
};

static int check_list_empty(struct test_case *test)
{
	pr_debug("%s...\n", __func__);

	return trace_alloc_get_entry(0) == NULL &&
		trace_alloc_get_free_num() == MALLOC_NUM;
}

static void run_malloc(struct test_case *test)
{
	test->data[0] = malloc(128);
	memset(test->data[0], 0, 128);
}

static int check_malloc(struct test_case *test)
{
	return check_one_entry(test->data[0], malloc_type);
}

static void error_malloc(struct test_case *test)
{
	printf("--malloc ptr = %p\n", test->data[0]);
}

static void run_free(struct test_case *test)
{
	fake_free((void *)0x5a5a0000);
}

static int check_free(struct test_case *test)
{
	return check_one_entry((void *)0x5a5a0000, fake_free_type);
}

static void error_free(struct test_case *test)
{
	printf("--fake free ptr = %p\n", (void *)0x5a5a0000);
}

static void run_free_null(struct test_case *test)
{
	free(NULL);
}

static void run_match(struct test_case *test)
{
	void *ptr = malloc(128);
	memset(ptr, 0, 128);
	free(ptr);
}

static void run_malloc2(struct test_case *test)
{
	test->data[0] = malloc(128);
	memset(test->data[0], 0, 128);

	test->data[1] = malloc(128);
	memset(test->data[1], 0, 128);
}

static int check_malloc2(struct test_case *test)
{
	struct malloc_node *entry;

	entry = trace_alloc_get_entry(0);;
	if (!check_entry(entry, test->data[0],
			 ~(unsigned int)test->data[0], malloc_type))
		return 0;

	entry = trace_alloc_get_entry(1);
	if (!check_entry(entry, test->data[1],
			 ~(unsigned int)test->data[1], malloc_type))
		return 0;

	if (!check_list_num(2))
		return 0;

	return 1;
}

static void error_malloc2(struct test_case *test)
{
	printf("--malloc ptr = %p\n", test->data[0]);
}

static void run_mismatch_malloc(struct test_case *test)
{
	test->data[0] = malloc(128);
	memset(test->data[0], 0, 128);

	test->data[1] = malloc(128);
	memset(test->data[1], 0, 128);

	free(test->data[0]);
	test->data[0] = NULL;
}

static int check_mismatch_malloc(struct test_case *test)
{
	return check_one_entry(test->data[1], malloc_type);
}

static void error_mismatch_malloc(struct test_case *test)
{
	printf("--malloc ptr = %p\n", test->data[1]);
}

static void run_mismatch_free(struct test_case *test)
{
	test->data[0] = malloc(128);
	memset(test->data[0], 0, 128);
	test->data[1] = malloc(128);
	memset(test->data[1], 0, 128);

	free(test->data[0]);
	test->data[0] = NULL;
	free(test->data[1]);
	test->data[1] = NULL;
	fake_free((void *)0x12345600);
}

static int check_mismatch_free(struct test_case *test)
{
	return check_one_entry((void *)0x12345600, fake_free_type);
}

static void error_mismatch_free(struct test_case *test)
{
	printf("--fake free ptr = %p\n", (void *)0x12345000);
}

static void run_malloc_overmuch(struct test_case *test)
{
	int i;

	for (i = 0; i < MALLOC_NUM + 1; i++) {
		test->data[i] = malloc(128);
		memset(test->data[i], 0, 128);
	}
}

static int check_malloc_overmuch(struct test_case *test)
{
	return check_one_entry(test->data[MALLOC_NUM], malloc_type);
}

static void error_malloc_overmuch(struct test_case *test)
{
	printf("--malloc ptr[%d] = %p\n", MALLOC_NUM, test->data[MALLOC_NUM]);
}

static void run_calloc(struct test_case *test)
{
	test->data[0] = calloc(1, 128);
	memset(test->data[0], 0, 128);
}

static int check_calloc(struct test_case *test)
{
	return check_one_entry(test->data[0], calloc_type);
}

static void error_calloc(struct test_case *test)
{
	printf("--calloc ptr = %p\n", test->data[0]);
}

static void run_strdup(struct test_case *test)
{
	test->data[0] = strdup("hello word!");
}

static int check_strdup(struct test_case *test)
{
	return check_one_entry(test->data[0], malloc_type);
}

static void error_strdup(struct test_case *test)
{
	printf("--strdup ptr = %p\n", test->data[0]);
}

static void run_strdup_null(struct test_case *test)
{
	test->data[0] = strdup("");
}

static int check_strdup_null(struct test_case *test)
{
	return check_one_entry(test->data[0], calloc_type);
}

static void run_strdup_no_builtin(struct test_case *test)
{
	int i;
	char s[33];

	for (i = 0; i < 32; i++)
		s[i] = random() % 26 + 'a';
	s[32] = '\0';
	test->data[0] = strdup(s);
}

static int check_strdup_no_builtin(struct test_case *test)
{
	return check_one_entry(test->data[0], __strdup_type);
}

static void error_strdup_no_builtin(struct test_case *test)
{
	printf("--__strdup ptr = %p\n", test->data[0]);
}

static void run_strdup_without_optimization(struct test_case *test)
{
#ifdef __OPTIMIZE__
#pragma push_macro("strdup")
#undef strdup
	test->data[0] = strdup("optimization");
#pragma pop_macro("strdup")
#else
	printf("[WARNNING] %s: Please use compiler flags -O2!\n", test->name);
#endif
}

static int check_strdup_without_optimization(struct test_case *test)
{
	return check_one_entry(test->data[0], strdup_type);
}

static void error_strdup_without_optimization(struct test_case *test)
{
	printf("--strdup ptr = %p\n", test->data[0]);
}

static void run_realloc(struct test_case *test)
{
	test->data[0] = malloc(128);
	memset(test->data[0], 0, 128);

	test->data[1] = realloc(test->data[0], 128);
	test->data[0] = NULL;
	memset(test->data[1], 0, 128);
}

static int check_realloc(struct test_case *test)
{
	return check_one_entry(test->data[1], realloc_type);
}

static void error_realloc(struct test_case *test)
{
	printf("--realloc ptr = %p\n", test->data[1]);
}

static void run_realloc_null(struct test_case *test)
{
	test->data[1] = realloc(NULL, 128);
	memset(test->data[1], 0, 128);
}

static void run_realloc_free(struct test_case *test)
{
	test->data[0] = realloc(NULL, 128);
	memset(test->data[0], 0, 128);
	free(test->data[0]);
	test->data[0] = NULL;
}

static void run_memalign(struct test_case *test)
{
	test->data[0] = memalign(2048, 128);
	memset(test->data[0], 0, 128);
}

static int check_memalign(struct test_case *test)
{
	return check_one_entry(test->data[0], memalign_type);
}

static void error_memalign(struct test_case *test)
{
	printf("--memalign ptr = %p\n", test->data[0]);
}

static void run_posix_memalign(struct test_case *test)
{
	int ret = posix_memalign(&test->data[0], 2048, 128);
	if (ret) {
		printf("posix_memalign: %s\n", strerror(ret));
		return ;
	}
	memset(test->data[0], 0, 128);
}

static int check_posix_memalign(struct test_case *test)
{
	return check_one_entry(test->data[0], posix_memalign_type);
}

static void run_valloc(struct test_case *test)
{
	test->data[0] = valloc(128);
	memset(test->data[0], 0, 128);
}

static int check_valloc(struct test_case *test)
{
	return check_one_entry(test->data[0], valloc_type);
}

static void error_valloc(struct test_case *test)
{
	printf("--valloc ptr = %p\n", test->data[0]);
}

#ifdef MIPS
static void fake_free_1(void) __attribute__((noinline));
static void fake_free_2(void) __attribute__((noinline));
static void fake_free_3(void) __attribute__((noinline));

static void fake_free_1(void)
{
	fake_free((void *)~0x1111);
}

static void fake_free_2(void)
{
	fake_free((void *)~0x2222);
}

static void fake_free_3(void)
{
	fake_free((void *)~0x3333);
}
#endif

static void run_sort(struct test_case *test)
{
	int i;

#ifdef MIPS
	fake_free_2();
	fake_free_1();
	fake_free_1();
	fake_free_2();
	fake_free_3();
	fake_free_1();
#else
	for (i = 0; i < 9; i++)
		fake_free((void *)~0x9999);
	for (i = 0; i < 5; i++)
		fake_free((void *)~0x5555);
	for (i = 0; i < 1; i++)
		fake_free((void *)~0x1111);
	for (i = 0; i < 3; i++)
		fake_free((void *)~0x3333);
	for (i = 0; i < 2; i++)
		fake_free((void *)~0x2222);
	for (i = 0; i < 6; i++)
		fake_free((void *)~0x6666);
	for (i = 0; i < 2; i++)
		fake_free((void *)~0xbbbb);
	for (i = 0; i < 7; i++)
		fake_free((void *)~0x7777);
	for (i = 0; i < 8; i++)
		fake_free((void *)~0x8888);
	for (i = 0; i < 4; i++)
		fake_free((void *)~0x4444);
	for (i = 0; i < 1; i++)
		fake_free((void *)~0xaaaa);
#endif
}

static int check_sort(struct test_case *test)
{
	struct malloc_node counter[3];

	trace_alloc_sort(counter, 3);

	if (counter[0].addr != (void *)0x9)
		return 0;
	if (counter[1].addr != (void *)0x8)
		return 0;
	if (counter[2].addr != (void *)0x7)
		return 0;

	return 1;
}

static void error_sort(struct test_case *test)
{
	printf("fake_free 9 9 times.\n");
	printf("fake_free 8 8 times.\n");
	printf("fake_free 7 7 times.\n");
}

static struct test_case trace_alloc_test_case[] = {
	{
		.name = "Nothing Test",
		.check = check_list_empty,
	},
	{
		.name = "Malloc Test",
		.run = run_malloc,
		.check = check_malloc,
		.error = error_malloc,
	},
	{
		.name = "Free Test",
		.run = run_free,
		.check = check_free,
		.error = error_free,
	},
	{
		.name = "Free NULL Test",
		.run = run_free_null,
		.check = check_list_empty,
	},
	{
		.name = "Match Test",
		.run = run_match,
		.check = check_list_empty,
	},
	{
		.name = "Malloc2 Test",
		.run = run_malloc2,
		.check = check_malloc2,
		.error = error_malloc2,
	},
	{
		.name = "Mismatch Malloc Test",
		.run = run_mismatch_malloc,
		.check = check_mismatch_malloc,
		.error = error_mismatch_malloc,
	},
	{
		.name = "Mismatch Free Test",
		.run = run_mismatch_free,
		.check = check_mismatch_free,
		.error = error_mismatch_free,
	},
	{
		.name = "Malloc Overmuch Test",
		.run = run_malloc_overmuch,
		.check = check_malloc_overmuch,
		.error = error_malloc_overmuch,
	},
	{
		.name = "Calloc Test",
		.run = run_calloc,
		.check = check_calloc,
		.error = error_calloc,
	},
	{
		.name = "Strdup Test",
		.run = run_strdup,
		.check = check_strdup,
		.error = error_strdup,
	},
	{
		.name = "Strdup Null Test",
		.run = run_strdup_null,
		.check = check_strdup_null,
		.error = error_strdup,
	},
	{
		.name = "Strdup No-Builtin Test",
		.run = run_strdup_no_builtin,
		.check = check_strdup_no_builtin,
		.error = error_strdup_no_builtin,
	},
	{
		.name = "Strdup Without Optimization Test",
		.run = run_strdup_without_optimization,
		.check = check_strdup_without_optimization,
		.error = error_strdup_without_optimization,
	},
	{
		.name = "Realloc Test",
		.run = run_realloc,
		.check = check_realloc,
		.error = error_realloc,
	},
	{
		.name = "Realloc Null Test",
		.run = run_realloc_null,
		.check = check_realloc,
		.error = error_realloc,
	},
	{
		.name = "Realloc Free Test",
		.run = run_realloc_free,
		.check = check_list_empty,
	},
	{
		.name = "Memalign Test",
		.run = run_memalign,
		.check = check_memalign,
		.error = error_memalign,
	},
	{
		.name = "Posix Memalign Test",
		.run = run_posix_memalign,
		.check = check_posix_memalign,
		.error = error_memalign,
	},
	{
		.name = "Valloc Test",
		.run = run_valloc,
		.check = check_valloc,
		.error = error_valloc,
	},
	{
		.name = "Sort Test",
		.run = run_sort,
		.check = check_sort,
		.error = error_sort,
	},
};

int test_template(struct test_case *test)
{
	int i;

	pr_debug("\n>>>> [Test Case] %s <<<<\n", test->name);

	trace_alloc_reset();
	barrier();

	pr_debug("%s run\n", test->name);
	if (test->run)
		test->run(test);

	pr_debug("%s check\n", test->name);
	if (!test->check(test)) {
		printf("**** [ERROR] %s error!!! ****\n", test->name);
		if (test->error)
			test->error(test);
		trace_alloc_result();
		return 1;
	}

	pr_debug("%s cleanup\n", test->name);
	for (i = 0; i < MALLOC_NUM + 1; i++) {
		if (test->data[i]) {
			free(test->data[i]);
			test->data[i] = NULL;
		}
	}

	return 0;
}

int main(int argc, char** argv)
{
	int i;
	struct test_case *test;

	printf("\n=======> trace_alloc test(%5d) <============\n", getpid());

	for (i = 0;i < ARRAY_SIZE(trace_alloc_test_case); i++) {
		test = &trace_alloc_test_case[i];
		test->result = test_template(test);
	}

	printf("\nResult:\n");
	for (i = 0;i < ARRAY_SIZE(trace_alloc_test_case); i++) {
		test = &trace_alloc_test_case[i];
		printf("    %s    %s\n", test->result ? "FAIL" : "OK  ",
			test->name);
	}

	printf("===========  trace_alloc over  ===============\n\n");

	return 0;
}
