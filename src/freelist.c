#include "xalloc.h"

/**
 * @brief Find the appropriate index in a freelist with a given size.
 *
 * @returns the index of the freelist (power of two).
 */
size_t	freelist_index(size_t size)
{
	size_t	index;

	index = 0;
	while (size > 1 && index < 127)
	{
		size >>= 1;
		index++;
	}
	return (index);
}

/**
 * @brief Find an avaible zone with a given zone type and requested sizes.
 *
 * @returns a pointer to an available block.
 */
t_mallblock	*find_free_block(size_t requested, t_zone_type type)
{
	t_mallblock		*cur;
	t_mallblock		*root;
	t_mallocstate	*state;
	size_t			index;
	size_t			align;

	state = get_malloc_state();
	align = align_up(requested, _Alignof(max_align_t));
	index = freelist_index(align);

	while (index < 128)
	{
		root = &state->freelists[index];
		cur = root->free_next;

		while (cur != root)
		{
			if (cur->zone_type == type
				&& cur->blocksize >= align)
				return (cur);
			cur = cur->free_next;
		}

		index++;
	}

	return (NULL);
}

/**
 * @brief Initializes the freelist sentinel.
 */
void	free_list_init(t_mallblock *root)
{
	root->free_next = root;
	root->free_prev = root;
}

/**
 * @brief Check if the freelist is empty.
 *
 * @returns true if the list only contains the sentinel.
 */
bool	free_list_empty(t_mallblock *root)
{
	return (root->free_next == root);
}

/**
 * @brief Inserts a block at the start of the freelist.
 */
void	insert_free_block(t_mallblock *block)
{
	t_mallocstate	*state;
	t_mallblock		*root;
	t_mallblock		*first;
	size_t			index;

	state = get_malloc_state();
	index = freelist_index(block->blocksize);
	root = &state->freelists[index];

	first = root->free_next;

	root->free_next = block;
	block->free_prev = root;
	block->free_next = first;
	first->free_prev = block;
}

/**
 * @brief Removes a block from the freelist.
 */
void	free_list_remove(t_mallblock *block)
{
	block->free_next->free_prev = block->free_prev;
	block->free_prev->free_next = block->free_next;

	block->free_next = NULL;
	block->free_prev = NULL;
}
