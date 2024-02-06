#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
// level从0计数，level 0的内存单元是可以分配的最小内存，大小为MIN_UNIT_SIZE
// 最大的level值，也就是一共有 level+1 个level
#define MAX_LEVEL 10
// 可分配的最小内存单元的大小，即第0层的每个内存单元的大小
#define MIN_UNIT_SIZE 1
// 内存池内存大小
#define MEM_SIZE MIN_UNIT_SIZE<<MAX_LEVEL
#define NODE_NUM (2<<(MAX_LEVEL+1))-1
// 二叉树从0开始，所以左子 idx*2+1 右子 idx*2+2
#define lson(idx) (idx<<1)+1
#define rson(idx) (idx<<1)+2
// 判断num是否为2的幂
#define IS_TWO_POWER(num) !(num&(num-1))
// 某个节点对应的内存块不可用，即它的一部分内存被其父辈或子辈节点所拥有
#define NOT_AVAILABLE 0
// 每个节点对应的内存块可用但已分配
#define IS_ALLOCATED 1
// 每个节点对应的内存块可用且未分配
#define IS_IDLE 2

// 用完全二叉树描述伙伴分配系统，每个节点的定义如下，分别是这个节点对应的内存单元的起始地址和大小
// 树的节点根的idx为0，左儿子为 2*ii+1 右儿子为 2*ii+2
typedef struct node
{
	void *start_addr;
	size_t size;
	//uint_8 state;
	char state;
}node_t;

