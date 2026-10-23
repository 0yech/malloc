#include "xalloc.h"

void	*rexalloc_impl(void *ptr, size_t size)
{
	t_MallocState	*state;
	t_MallBlock		*block;
	t_MallBlock		*next;
	t_MallBlock		*rest;
	void			*new_ptr;
	size_t			copy_size;

	if (ptr == NULL)
		return (xalloc_impl(size));

	if (size == 0)
	{
		xfree_impl(ptr);
		return (NULL);
	}

	state = get_malloc_state();
	block = data_block(ptr);

    // HANDLE LARGE REALLOC TO AVOID MAPPING ABSORB
    if (block->zone_type == ZONE_LARGE)
    {
        if (block->blocksize >= size)
            return (ptr);

        new_ptr = xalloc_impl(size);
        if (new_ptr == NULL)
            return (NULL);

        memcpy(new_ptr, ptr, block->blocksize);
        xfree_impl(ptr);
        return (new_ptr);
    }

	if (block->blocksize >= size)
	{
		if (can_split(block, size))
		{
			rest = split_block(block, size);
			if (rest != NULL)
				insert_free_block(rest);
		}
		return (ptr);
	}

	next = block->phys_next;

	if (next != &state->all_blocks
		&& block_is_free(next)
        && blocks_can_merge(block, next)
        && next->zone_type == block->zone_type
		&& block->blocksize
			+ header_size()
			+ next->blocksize >= size)
	{
		grow_block_into_next(block);

		if (can_split(block, size))
		{
			rest = split_block(block, size);
			if (rest != NULL)
				insert_free_block(rest);
		}

		return (ptr);
	}

	new_ptr = xalloc_impl(size);
	if (new_ptr == NULL)
		return (NULL);

	copy_size = block->blocksize;

	memcpy(new_ptr, ptr, copy_size);

	xfree_impl(ptr);

	return (new_ptr);
}

void	*rexalloc(void *ptr, size_t size)
{
	void	*ret;

	pthread_mutex_lock(&g_malloc_mutex);

	ret = rexalloc_impl(ptr, size);

	pthread_mutex_unlock(&g_malloc_mutex);

	return (ret);
}