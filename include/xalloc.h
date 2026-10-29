#ifndef XALLOC_H
# define XALLOC_H

# include <unistd.h>
# include <stdio.h>
# include <stddef.h>
# include <string.h>
# include <stdbool.h>
# include <stdint.h>
# include <sys/mman.h>
# include <pthread.h>

# define TINY_MAX 128
# define SMALL_MAX 1024

/* Min number of allocs a TINY/SMALL zone should be able to store */
# define ZONE_MIN_ALLOCS 100

/* Max number of fully free TINY/SMALL zones kept for reuse */
# define MAX_RETURNABLE_ZONES 5

/* Min payload size required in the remainder after splitting */
# define MIN_SPLIT_DATA 64

/* Global variable used to make this implementation thread safe */
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
 *	Circular, double linked. Nodes pointing to themselves are root in their list.
 *		List roots are sentinel nodes, not real allocations.
 *		An empty circular list has root->next == root and root->prev == root.
 *
 *	phys_next/prev pointers point to the next physical header/memory block.
 *
 *	free_next/prev pointers point to free blocks available for reuse after freed.
 *		The block is considered allocated when both free pointers are null.
 *		Therefore, blocks which free_next/prev != null are freed.
 *
 *	returnable_next/prev pointers are set when a zone has all its block freed.
 *		returnable zones are kept for reuse to avoid multiplying mmap() calls.
 *
 *
 *	Physical List
 *		Circular list of all physical blocks known by the allocator.
 *
 *		Used to navigate adjacent blocks to merge free blocks and for realloc.
 *
 *		Consecutive nodes may belong to different mmap zones, so zone
 *		boundaries must be checked before merging them.
 *
 *		is_lower / is_upper must be checked before merging across phys_next/prev.
 *
 *	Free List
 *		Freelist blocks are inserted into size-based freelists and reused
 *		by future allocations.
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
 *		Returnable zones are reused on malloc call when available instead of
 *		calling mmap().
 *
 *		Returnable zones get munmap()'d when more than 5 pile up. They also get
 *		munmap()'d when all blocks in this malloc implementation get freed.
 *		
 *		Giving the memory back to the OS with a function would've been better
 *		practice for this implementation of malloc, but 42's subject is
 *		stricter on malloc/realloc/free doing it all. 
*/
typedef struct s_mallblock	t_mallblock;

struct s_mallblock
{
	t_mallblock		*phys_next;
	t_mallblock		*phys_prev;
	union
	{
		struct
		{
			t_mallblock		*free_next;
			t_mallblock		*free_prev;
		};
		struct
		{
			t_mallblock		*returnable_next;
			t_mallblock		*returnable_prev;
		};
	};

	size_t			blocksize;
	unsigned char	zone_type;

	bool			is_upper;
	bool			is_lower;
	bool			is_returnable;
};

/*
 *	Single/static struct holding the implementation's state
*/
typedef struct s_mallocstate
{
	t_mallblock	freelists[128];
	t_mallblock	all_blocks;
	t_mallblock	returnable;
	bool		initialized;
	size_t		active_allocations;
	bool		debug;
	bool		scribble;
	bool		show_state;
}	t_mallocstate;

/* allocator.c */
void			*malloc(size_t size);
void			*xalloc_impl(size_t size);

/* state.c */
void			allocator_init(void);
t_mallocstate	*get_malloc_state(void);

/* physlist.c */
void			phys_list_init(t_mallblock *root);

/* block.c */
size_t			header_size(void);
void			*block_data(t_mallblock *block);
t_mallblock		*data_block(void *ptr);
t_mallblock		*find_block_from_ptr(void *ptr);
bool			block_is_free(t_mallblock *block);
bool			can_split(t_mallblock *block, size_t requested);
t_mallblock		*split_block(t_mallblock *block, size_t requested);
bool			blocks_can_merge(t_mallblock *lower, t_mallblock *upper);
t_mallblock		*merge_blocks(t_mallblock *lower, t_mallblock *upper);
t_mallblock		*grow_block_into_next(t_mallblock *block);

/* size_utils.c */
size_t			align_up(size_t size, size_t alignment);
bool			align_up_safe(size_t size, size_t alignment, size_t *result);
bool			size_add_overflow(size_t a, size_t b, size_t *result);
bool			size_mul_overflow(size_t a, size_t b, size_t *result);

/* zones.c */
t_zone_type		get_zone_type(size_t size);
size_t			zone_size_for(size_t max_payload, size_t pagesize);
t_mallblock		*create_zone(size_t zone_size, t_zone_type type);
void			*alloc_large(size_t size);

/* freelist.c */
void			free_list_init(t_mallblock *root);
bool			free_list_empty(t_mallblock *root);
void			free_list_remove(t_mallblock *block);
size_t			freelist_index(size_t size);
void			insert_free_block(t_mallblock *block);
t_mallblock		*find_free_block(size_t requested, t_zone_type type);

/* returnable.c */
void			returnable_list_init(t_mallblock *root);
bool			block_is_returnable(t_mallblock *block);
t_mallblock		*find_returnable_block(size_t requested, t_zone_type type);
void			returnable_insert(t_mallblock *root, t_mallblock *block);
void			returnable_remove(t_mallblock *block);
void			unmap_returnable_zone(t_mallblock *block);
void			trim_all_returnable(void);
void			trim_returnable_excess(void);

/* free.c */
void			free(void *ptr);
void			xfree_impl(void *ptr);

/* realloc.c */
void			*realloc(void *ptr, size_t size);
void			*rexalloc_impl(void *ptr, size_t size);

/* debug.c */
void			debug_malloc(size_t size, void *ptr);
void			debug_free(void *ptr);
void			debug_realloc(void *old_ptr, size_t size, void *new_ptr);
void			show_alloc_mem(void);
void			show_alloc_mem_ex(void);
void			show_malloc_state(void);

#endif
