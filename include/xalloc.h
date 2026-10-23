#ifndef XALLOC_H
#define XALLOC_H

#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <pthread.h>

#define TINY_MAX 128
#define SMALL_MAX 1024

/* Min number of allocs a TINY/SMALL zone should be able to store */
#define ZONE_MIN_ALLOCS 100

/* Max number of fully free TINY/SMALL zones kept for reuse*/
#define MAX_RETURNABLE_ZONES 5

/* Min payload size required in the remainder after splitting*/
#define MIN_SPLIT_DATA 64

extern pthread_mutex_t	g_malloc_mutex;

typedef enum e_zone_type
{
	ZONE_TINY,
	ZONE_SMALL,
	ZONE_LARGE
}	t_zone_type;

/*
 *	Main struct, acts as Headers, Memory Blocks and pretty much everything.
 *
 *	Circular, double linked. Nodes pointing to themselves mean they are the root in the list.
 *		List roots are sentinel nodes, not real allocations.
 *		An empty circular list has root->next == root and root->prev == root.
 *
 *	phys_next/prev pointers point to the next physical header/memory block.
 *
 *	free_next/prev pointers point to free blocks available for reuse after data was freed.
 *		The block is considered allocated when both free pointers are null.
 *		Therefore, blocks which free_next/prev != null are freed.
 *
 *	returnable_next/prev pointers are set when an entire zone has all its block freed.
 *		returnable zones are kept for reuse to avoid multiplying mmap() calls.
 *
 *
 *	Physical List
 *		Circular list of all physical blocks known by the allocator.
 *
 *		It is used to navigate neighbouring blocks to merge free blocks and for realloc.
 *
 *		Consecutive nodes may belong to different mmap zones, so zone
 *		boundaries must be checked before merging them.
 *
 *		is_lower / is_upper must be checked before merging across phys_next/prev.
 *
 *	Free List
 *		Freelist blocks are inserted into size-based freelists and reused by future
 *		allocations.
 *
 *      Two free blocks can only be merged if they are physically adjacent
 *      and belong to the same mmap zone.
 *      
 *		free_next/prev describe freelist membership, not physical
 *      adjacency.
 *
 *		A page boundary does not prevent merging. Zones may span several pages.
 *		Mapping boundaries are detected with is_lower / is_upper.
 *
 *	Returnable List
 *		Returnable zones are reused on malloc call when available instead of calling mmap().
 *
 *		Returnable zones get munmap()'d when more than 5 pile up. They also get munmap()'d when all 
 *		blocks in this malloc implementation get freed.
 *		
 *		Giving the memory back to the OS with a function would've been better practice for this
 *		implementation of malloc, but 42's subject is stricter on malloc/realloc/free doing it all. 
*/
typedef struct s_MallBlock t_MallBlock;

struct s_MallBlock
{
	t_MallBlock *phys_next;
	t_MallBlock *phys_prev;

	t_MallBlock *free_next;
	t_MallBlock *free_prev;

	t_MallBlock *returnable_next;
	t_MallBlock *returnable_prev;

	bool		is_upper;
	bool		is_lower;

	size_t		blocksize;
	t_zone_type	zone_type;
};

typedef struct s_MallocState
{
	t_MallBlock	freelists[128];
	t_MallBlock	all_blocks;
	t_MallBlock	returnable;
	bool		initialized;
	size_t	active_allocations;
}	t_MallocState;

/* allocator.c */
void			*xalloc(size_t size);
void			*xalloc_impl(size_t size);

/* state.c */
void			allocator_init(void);
t_MallocState	*get_malloc_state(void);

/* physlist.c */
void			phys_list_init(t_MallBlock *root);

/* block.c */
size_t			header_size(void);
void			*block_data(t_MallBlock *block);
t_MallBlock		*data_block(void *ptr);
bool			block_is_free(t_MallBlock *block);
bool			can_split(t_MallBlock *block, size_t requested);
t_MallBlock		*split_block(t_MallBlock *block, size_t requested);
bool			blocks_can_merge(t_MallBlock *lower, t_MallBlock *upper);
t_MallBlock		*merge_blocks(t_MallBlock *lower, t_MallBlock *upper);
t_MallBlock		*grow_block_into_next(t_MallBlock *block);

/* size_utils.c */
size_t			align_up(size_t size, size_t alignment);
bool			align_up_safe(size_t size, size_t alignment, size_t *result);
bool			size_add_overflow(size_t a, size_t b, size_t *result);
bool			size_mul_overflow(size_t a, size_t b, size_t *result);

/* zones.c */
t_zone_type		get_zone_type(size_t size);
size_t			tiny_zone_size(size_t pagesize);
size_t			small_zone_size(size_t pagesize);
t_MallBlock		*create_zone(size_t zone_size, t_zone_type type);
void			*alloc_large(size_t size);

/* freelist.c */
void			free_list_init(t_MallBlock *root);
bool			free_list_empty(t_MallBlock *root);
void			free_list_insert(t_MallBlock *root, t_MallBlock *block);
void			free_list_remove(t_MallBlock *block);
size_t			freelist_index(size_t size);
void			insert_free_block(t_MallBlock *block);
t_MallBlock		*find_free_block(size_t requested, t_zone_type type);

/* returnable.c */
void			returnable_list_init(t_MallBlock *root);
bool			block_is_returnable(t_MallBlock *block);
void			returnable_insert(t_MallBlock *root, t_MallBlock *block);
void			returnable_remove(t_MallBlock *block);
void			unmap_returnable_zone(t_MallBlock *block);
void			trim_all_returnable(void);
void			trim_returnable_excess(void);

/* free.c */
void			xfree(void *ptr);
void			xfree_impl(void *ptr);

/* realloc.c */
void			*rexalloc(void *ptr, size_t size);
void			*rexalloc_impl(void *ptr, size_t size);

/* debug.c */
void			show_alloc_mem(void);
void			show_alloc_mem_ex(void);
void			show_malloc_state(void);


#endif
