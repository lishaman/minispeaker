#ifndef _TRACE_ALLOC_H_
#define _TRACE_ALLOC_H_

#ifdef TEST
#define MALLOC_NUM	64
#else
#define MALLOC_NUM	32 * 1024
#endif

enum malloc_t {
	malloc_type = 1, calloc_type, realloc_type, memalign_type, posix_memalign_type,
	strdup_type, __strdup_type, valloc_type, free_type, fake_free_type, free_realloc_type,
};

struct malloc_node {
	unsigned long long index;
	enum malloc_t type;
	void *addr;
	unsigned int pid;
	unsigned int ra;
	unsigned int size;
	unsigned int code[5];
};

struct list_head {
	struct malloc_node data;
	struct list_head *next, *prev;
};

unsigned int trace_alloc_get_free_num(void);
void trace_alloc_result(void);
void trace_alloc_sort(struct malloc_node *counter, unsigned int num);
void trace_alloc_reset(void);
struct malloc_node *trace_alloc_get_entry(int n);
void fake_free(void *ptr);

#endif	/* _TRACE_ALLOC_H_ */
