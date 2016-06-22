TRACE ALLOC
============
程序用于跟踪*进程*内存的申请与释放情况，以确认是否有内存泄漏的现象。

功能
====
* 记录内存的申请与释放
* 支持把结果输出到文件中
* 支持不同的strdup实现
* 支持根据调用次数排序
* 支持fake_free

编译与使用
==========
## libtrace_alloc.so
* `make libtrace_alloc.mips.so`生成libtrace_alloc.so。
* 把libtrace_alloc.so放到文件系统中，比如mozart/app/usr/debug目录。
* 程序执行时添加`LD_PRELOAD="/usr/debug/libtrace_alloc.so"`环境变量，比如
        > LD_PRELOAD="/usr/debug/libtrace_alloc.so" mozart -sb
* ps查看mozart的pid，比如为100，执行如下命令
        > touch /tmp/trace_alloc_ctrl.110
* 最后查看结果
        > cat /mnt/sdcard/trace_alloc/trace_alloc_log.110

## trace_alloc_test.mips
* `make trace_alloc_test.mips`生成mips版测试程序
* 开发板上执行
        > LD_LIBRARY_PATH="/usr/debug/" /usr/debug/trace_alloc_test.mips

## trace_alloc_test.mips.x86
* ‘make x86’编译并执行

## mozart中编译
* 在top目录下，执行
        > make trace_alloc

说明
====
LD_PRELOAD
----------------
LD\_PRELOAD可以指定程序优先加载的动态链接库，链接器会优先链接LD\_PRELOAD所指定的库。通过这个特性，可以在库中实现某些函数，以截获他们。
现在程序截获了malloc, calloc, realloc, strdup, memalign, posix_memalign, valloc, free这些内存相关函数。并且程序内部会维护一个链表，如
果有新申请的内存就会记录下申请的地址，大小以及函数的返回值等信息; 如果有释放的内存就会把之前申请的记录删除。如果发现释放的内存，没有对
应的申请项，说明没有截获到相应的函数，会记录一个free项。

内存泄漏
------------
内存泄漏：程序动态分配的内存空间，在使用完毕后未释放。
观察记录的结果，如果未释放的内存越来越多，就有可能是内存泄漏的情况。可以通过记录的函数返回地址，来找到函数的调用位置，并结合代码逻辑来确定是否有内存泄漏。
因为backtrace()工作不正常，这里的使用比较麻烦一点。如果发现有新的方法或者有可以使用backtrace()的方法，欢迎修正。
下面举一个例子：
```
    955 [  malloc  ] addr = 0x004223A0, ra = 0x2BC7B830, size = 0x00000168, pid = 110, index = 1533
```
根据记录发现，‘ra = 0x2BC7B830’这个地址申请的内存一直没有释放，怀疑是内存泄漏。
进程的pid是110，cat /proc/110/maps找到包含ra的线性区。
```
2bc79000-2bc7d000 r-xp 00000000 1f:04 3681948    /usr/fs/usr/lib/libplayer.so
```
发现是在libplayer.so中，通过计算：
        0x2BC7B830 - 0x2bc79000 = 0x2830
得到是在libplayer.so的0x2830偏移处的代码申请的内存。
```plain
[DUMP] libplayer.so
00002788 <wrapper2status>:
    2808:	8e220004        lw	v0,4(s1)
    280c:	18400020        blez	v0,2890 <wrapper2status+0x108>
    2810:	8f99808c        lw	t9,-32628(gp)
    2814:	2631001c        addiu	s1,s1,28
    2818:	0320f809        jalr	t9
    281c:	02202021        move	a0,s1
    2820:	8fbc0010        lw	gp,16(sp)
    2824:	8f99809c        lw	t9,-32612(gp)
    2828:	0320f809        jalr	t9
    282c:	24440001        addiu	a0,v0,1
 -> 2830:	8fbc0010        lw	gp,16(sp)
    2834:	02202021        move	a0,s1
    2838:	8f99808c        lw	t9,-32628(gp)
    283c:	ae020000        sw	v0,0(s0)
    2840:	0320f809        jalr	t9
    2844:	00409021        move	s2,v0
    2848:	8fbc0010        lw	gp,16(sp)
    284c:	02402021        move	a0,s2
    2850:	8f998084        lw	t9,-32636(gp)
    2854:	00002821        move	a1,zero
    2858:	0320f809        jalr	t9
```
阅读代码找到wrapper2status函数，再阅读代码逻辑，添加调试信息，判断是不是内存泄漏，这里需要具体分析。
这里也可以使用mipsel-linux-addr2line工具。

LOG输出
----------
### 输出方式
* 输出到文件
`trace_alloc.c`中`log_file`不为空时，`log_file`为输出文件。
或者`log_file_prefix`不为空时，`log_file_prefix`加上pid号，组合为输出文件。
`log_file`优先级更高。
默认使用`log_file_prefix`，记录到sd卡中，可以区分不同进程。
* 输出到串口
`log_file`和`log_file_prefix`都为空，或者创建文件失败时，结果输出到串口。

### 输出控制
* 文件控制
`trace_alloc.c`中`log_ctrl_file`不为空时，`log_ctrl_file`为控制文件。
`log_ctrl_prefix`不为空时，`log_ctrl_prefix`加上pid号，组合为控制文件。
`log_ctrl_file`优先级更高。
在执行截获的函数时，发现存在控制文件，程序就会输出记录结果，并删除控制文件。
* 直接调用（截获特定函数）
直接调用`trace_alloc_result`函数，但是编译时需要链接`libtrace_alloc.so`。
也可以截获一些可控的函数，在函数中调用`trace_alloc_result`。比如截获了mozart中`mozart_localplayer_prev_music`，
按键就可以打印结果。

fake_free
-----------
如果程序调用free函数，但参数不是四字节对齐的，程序执行`fake_free_type`的逻辑。不会执行真正的free函数，但是会执行链表的操作。
这个功能主要用于测试。比如要找一段内存是什么时候释放的，malloc得到的地址是ptr，之后直接调用free(ptr | 1)。由于fake_free不会
执行真正的free，但会操作链表。这样之前malloc的项被删除，而之后真正的free会由于没有匹配的malloc项而留在链表中。
还可以用来确认一段内存有没有被释放，可以在这段内存申请前后添加特殊地址的free函数，然后在链表中找到这个`fake_free`，再观察其
中的内存有没有被释放。
同理，可以在特定的位置添加free函数，来确定代码的执行逻辑（常用来配合特定的malloc项）。

其他
------
可以通过阅读测试代码main.c中的case是如何运行和check的，来理解程序的一些行为。
比如strdup不同的使用方法或不同的编译选项，截获的方法会不同。
