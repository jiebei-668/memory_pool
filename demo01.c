/****************************************************************************
 * program: demo01
 * author: jiebei
 * 本程序实现了一个简单的内存池，具体策略如下，详情见代码
 * 内存池策略：
 * 共申请MEM_POOL_SIZE字节的堆内存作为内存池
 * 内存池分为若干个BLOCK，每个BLOCK大小为MEM_BLOCK_SIZE，将空闲BLOCK构成链表
 * 内存的分配是定长分配，每次分配若干个MEM_BLOCK_SIZE大小的内存空间
 * 内存释放后插到空闲链表的头
 * 内存释放策略：打表用一个数组记录分配内存时一次连续分配的内存单元的个数，回收时通过查表一次回收所有的内存
 * 注：MEM_POOL_SIZE必须是MEM_BLOCK_SIZE的整数倍
 * Usage：./demo01
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MEM_POOL_SIZE 4096
#define MEM_BLOCK_SIZE 256


struct mem_pool
{
	// 内存池起始地址
	void *p_start;
	// 内存池空闲内存地址
	void *idle;
	// 内存池总大小
	size_t total_size;
	// 内存池最小分配的单元内存大小
	size_t block_size;
	// 内存池总单元内存数
	size_t block_num;
	// 内存池空闲内存数
	size_t idle_num;
	// 记录分配的内存的单元个数，方便回收
	// 是一个数组，malloced_block_num[i]记录以(p_start+i*block_size)为起始地址的分配出去的内存一次分配的block个数，如果没分配就是0
	size_t *malloced_block_num;
};
typedef struct mem_pool mempool_t;
// 内存池的初始化
void pool_init();
// 将mempool中某单元地址转换为在mempool的malloced_block_num成员中的下标
int addr_to_idx(void *p);
// 分配内存，成功返回地址，失败返回NULL
void *_malloc(size_t size);
// 回收内存
void _free(void *p);
// 销毁内存池
void pool_destory();

// 内存池的初始化
void pool_init(mempool_t *pool)
{
	pool->total_size = MEM_POOL_SIZE;
	pool->block_size = MEM_BLOCK_SIZE;
	pool->block_num = pool->total_size/pool->block_size;
	pool->idle_num = 0;

	pool->p_start = NULL;
	pool->p_start = malloc(pool->total_size);
	if(!pool->p_start)
	{
		printf("malloc() failed-0!\n");
		exit(-1);
	}
	pool->malloced_block_num = NULL;
	pool->malloced_block_num = (size_t *)malloc(sizeof(size_t)*pool->block_num);
	if(!pool->malloced_block_num)
	{
		printf("malloc() failed-1!\n");
		free(pool->p_start);
		exit(-1);
	}
	memset(pool->malloced_block_num, 0, sizeof(size_t)*pool->block_num);
	pool->idle = pool->p_start;
	pool->idle_num = pool->block_num;
	
	// 实现链表结构
	char *p = (char *)pool->p_start;
	for(int ii = 0; ii < pool->block_num-1; ii++)
	{
		*(char **)p = (char *)p + pool->block_size;
		p += pool->block_size;
	}
	*(char **)p = NULL;
	return;
}
// 销毁内存池
void pool_destory(mempool_t *pool)
{
	if(pool->p_start)
	{
		free(pool->p_start);
		pool->p_start = NULL;
		pool->idle = NULL;
	}
	if(pool->malloced_block_num)
	{
		free(pool->malloced_block_num);
		pool->malloced_block_num = NULL;
	}
	printf("线程池已销毁\n");
}
#define malloc(size) _malloc(size)
#define free(ptr) _free(ptr)

mempool_t pool;
int main(int argc, char* argv[])
{
	pool_init(&pool);
	
	// for test pool_init
	// 测试pool_init，测试链表结构是否正确实现
	/*
	printf("mempool start addr[%p]\n", pool.p_start);
	for(int ii = 0; ii < pool.block_num; ii++)
	{
		printf("block[%2d] addr[%p], point to addr[%p]\n", ii, pool.p_start+ii*pool.block_size, *(char **)(pool.p_start+ii*pool.block_size));
	}
	*/
	
	void *p1 = malloc(210);
	void *p2 = malloc(256);
	void *p3 = malloc(257);
	void *p4 = malloc(510);
	void *p5 = malloc(2560);
	void *p6 = malloc(2);
	free(p1);
	free(p2);
	free(p3);
	free(p4);
	free(p5);
	free(p6);
	pool_destory(&pool);
	return 0;
}
// 分配内存，成功返回地址，失败返回NULL
void *_malloc(size_t size)
{
	if(pool.idle_num * pool.block_size >= size)
	{
		void *p_ret = pool.idle;
		// 上取整
		size_t block_num = (size + pool.block_size -1)/pool.block_size;
		pool.idle_num -= block_num;
		// 在表中记录分配的单元数，方便回收
		pool.malloced_block_num[addr_to_idx(p_ret)] = block_num;
		// 移动空闲单元指针
		for(int ii = 0; ii < block_num; ii++)
		{
			pool.idle = *(char **)pool.idle;
		}
		printf("申请%lu空间，分配%lu个单元，共分配%lu空间，内存池还剩%lu个单元\n", size, block_num, block_num*pool.block_size, pool.idle_num);
		return p_ret;
	}
	else
	{
		printf("申请%lu空间，内存池还剩%lu个单元，共%lu空间，内存不足，分配失败\n", size, pool.idle_num, pool.idle_num*pool.block_size);
		return NULL;
	}
}
// 回收内存
void _free(void *p)
{
	if(p)
	{
		size_t block_num = pool.malloced_block_num[addr_to_idx(p)];
		pool.idle_num += block_num;
		void *new_idle = p;
		char *p_ch = p;
		for(int ii = 0; ii < block_num-1; ii++)
		{
			*(char **)p_ch = (char *)p_ch + pool.block_size;
			p_ch += pool.block_size;
		}
		*(char **)p_ch = (char *)pool.idle;
		// for test _free
		printf("已回收以[%p]为开头的空间，共%lu个单元，共%lu空间\n", p, block_num, block_num*pool.block_size);
		printf("原空闲内存地址[%p]\n", pool.idle);
		char *tmp = new_idle;
		for(int ii = 0; ii < block_num; ii++)
		{
			printf("内存[%p]的下一块为[%p]\n", tmp, *(char **)tmp);
			tmp += pool.block_size;
		}
		printf("回收后新的空闲内存地址[%p]，空闲内存单元有%lu个，共%lu空间\n", new_idle, pool.idle_num, pool.idle_num*pool.block_size);

		pool.idle = new_idle;
		p = NULL;
		return;
	}
	printf("空指针，无需释放\n");
	return;
}
// 将mempool中某单元地址转换为在mempool的malloced_block_num成员中的下标
int addr_to_idx(void *p)
{
	return ((char *)p-(char *)pool.p_start)/pool.block_size;
}
