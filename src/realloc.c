#include "xalloc.h"

/**
 * @brief Reallocates a LARGE block.
 *
 * Keeps the current mapping if it is already large enough.
 * Otherwise, allocates a new block, copies the existing data,
 * and frees the old LARGE mapping.
 *
 * @return Pointer to the reallocated data, or NULL on allocation failure.
 */
static void	*realloc_large(void *ptr, t_mallblock *block, size_t size)
{
	void	*new_ptr;

	if (block->blocksize >= size)
		return (ptr);

	new_ptr = xalloc_impl(size);
	if (new_ptr == NULL)
		return (NULL);

	memcpy(new_ptr, ptr, block->blocksize);
	xfree_impl(ptr);

	return (new_ptr);
}

/**
 * @brief Moves an allocation to a different zone type.
 *
 * Allocates a new block in a zone matching the requested size,
 * copies as much existing data as possible then frees the old block.
 *
 * @return Pointer to the new alloc, or NULL fail.
 */
static void	*realloc_new_zone(void *ptr, t_mallblock *block, size_t size)
{
	void	*new_ptr;
	size_t	copy_size;

	new_ptr = xalloc_impl(size);

	if (new_ptr == NULL)
		return (NULL);

	copy_size = block->blocksize;

	if (copy_size > size)
		copy_size = size;

	memcpy(new_ptr, ptr, copy_size);
	xfree_impl(ptr);

	return (new_ptr);
}

/**
 * @brief Shrinks an allocation in place when possible.
 *
 * Keeps the same block and splits its unused remainder if the remaining
 * space is large enough to create another free block.
 *
 * @return The original pointer.
 */
static void	*realloc_shrink(void *ptr, t_mallblock *block, size_t size)
{
	t_mallblock	*rest;

	if (can_split(block, size))
	{
		rest = split_block(block, size);
		if (rest != NULL)
			insert_free_block(rest);
	}
	return (ptr);
}

/**
 * @brief Tries to grow a block into its next physical free block.
 *
 * The next block must be free and belong to the same zone
 * and provide enough combined space for the requested size.
 * Any unused remainder is split and returned to the freelist.
 *
 * @return The original pointer if block expanded or NULL.
 */
static void	*realloc_expand(void *ptr, t_mallblock *block, size_t size)
{
	t_mallocstate	*state;
	t_mallblock		*next;
	t_mallblock		*rest;

	state = get_malloc_state();
	next = block->phys_next;

	if (next == &state->all_blocks
		|| !block_is_free(next)
		|| !blocks_can_merge(block, next)
		|| next->zone_type != block->zone_type
		|| block->blocksize + header_size() + next->blocksize < size)
		return (NULL);

	grow_block_into_next(block);

	if (can_split(block, size))
	{
		rest = split_block(block, size);
		if (rest != NULL)
			insert_free_block(rest);
	}

	return (ptr);
}

/**
 * @brief Actual implementation of realloc.
 *
 * Chooses between keeping the current block, moving to another zone type,
 * growing into the next free block, or allocating a new block.
 *
 * @return Pointer to the reallocated data.
 */
void	*rexalloc_impl(void *ptr, size_t size)
{
	t_mallblock		*block;
	t_zone_type		new_type;
	void			*new_ptr;

	if (ptr == NULL)
		return (xalloc_impl(size));

	if (size == 0)
	{
		xfree_impl(ptr);
		return (NULL);
	}

	block = find_block_from_ptr(ptr);
	if (block == NULL || block_is_free(block))
		return (NULL);
	new_type = get_zone_type(size);

	if (block->zone_type == ZONE_LARGE)
		return (realloc_large(ptr, block, size));

	if (new_type != block->zone_type)
		return (realloc_new_zone(ptr, block, size));

	if (block->blocksize >= size)
		return (realloc_shrink(ptr, block, size));

	new_ptr = realloc_expand(ptr, block, size);
	if (new_ptr != NULL)
		return (new_ptr);

	return (realloc_new_zone(ptr, block, size));
}

/**
 * @brief Reallocates a pointer  with its new size.
 *
 * Actual implementation is done in realloc_impl(),
 * realloc() here is simply a wrapper that locks the mutex.
 *
 * @returns the pointer to the new block's data.
 */
void	*realloc(void *ptr, size_t size)
{
	t_mallocstate	*state;
	void			*ret;

	pthread_mutex_lock(&g_malloc_mutex);

	ret = rexalloc_impl(ptr, size);
	state = get_malloc_state();

	if (state->debug)
		debug_realloc(ptr, size, ret);

	pthread_mutex_unlock(&g_malloc_mutex);

	return (ret);
}
