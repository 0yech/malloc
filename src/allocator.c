#include "xalloc.h"

/**
 * @brief actual implementation of malloc.
 *
 * Starts by initializing all list types if not done yet.
 * 
 * For a LARGE zone, allocates a zone dedicated to the allocation.
 *
 * For TINY/SMALL allocs, it seeks in the freelist/returnables for an
 * available block. If it fails to find any usable blocks. It will map
 * a new zone.
 *
 * @returns the pointer to the new block's data.
 */
void	*xalloc_impl(size_t size)
{
	t_mallocstate	*state;
	t_mallblock		*block;
	t_mallblock		*rest;
	t_zone_type		type;
	size_t			pagesize;
	size_t			zone_size;
	void			*ptr;

	if (size == 0)
		return (NULL);

	allocator_init();
	state = get_malloc_state();
	type = get_zone_type(size);

	if (type == ZONE_LARGE)
	{
		ptr = alloc_large(size);
		if (ptr == NULL)
			return (ptr);
		state->active_allocations++;
		// MALLOC_SCRIBBLE ENV VAR
		if (state->scribble)
			memset(ptr, 0xAA, size);
		return (ptr);
	}

	block = find_free_block(size, type);

	if (block == NULL)
	{
		pagesize = sysconf(_SC_PAGESIZE);
		if (pagesize == (size_t)-1)
			return (NULL);

		if (type == ZONE_TINY)
			zone_size = zone_size_for(TINY_MAX, pagesize);
		else
			zone_size = zone_size_for(SMALL_MAX, pagesize);

		if (zone_size == 0)
			return (NULL);

		block = create_zone(zone_size, type);
		if (block == NULL)
			return (NULL);
	}

	if (block_is_returnable(block))
		returnable_remove(block);

	free_list_remove(block);

	if (can_split(block, size))
	{
		rest = split_block(block, size);
		if (rest != NULL)
			insert_free_block(rest);
	}

	state->active_allocations++;

	// MALLOC_SCRIBBLE ENV VAR
	if (state->scribble)
		memset(block_data(block), 0xAA, block->blocksize);

	return (block_data(block));
}

/**
 * @brief allocates a pointer with a given size.
 *
 * Actual implementation is done in malloc(),
 * malloc() here is simply a wrapper that locks the mutex.
 *
 * @returns the pointer to the new block's data.
 */
void	*malloc(size_t size)
{
	t_mallocstate	*state;
	void			*ptr;

	pthread_mutex_lock(&g_malloc_mutex);

	ptr = xalloc_impl(size);
	state = get_malloc_state();

	if (state->debug)
		debug_malloc(size, ptr);

	pthread_mutex_unlock(&g_malloc_mutex);

	return (ptr);
}
