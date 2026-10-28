#include "xalloc.h"

/**
 * @brief Frees/unmaps a LARGE zone.
 */
void	free_large(t_mallblock *block)
{
	size_t	mapping_size;

	mapping_size = header_size() + block->blocksize;

	block->phys_prev->phys_next = block->phys_next;
	block->phys_next->phys_prev = block->phys_prev;

	munmap(block, mapping_size);
}

/**
 * @brief Frees the data of a pointer.
 *
 * LARGE blocks are munmap directly.
 * 
 * ptr's block is moved into the freelist for reuse.
 * If freelist blocks are phys adjacent, they will be merged.
 * If an entire zone ends up free, it is added to returnables.
 *
 * Due to subject requirements, all mapped zones are returned to
 * the OS whenever all blocks are freed. Returnables are also checked
 * for trimming on free() calls.
 */
void	xfree_impl(void *ptr)
{
	t_mallblock		*block;
	t_mallocstate	*state;

	if (ptr == NULL)
		return ;

	state = get_malloc_state();
	block = find_block_from_ptr(ptr);
	if (block == NULL)
		return ;

	if (block_is_free(block))
		return ;

	if (state->scribble)
		memset(ptr, 0xDD, block->blocksize);

	if (block->zone_type == ZONE_LARGE)
	{
		free_large(block);
		if (state->active_allocations > 0)
			state->active_allocations--;
		if (state->active_allocations == 0)
			trim_all_returnable();
		return ;
	}

	insert_free_block(block);

	if (block->phys_prev != &state->all_blocks
		&& block_is_free(block->phys_prev)
		&& blocks_can_merge(block->phys_prev, block))
		block = merge_blocks(block->phys_prev, block);

	if (block->phys_next != &state->all_blocks
		&& block_is_free(block->phys_next)
		&& blocks_can_merge(block, block->phys_next))
		block = merge_blocks(block, block->phys_next);

	if (block->is_lower && block->is_upper)
		returnable_insert(&state->returnable, block);

	if (state->active_allocations > 0)
		state->active_allocations--;

	trim_returnable_excess();
}

/**
 * @brief Frees the data of a pointer.
 *
 * Actual implementation is done in free_impl(),
 * free() here is simply a wrapper that locks the mutex.
 *
 * @returns the pointer to the new block's data.
 */
void	free(void *ptr)
{
	t_mallocstate	*state;

	pthread_mutex_lock(&g_malloc_mutex);

	state = get_malloc_state();

	if (state->debug)
		debug_free(ptr);

	xfree_impl(ptr);

	pthread_mutex_unlock(&g_malloc_mutex);
}
