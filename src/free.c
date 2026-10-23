#include "xalloc.h"

void	free_large(t_MallBlock *block)
{
	size_t	mapping_size;

	mapping_size = header_size() + block->blocksize;

	block->phys_prev->phys_next = block->phys_next;
	block->phys_next->phys_prev = block->phys_prev;

	munmap(block, mapping_size);
}

void	xfree_impl(void *ptr)
{
	t_MallBlock		*block;
	t_MallocState	*state;

	if (ptr == NULL)
		return ;

	state = get_malloc_state();
	block = data_block(ptr);

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

	if (state->active_allocations == 0)
		trim_all_returnable();
    else
        trim_returnable_excess();
}

void	xfree(void *ptr)
{
	pthread_mutex_lock(&g_malloc_mutex);

	xfree_impl(ptr);

	pthread_mutex_unlock(&g_malloc_mutex);
}