#include <stdio.h>
#include <stdlib.h>
#define MEM_POOL_SIZE 4096
#define MEM_BLOCK_SIZE 256

// 内存池策略：
// 共申请MEM_POOL_SIZE字节的堆内存作为内存池
// 内存池分为若干个BLOCK，每个BLOCK大小为MEM_BLOCK_SIZE，将空闲BLOCK构成链表
// 内存的分配是定长分配，每次分配若干个MEM_BLOCK_SIZE大小的内存空间
// 内存释放后插到空闲链表的头
// 注：MEM_POOL_SIZE必须是MEM_BLOCK_SIZE的整数倍

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
};
typedef struct mem_pool mempool_t;
// 内存池的初始化
void pool_init();
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
		printf("malloc() failed!\n");
		exit(-1);
	}
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
		printf("线程池已销毁\n");
		free(pool->p_start);
		pool->p_start = NULL;
		pool->idle = NULL;
	}
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
	
	malloc(210);
	malloc(256);
	malloc(257);
	malloc(510);
	malloc(2560);
	malloc(2);
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
	if(!p)
	{
		return;
	}
}
