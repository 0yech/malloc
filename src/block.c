#include "xalloc.h"

/**
 * @brief Helper function for header
 *
 * @returns the size of the header/block struct.
 */
size_t	header_size(void)
{
	return (align_up(sizeof(t_mallblock), _Alignof(max_align_t)));
}

/**
 * @brief Offset helper to get the data pointer from a header.
 *
 * @returns a pointer to the data of a block from its header.
 */
void	*block_data(t_mallblock *block)
{
	return ((void *)((char *)block + header_size()));
}

/**
 * @brief Offset helper to get header from data pointer.
 *
 * @returns the block header from a pointer to its data.
 */
t_mallblock	*data_block(void *ptr)
{
	return ((t_mallblock *)((char *)ptr - header_size()));
}

/**
 * @brief Finds the allocated block matching a user pointer.
 *
 * Traverses the physical block list and compares the data address
 * of each known block with ptr.
 *
 * @return Matching block header, or NULL if invalid.
 */
t_mallblock	*find_block_from_ptr(void *ptr)
{
	t_mallocstate	*state;
	t_mallblock		*block;

	state = get_malloc_state();
	block = state->all_blocks.phys_next;
	while (block != &state->all_blocks)
	{
		if (block_data(block) == ptr)
			return (block);
		block = block->phys_next;
	}
	return (NULL);
}

/**
 * @brief Checks if a freelist block can be split.
 *
 * Request alloc size, header size, align and an arbitrary size
 * are taken in count to check if there's enough space in the zone.
 *
 * @returns true if the zone can hold the block.
 */
bool	can_split(t_mallblock *block, size_t requested)
{
	size_t	align;

	align = align_up(requested, _Alignof(max_align_t));
	return (block->blocksize >= (align + header_size() + MIN_SPLIT_DATA));
}

/**
 * @brief Creates a block with a requested size, rest goes in freelist.
 *
 * Remainings of the first alloc stays at its full zone size until a
 * new alloc is requested. Upon a new alloc requested, you split/slice
 * a part of that bigger block.
 *
 * Basically you start with a single cake slice. Remainings are the rest
 * of the cake. And then you keep slicing the cake. "requested" is the size of
 * the wanted slice. Slice goes to the plate, cake goes to the fridge (freelist).
 *
 * @returns the new requested block.
 */
t_mallblock	*split_block(t_mallblock *block, size_t requested)
{
	t_mallblock	*new;
	t_mallblock	*tmp;
	size_t		align;
	size_t		old_size;
	size_t		new_size;
	bool		old_upper;

	old_size = block->blocksize;
	old_upper = block->is_upper;
	align = align_up(requested, _Alignof(max_align_t));

	if (!can_split(block, requested))
		return (NULL);

	new = (t_mallblock *)((char *)block + header_size() + align);
	new_size = old_size - align - header_size();
	block->blocksize = align;
	new->blocksize = new_size;
	new->zone_type = block->zone_type;

	tmp = block->phys_next;

	new->phys_prev = block;
	new->phys_next = tmp;
	new->free_next = NULL;
	new->free_prev = NULL;
	new->returnable_next = NULL;
	new->returnable_prev = NULL;

	block->phys_next = new;
	tmp->phys_prev = new;

	block->is_upper = false;

	new->is_lower = false;
	new->is_upper = old_upper;

	return (new);
}

/**
 * @brief Checks if two blocks can be merged.
 *
 * Block have to be physically adjacent.
 * Lowest block in list can't be the highest one and vice-versa
 *
 * @returns true if mergeable.
 */
bool	blocks_can_merge(t_mallblock *lower, t_mallblock *upper)
{
	if (lower == NULL || upper == NULL)
		return (false);

	if (lower->phys_next != upper)
		return (false);

	if (lower->is_upper)
		return (false);

	if (upper->is_lower)
		return (false);

	return (true);
}

/**
 * @brief Merges two physically adjacent free blocks into a larger block.
 *
 * Newly merged block is placed back into the freelist.
 *
 * @returns the new merged block.
 */
t_mallblock	*merge_blocks(t_mallblock *lower, t_mallblock *upper)
{
	if (!blocks_can_merge(lower, upper))
		return (NULL);

	free_list_remove(lower);
	free_list_remove(upper);

	lower->blocksize += header_size() + upper->blocksize;

	lower->phys_next = upper->phys_next;
	upper->phys_next->phys_prev = lower;

	lower->is_upper = upper->is_upper;

	insert_free_block(lower);

	return (lower);
}

/**
 * @brief Extends a physical block using the next one.
 *
 * Used in realloc().
 *
 * @returns new extended block.
 */
t_mallblock	*grow_block_into_next(t_mallblock *block)
{
	t_mallblock	*next;

	next = block->phys_next;

	free_list_remove(next);

	block->blocksize += header_size() + next->blocksize;

	block->phys_next = next->phys_next;
	next->phys_next->phys_prev = block;

	block->is_upper = next->is_upper;

	return (block);
}

/**
 * @brief Checks if a block is free.
 *
 * @returns true if free.
 */
bool	block_is_free(t_mallblock *block)
{
	return (block->free_next != NULL && block->free_prev != NULL);
}
