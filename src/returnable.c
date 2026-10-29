#include "xalloc.h"

/**
 * @brief Initialize the returnable list.
 */
void	returnable_list_init(t_mallblock *root)
{
	root->returnable_next = root;
	root->returnable_prev = root;
}

/**
 * @brief Counts returnable zones.
 *
 * When exceeding MAX_RETURNABLE_ZONES,
 * older returnable zones are munmap()'d.
 */
size_t	returnable_count(void)
{
	t_mallocstate	*state;
	t_mallblock		*cur;
	size_t			count;

	state = get_malloc_state();
	cur = state->returnable.returnable_next;
	count = 0;

	while (cur != &state->returnable)
	{
		count++;
		cur = cur->returnable_next;
	}

	return (count);
}

/**
 * @brief Checks if a block is returnable.
 *
 * @returns true if the node is returnable. 
 */
bool	block_is_returnable(t_mallblock *block)
{
	return (block->is_returnable);
}

t_mallblock	*find_returnable_block(size_t requested, t_zone_type type)
{
	t_mallocstate	*state;
	t_mallblock		*cur;
	size_t			align;

	state = get_malloc_state();
	align = align_up(requested, _Alignof(max_align_t));
	cur = state->returnable.returnable_next;
	while (cur != &state->returnable)
	{
		if (cur->zone_type == type && cur->blocksize >= align)
			return (cur);
		cur = cur->returnable_next;
	}
	return (NULL);
}

/**
 * @brief Adds a block to the start of the returnable list.
 */
void	returnable_insert(t_mallblock *root, t_mallblock *block)
{
	t_mallblock	*first;

	first = root->returnable_next;

	root->returnable_next = block;
	block->returnable_prev = root;
	block->returnable_next = first;
	first->returnable_prev = block;
	block->is_returnable = true;
}

/**
 * @brief remove a block to the start of the returnable list.
 */
void	returnable_remove(t_mallblock *block)
{
	block->returnable_next->returnable_prev = block->returnable_prev;
	block->returnable_prev->returnable_next = block->returnable_next;

	block->returnable_next = NULL;
	block->returnable_prev = NULL;
	block->is_returnable = false;
}

/**
 * @brief munmap()s a returnable zone/bock.
 */
void	unmap_returnable_zone(t_mallblock *block)
{
	size_t	mapping_size;

	mapping_size = header_size() + block->blocksize;

	returnable_remove(block);

	block->phys_prev->phys_next = block->phys_next;
	block->phys_next->phys_prev = block->phys_prev;

	munmap(block, mapping_size);
}

/**
 * @brief munmap()s all returnable zones/bocks. 
 */
void	trim_all_returnable(void)
{
	t_mallocstate	*state;
	t_mallblock		*block;

	state = get_malloc_state();

	while (state->returnable.returnable_next != &state->returnable)
	{
		block = state->returnable.returnable_next;
		unmap_returnable_zone(block);
	}
}

/**
 * @brief munmap()s older zones exceeding MAX_RETURNABLE_ZONES.
 */
void	trim_returnable_excess(void)
{
	t_mallocstate	*state;
	t_mallblock		*block;
	size_t			count;

	state = get_malloc_state();
	count = returnable_count();

	while (count > MAX_RETURNABLE_ZONES)
	{
		block = state->returnable.returnable_prev;
		unmap_returnable_zone(block);
		count--;
	}
}
