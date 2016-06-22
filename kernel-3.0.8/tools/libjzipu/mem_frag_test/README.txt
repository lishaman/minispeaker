
参考文档： 《JZ4785_IPU&VPU_TLB测试报告-new .doc》

后台运行了一个内存打碎的测试程序。
代码位置：kernel-3.0.8/drivers/video/jz4785-ipu/libjzipu_85/memery
主程序为：
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#define MAX_ALLOC_NUM   10000
struct mem_info {
    void *ptr;
    int len;
};
static struct mem_info mem_ptrs[MAX_ALLOC_NUM];
int main() {
    int i = 0, count = 0, count1 = 0;
    srand(time(NULL));
    printf("=====>start frag......\n");
    while (1) {
        count = 0;
        for (i = 0; i < MAX_ALLOC_NUM; i++) {
            int alloc_size = rand() % (4 * 1024);
            mem_ptrs[i].ptr = malloc(alloc_size);
            if (!mem_ptrs[i].ptr) {
                printf("===>mem exhausting......\n");
                break;
            } else {
                memset(mem_ptrs[i].ptr, 1, alloc_size);
                count++;
            }
        }
        printf("====>returning memory(count = %d)\n", count);
        sleep(5);
        for (i = 0; i < MAX_ALLOC_NUM; i++) {
            if (mem_ptrs[i].ptr) {
                free(mem_ptrs[i].ptr);
                mem_ptrs[i].ptr = NULL;
            }
        }
        count1++;
        printf("===>frag done...count=%d count1=%d\n",count,count1);
    }
    return 0;
}
编译以上文件生成mem_frag.
编写后台运行脚本：
#!/system/bin/sh
/data/mem_frag &
/data/mem_frag &
/data/mem_frag &
/data/mem_frag &
/data/mem_frag &
/data/mem_frag &
/data/mem_frag &
/data/mem_frag &
while :; do
    sleep 3
    cat  /sys/devices/platform/jz-ipu.0/dump_mem
done


其中的cat /sys/devices/platform/jz-ipu.0/dump_mem是在ipu中添家的ipu的一 个sys接口接口实现是 show_mem(SHOW_MEM_FILTER_NODES);
这个可以查看系统当前内存的使用情况和内存块的大小和数量。
