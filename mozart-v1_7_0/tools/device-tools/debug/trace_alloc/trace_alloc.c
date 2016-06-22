#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "trace_alloc.h"

#define DEBUG 0
#if DEBUG
#define pr_debug(fmt, args...)			\
	printf(fmt, ##args)
#else
#define pr_debug(fmt, args...)			\
	do {} while(0)
#endif

static void __trace_alloc_result();
static void __trace_alloc_reset(void);
void trace_alloc_result(void);
void trace_alloc_reset(void);

unsigned long long global_index = 1;
static struct list_head g_list, free_list;
static struct list_head entry[MALLOC_NUM];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

volatile static unsigned init, dlsym_calloc;
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
static void *(* real_malloc)(size_t size);
static void *(* real_calloc)(size_t __nmemb, size_t __size);
static void *(* real_realloc)(void *__ptr, size_t __size);
static void *(* real_memalign)(size_t __alignment, size_t __size);
static int   (* real_posix_memalign)(void **__memptr, size_t __alignment, size_t __size);
static void *(* real_valloc)(size_t __size);
static void (* real_free)(void *ptr);

static FILE *filep = NULL;
static unsigned int log_count = 1;
#ifdef LOG_FILE
#ifdef TEST
char *log_file = "./trace_alloc_test_log";
static char *log_ctrl_file = "./trace_alloc_test_begin";
static char *log_file_prefix = "";
static char *log_ctrl_prefix = "";
#else  /* !TEST */
char log_file[64];
static char log_ctrl_file[64];
static char *log_file_prefix = "/mnt/sdcard/trace_alloc/trace_alloc_log";
static char *log_ctrl_prefix = "/tmp/trace_alloc_ctrl";
#endif	/* TEST */
#else  /* !LOG_FILE */
char log_file[64];
static char log_ctrl_file[64];
static char *log_file_prefix = "";
static char *log_ctrl_prefix = "";
#endif	/* LOG_FILE */

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

static char *type2str(enum malloc_t type)
{
	switch (type) {
	case malloc_type:
		return "  malloc  ";
	case calloc_type:
		return "  calloc  ";
	case realloc_type:
		return " realloc  ";
	case memalign_type:
		return " memalign ";
	case posix_memalign_type:
		return "p_memalign";
	case strdup_type:
		return "  strdup  ";
	case __strdup_type:
		return " __strdup ";
	case valloc_type:
		return "  valloc  ";
	case free_type:
		return "   free   ";
	case fake_free_type:
		return " fak_free ";
	case free_realloc_type:
		return " free_re  ";
	default:
		return NULL;
	}
}

static struct list_head *get_entry(void)
{
	struct list_head *entry;

again:
	if (!list_empty(&free_list)) {
		entry = free_list.next;
		list_del(free_list.next);
		return entry;
	} else {
		__trace_alloc_result();
		__trace_alloc_reset();

		goto again;
	}
}

static void put_entry(struct list_head *entry)
{
	memset(entry, 0, sizeof(struct list_head));
	list_add_tail(entry, &free_list);
}

static void add_entry(enum malloc_t type, void *addr, unsigned int ra, unsigned int size)
{
	int i;
	struct list_head *entry;

	entry = get_entry();

	entry->data.index = global_index++;
	entry->data.type = type;
	entry->data.addr = addr;
	entry->data.pid = getpid();
	entry->data.ra = ra;
	entry->data.size = size;
#ifdef MIPS
	for (i = 0; i < 5; i++)
		entry->data.code[i] = *(unsigned int *)(ra - 8 + i * 4);
#else
	for (i = 0; i < 5; i++)
		entry->data.code[i] = i;
#endif
	list_add_tail(entry, &g_list);
}

static void del_entry(enum malloc_t type, void *addr, unsigned int ra, unsigned int size)
{
	struct list_head *entry;

	list_for_each_prev(entry, &g_list) {
		if (entry->data.type > 0 && entry->data.type < free_type &&
		    entry->data.addr == addr) {
			list_del(entry);
			put_entry(entry);
			return ;
		}
	}

	add_entry(type, addr, ra, size);
}

static void __trace_alloc_reset(void)
{
	int i;

	memset(&g_list, 0, sizeof(struct list_head));
	memset(&free_list, 0, sizeof(struct list_head));
	for (i = 0; i < MALLOC_NUM ; i++)
		memset(&entry[i], 0, sizeof(struct list_head));

	INIT_LIST_HEAD(&g_list);
	INIT_LIST_HEAD(&free_list);

	for (i = 0; i < MALLOC_NUM ; i++)
		list_add_tail(&entry[i], &free_list);
}

pthread_t trace_alloc_thread;
void *trace_alloc_func(void *args)
{
	while (1) {
		usleep(1000000);

		if (pthread_mutex_trylock(&mutex) == 0) {
			if (access(log_ctrl_file, F_OK) == 0) {
				__trace_alloc_result();
				remove(log_ctrl_file);
			}
			pthread_mutex_unlock(&mutex);
		}
	}
}

static void trace_alloc_init()
{
	pthread_mutex_lock(&init_mutex);

	if (init) {
		pthread_mutex_unlock(&init_mutex);
		return ;
	}

	dlsym_calloc = 1;
	real_malloc = (void * (*) (size_t))dlsym(RTLD_NEXT, "malloc");
	real_calloc = (void * (*) (size_t, size_t))dlsym(RTLD_NEXT, "calloc");
	real_realloc = (void * (*) (void *, size_t))dlsym(RTLD_NEXT, "realloc");
	real_memalign = (void * (*) (size_t, size_t))dlsym(RTLD_NEXT, "memalign");
	real_posix_memalign =
		(int (*) (void **, size_t, size_t))dlsym(RTLD_NEXT, "posix_memalign");
	real_valloc = (void * (*) (size_t))dlsym(RTLD_NEXT, "valloc");
	real_free = (void (*) (void *))dlsym(RTLD_NEXT, "free");
	dlsym_calloc = 0;

	pthread_mutex_lock(&mutex);
	__trace_alloc_reset();
	pthread_mutex_unlock(&mutex);

	init = 1;
	pthread_mutex_unlock(&init_mutex);

	if (strlen(log_file)) {
		filep = fopen(log_file, "w+");
	} else if (strlen(log_file_prefix)) {
		snprintf(log_file, 64, "%s.%d", log_file_prefix, getpid());
		filep = fopen(log_file, "w+");
	}

	if (filep == NULL)
		filep = stdout;

	if (pthread_create(&trace_alloc_thread, NULL, trace_alloc_func, NULL) != 0) {
		printf("trace_alloc_thread: %s\n", strerror(errno));
		exit(1);
	}

	if (!strlen(log_ctrl_file))
		snprintf(log_ctrl_file, 64, "%s.%d", log_ctrl_prefix, getpid());
}

static void add_up(enum malloc_t type, void *addr, unsigned int ra, unsigned int size)
{
#if 0
	pr_debug("%s: %s, addr: 0x%08x, ra = 0x%08x\n", __func__, type2str(type),
	       (unsigned int)addr, ra);
#endif
	pthread_mutex_lock(&mutex);

	switch (type) {
	case malloc_type:
	case calloc_type:
	case realloc_type:
	case memalign_type:
	case posix_memalign_type:
	case strdup_type:
	case __strdup_type:
	case valloc_type:
		add_entry(type, addr, ra, size);
		break;
	case free_type:
	case fake_free_type:
	case free_realloc_type:
		if (addr == NULL) {
			pthread_mutex_unlock(&mutex);
			return ;
		}
		del_entry(type, addr, ra, size);
		break;
	default:
		printf("invalid malloc type: %d\n", type);
		break;
	}

	pthread_mutex_unlock(&mutex);
}

void *malloc(size_t size)
{
	void *ptr;
	unsigned int ra = 0;

#ifdef MIPS
	asm volatile("move %0, $31" :"=r"(ra));
#endif

	pr_debug("%s...\n", __func__);
	trace_alloc_init();
	ptr = real_malloc(size);

#ifndef MIPS
	ra = ~(unsigned int)ptr;
#endif

	add_up(malloc_type, ptr, ra, size);

	return ptr;
}

static void *fake_calloc(size_t __nmemb, size_t __size)
{
	return NULL;
}

void *calloc(size_t __nmemb, size_t __size)
{
	void *ptr;
	unsigned int ra = 0;

#ifdef MIPS
	asm volatile("move %0, $31" :"=r"(ra));
#endif

	if (dlsym_calloc)
		return fake_calloc(__nmemb, __size);

	pr_debug("%s...\n", __func__);
	trace_alloc_init();
	ptr = real_calloc(__nmemb, __size);

#ifndef MIPS
	ra = ~(unsigned int)ptr;
#endif

	add_up(calloc_type, ptr, ra, __size);

	return ptr;
}

#undef strdup
char *strdup(const char *s)
{
	void *ptr, *new;
	unsigned int len, ra = 0;

#ifdef MIPS
	asm volatile("move %0, $31" :"=r"(ra));
#endif

	pr_debug("%s...\n", __func__);
	trace_alloc_init();

	len = strlen(s) + 1;
	new = real_malloc(len);
	if (new == NULL)
		return NULL;
	else
		ptr = (char *)memcpy(new, s, len);

#ifndef MIPS
	ra = ~(unsigned int)ptr;
#endif

	add_up(strdup_type, ptr, ra, len);

	return ptr;
}

#undef __strdup
char *__strdup(const char *s)
{
	void *ptr, *new;
	unsigned int len, ra = 0;

#ifdef MIPS
	asm volatile("move %0, $31" :"=r"(ra));
#endif

	pr_debug("%s...\n", __func__);
	trace_alloc_init();

	len = strlen(s) + 1;
	new = real_malloc(len);
	if (new == NULL)
		return NULL;
	else
		ptr = (char *)memcpy(new, s, len);

#ifndef MIPS
	ra = ~(unsigned int)ptr;
#endif

	add_up(__strdup_type, ptr, ra, len);

	return ptr;
}

void *realloc(void *__ptr, size_t __size)
{
	void *ptr;
	unsigned int ra = 0;

#ifdef MIPS
	asm volatile("move %0, $31" :"=r"(ra));
#endif

	pr_debug("%s...\n", __func__);
	trace_alloc_init();
	ptr = real_realloc(__ptr, __size);

#ifndef MIPS
	ra = ~(unsigned int)ptr;
#endif

	add_up(free_realloc_type, __ptr, ra, __size);
	add_up(realloc_type, ptr, ra, __size);

	return ptr;
}

void *memalign(size_t __alignment, size_t __size)
{
	void *ptr;
	unsigned int ra = 0;

#ifdef MIPS
	asm volatile("move %0, $31" :"=r"(ra));
#endif

	pr_debug("%s...\n", __func__);
	trace_alloc_init();
	ptr = real_memalign(__alignment, __size);

#ifndef MIPS
	ra = ~(unsigned int)ptr;
#endif

	add_up(memalign_type, ptr, ra, __size);

	return ptr;
}

int posix_memalign(void **__memptr, size_t __alignment, size_t __size)
{
	int ret;
	unsigned int ra = 0;

#ifdef MIPS
	asm volatile("move %0, $31" :"=r"(ra));
#endif

	pr_debug("%s...\n", __func__);
	trace_alloc_init();
	ret = real_posix_memalign(__memptr, __alignment, __size);

#ifndef MIPS
	ra = ~(unsigned int)(* __memptr);
#endif

	if (!ret)
		add_up(posix_memalign_type, *__memptr, ra, __size);

	return ret;
}

void *valloc(size_t __size)
{
	void *ptr;
	unsigned int ra = 0;

#ifdef MIPS
	asm volatile("move %0, $31" :"=r"(ra));
#endif

	pr_debug("%s...\n", __func__);
	trace_alloc_init();
	ptr = real_valloc(__size);

#ifndef MIPS
	ra = ~(unsigned int)ptr;
#endif

	add_up(valloc_type, ptr, ra, __size);

	return ptr;
}

void free(void *ptr)
{
	unsigned int ra = 0;

	/* for printf */
	if (ptr == NULL)
		return ;

#ifdef MIPS
	asm volatile("move %0, $31" :"=r"(ra));
#endif

	pr_debug("%s...\n", __func__);
	trace_alloc_init();

#ifndef MIPS
	ra = ~(unsigned int)ptr;
#endif

	if ((int)ptr & 0x3) {
		add_up(fake_free_type, (void *)((int)ptr & ~3), ra, 0);
	} else {
		add_up(free_type, ptr, ra, 0);
		real_free(ptr);
	}
}

/* result */
static unsigned int __trace_alloc_get_free_num(void)
{
	unsigned int count = 0;
	struct list_head *entry;

	list_for_each(entry, &free_list) {
		count++;
	}

	return count;
}

unsigned int trace_alloc_get_free_num(void)
{
	unsigned int count = 0;

	pr_debug("%s...\n", __func__);
	trace_alloc_init();

	pthread_mutex_lock(&mutex);
	count = __trace_alloc_get_free_num();
	pthread_mutex_unlock(&mutex);

	return count;
}

#if 0
int mozart_localplayer_prev_music(void)
{
	pr_debug("%s...\n", __func__);
	trace_alloc_result();

	return 0;
}
#endif

static int container_sort(struct malloc_node *c)
{
	int i, j, len;
	struct malloc_node temp;

	for (i = 0; i < MALLOC_NUM; i++) {
		if (c[i].ra == 0)
			break;
	}
	len = i;

	for (i = 0; i < len; i++) {
		for (j = 0; j < len - i - 1; j++) {
			if ((int)c[j].addr < (int)c[j + 1].addr) {
				memcpy(&temp, &c[j], sizeof(struct malloc_node));
				memcpy(&c[j], &c[j + 1], sizeof(struct malloc_node));
				memcpy(&c[j + 1], &temp, sizeof(struct malloc_node));
			}
		}
	}

	return len;
}

static int __trace_alloc_sort(struct malloc_node *counter, unsigned int num)
{
	int i, len;
	struct list_head *entry;
	struct malloc_node c[MALLOC_NUM];

	memset(counter, 0, sizeof(struct malloc_node) * num);
	memset(c, 0, sizeof(struct malloc_node) * MALLOC_NUM);

	list_for_each(entry, &g_list) {
		for (i = 0; i < MALLOC_NUM; i++) {
			if (c[i].ra == 0) {
				memcpy(&c[i], &entry->data, sizeof(struct malloc_node));
				c[i].addr = (void *)0;
				break;
			}
			if (c[i].ra == entry->data.ra)
				break;
		}
		/* count++ */
		c[i].addr = (void *)((unsigned int)c[i].addr + 1);
	}

	len = container_sort(c);

	if (len > num)
		len = num;

	for (i = 0; i < len; i++)
		memcpy(&counter[i], &c[i], sizeof(struct malloc_node));

	return len;
}

void trace_alloc_sort(struct malloc_node *counter, unsigned int num)
{
	pr_debug("%s...\n", __func__);
	trace_alloc_init();

	pthread_mutex_lock(&mutex);
	__trace_alloc_sort(counter, num);
	pthread_mutex_unlock(&mutex);
}

static void __trace_alloc_result(void)
{
	int i, len, count = 0;
	struct malloc_node *node;
	struct list_head *entry;
	struct malloc_node counter[10];

	if (filep == NULL)
		return ;

	fprintf(filep,
		"== LOG%3d ================================================================\n",
		log_count++);
	fprintf(filep,
		"---------- Result --------------------------------------------------------\n");
	list_for_each(entry, &g_list) {
		fprintf(filep, "  %5d [%s] addr = 0x%08x, ra = 0x%08x, size = 0x%08x,"
			" pid = %3d, index = %lld\n", count++, type2str(entry->data.type),
			(unsigned int)entry->data.addr, entry->data.ra,
			entry->data.size, entry->data.pid, entry->data.index);
		fprintf(filep, "                   code: 0x%08x 0x%08x <0x%08x> 0x%08x 0x%08x\n",
			entry->data.code[0], entry->data.code[1], entry->data.code[2],
			entry->data.code[3], entry->data.code[4]);
		fflush(filep);
	}

	len = __trace_alloc_sort(counter, 10);
	fprintf(filep, "Top 10:\n");
	for (i = 0; i < len; i++) {
		node = &counter[i];
		fprintf(filep, "  %5d [%s] count = %4d, ra = 0x%08x, size = 0x%08x,"
			" pid = %3d, index = %lld\n", i + 1, type2str(node->type),
			(unsigned int)node->addr, node->ra, node->size, node->pid, node->index);
		fprintf(filep, "                   code: 0x%08x 0x%08x <0x%08x> 0x%08x 0x%08x\n",
			node->code[0], node->code[1], node->code[2],
			node->code[3], node->code[4]);
		fflush(filep);
	}

	fprintf(filep,
		"==== free count = %d =====================================================\n",
		__trace_alloc_get_free_num());
	fprintf(filep, "\n\n\n");
	fflush(filep);
}

void trace_alloc_result(void)
{
	pr_debug("%s...\n", __func__);
	trace_alloc_init();

	pthread_mutex_lock(&mutex);
	__trace_alloc_result();
	pthread_mutex_unlock(&mutex);
}

void trace_alloc_reset(void)
{
	pr_debug("%s...\n", __func__);
	trace_alloc_init();

	pthread_mutex_lock(&mutex);
	__trace_alloc_reset();
	pthread_mutex_unlock(&mutex);
}

struct malloc_node *trace_alloc_get_entry(int n)
{
	int count = 0;
	struct list_head *entry;

	pr_debug("%s...\n", __func__);
	trace_alloc_init();

	pthread_mutex_lock(&mutex);
	list_for_each(entry, &g_list) {
		if (count++ == n) {
			pthread_mutex_unlock(&mutex);
			return &entry->data;
		}
	}

	pthread_mutex_unlock(&mutex);

	return NULL;
}

void fake_free(void *ptr)
{
	unsigned int ra = 0;

#ifdef MIPS
	asm volatile("move %0, $31" :"=r"(ra));
#else
	ra = ~(unsigned int)ptr;
#endif

	pr_debug("%s...\n", __func__);
	trace_alloc_init();
	add_up(fake_free_type, ptr, ra, 0);
}