typedef struct mempool
{
	// 内存池初始地址
	void *p_start;
	// 内存池对应的二叉树起始地址
	node_t *tree;
}mempool_t;
// 初始化内存池，主要是申请内存和初始化二叉树
mempool_t *pool_init();
// dfs初始化tree的各节点
// tree: 树的起始地址
// idx: 当前节点下标
// level: 当前节点的level
// addr: 当前节点对应内存块的起始地址
void init_tree(node_t *tree, int level, int idx, void *addr);
// 销毁内存池
void pool_destory(mempool_t *pool);
// 分配size大小的空间，成功返回地址，失败返回0地址
void *_malloc(mempool_t *pool, size_t size);
// 深搜遍历二叉树，寻找合适的内存块
// 成功返回内存块起始地址，失败返回0地址
// tree: 树的起始地址
// idx: 当前节点下标
// level: 当前节点的level
// target_level: 要分配的内存块所在的level
void *dfs_alloc(int level, int target_level, node_t *tree, int idx);
// 将idx节点分裂为一对兄弟
void split(node_t *tree, int idx);
void _free(mempool_t *pool, void *addr);
// 深搜找到要释放内存块对应的节点，然后释放
// tree: 树的起始地址
// idx: 当前节点下标
// level: 当前节点的level
// addr: 要分配的内存块的起始地址
void dfs_free(int level, node_t *tree, int idx, void *addr);
// 合并一堆兄弟
void merge(node_t *tree, int idx);
// 获得最小的大于size的2次幂对应的level
int get_fix_level(size_t size);
int main(int argc, char* argv[])
{
	mempool_t *pool = pool_init();
	if(pool == NULL)
	{
		exit(-1);
	}
	void *ad1 = _malloc(pool, 500);
	void *ad2 = _malloc(pool, 256);
	void *ad3 = _malloc(pool, 255);
	void *ad4 = _malloc(pool, 500);
	_free(pool, ad2);
	void *ad5 = _malloc(pool, 500);
	_free(pool, ad3);
	void *ad6 = _malloc(pool, 500);
	pool_destory(pool);
	return 0;
}
mempool_t *pool_init()
{
	mempool_t *pool = (mempool_t *)malloc(sizeof(mempool_t));
	if(pool == NULL)
	{
		return NULL;
	}
	pool->p_start = malloc(sizeof(char) * MEM_SIZE);
	if(pool->p_start == NULL)
	{
		free(pool);
		pool = NULL;
		return NULL;
	}
	pool->tree = (node_t *)malloc(sizeof(node_t) * NODE_NUM);
	if(pool->tree == NULL)
	{
		free(pool->p_start);
		pool->p_start = NULL;

		free(pool);
		pool = NULL;
		return NULL;
	}
	// 初始化tree的各节点
	init_tree(pool->tree, MAX_LEVEL, 0, pool->p_start);
	return pool;
}
void init_tree(node_t *tree, int level, int idx, void *addr)
{
	if(level == -1)
	{
		return;
	}
	tree[idx].size = MIN_UNIT_SIZE<<level;
	tree[idx].start_addr = addr; 
	if(level == MAX_LEVEL)
	{
		tree[idx].state = IS_IDLE;
	}
	else
	{
		tree[idx].state = NOT_AVAILABLE;
	}
	init_tree(tree, level-1, lson(idx), addr);
	init_tree(tree, level-1, rson(idx), (char *)addr+tree[idx].size/2);
	return;
}
void pool_destory(mempool_t *pool)
{
	if(pool == NULL)
	{
		return;
	}
	free(pool->tree);
	pool->tree = NULL;
	free(pool->p_start);
	pool->p_start = NULL;
	return;
}
int get_fix_level(size_t size)
{
	if(!IS_TWO_POWER(size))
	{
		size |= (size >> 1);
		size |= (size >> 2);
		size |= (size >> 4);
		size |= (size >> 8);
		size |= (size >> 16);
		size += 1;
	}
	int ii;
	for(ii = MAX_LEVEL; ii >= 0; ii--)
	{
		if(size & (MIN_UNIT_SIZE<<ii))
		{
			break;
		}
	}
	return ii;
}
void split(node_t *tree, int idx)
{
	tree[idx].state = NOT_AVAILABLE;
	tree[lson(idx)].state = IS_IDLE;
	tree[rson(idx)].state = IS_IDLE;
	return;
}
void *dfs_alloc(int level, int target_level, node_t *tree, int idx)
{
	// 目标lecel及以上没有空闲块，返回NULL
	if(level < target_level)
	{
		return NULL;
	}
	// 当前块已分配直接返回
	if(tree[idx].state == IS_ALLOCATED)
	{
		return NULL;
	}
	// 在目标lecel上找到了空闲块，直接分配
	if(level == target_level && tree[idx].state == IS_IDLE)
	{
		tree[idx].state = IS_ALLOCATED;
		return tree[idx].start_addr;
	}
	if(level > 0 && tree[idx].state == IS_IDLE)
	{
		split(tree, idx);
	}
	void *l_addr = dfs_alloc(level-1, target_level, tree, lson(idx));
	if(l_addr)
	{
		return l_addr;
	}
	return dfs_alloc(level-1, target_level, tree, rson(idx));
}
void *_malloc(mempool_t *pool, size_t size)
{
	if(size > MEM_SIZE)
	{
		return NULL;
	}
	int target_level = get_fix_level(size);
	return dfs_alloc(MAX_LEVEL, target_level, pool->tree, 0);
}
void merge(node_t *tree, int idx)
{
	if(tree[lson(idx)].state == IS_IDLE && tree[rson(idx)].state == IS_IDLE)
	{
		tree[idx].state = IS_IDLE;
		tree[lson(idx)].state = NOT_AVAILABLE;
		tree[rson(idx)].state = NOT_AVAILABLE;
	}
	return;
}
void dfs_free(int level, node_t *tree, int idx, void *addr)
{
	if(level < 0)
	{
		return;
	}
	// 找到要释放内存块对应的节点，修改state
	if(tree[idx].start_addr == addr && tree[idx].state == IS_ALLOCATED)
	{
		tree[idx].state = IS_IDLE;
		return;
	}
	if(addr <= tree[idx].start_addr)
	{
		dfs_free(level-1, tree, lson(idx), addr);
	}
	else
	{
		dfs_free(level-1, tree, rson(idx), addr);
	}
	if(level > 0)
	{
		merge(tree, idx);
	}
	return;
}
void _free(mempool_t *pool, void *addr)
{
	if(addr == NULL)
	{
		return;
	}
	dfs_free(MAX_LEVEL, pool->tree, 0, addr);
	return;
}
