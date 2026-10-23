#include "xalloc.h"

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

void	insert_free_block(t_MallBlock *block)
{
    t_MallocState *state;
    size_t index;

    state = get_malloc_state();
    index = freelist_index(block->blocksize);
    free_list_insert(&state->freelists[index], block);
}

t_MallBlock	*find_free_block(size_t requested, t_zone_type type)
{
    t_MallBlock *cur;
    t_MallBlock *root;
    t_MallocState *state;
    size_t index;
    size_t align;

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

void	free_list_init(t_MallBlock *root)
{
    root->free_next = root;
    root->free_prev = root;
}

bool	free_list_empty(t_MallBlock *root)
{
    return (root->free_next == root);
}

void	free_list_insert(t_MallBlock *root, t_MallBlock *block)
{
	t_MallBlock	*first;

	first = root->free_next;

    root->free_next = block;
    block->free_prev = root;
    block->free_next = first;
    first->free_prev = block;
}

void	free_list_remove(t_MallBlock *block)
{
    block->free_next->free_prev = block->free_prev;
    block->free_prev->free_next = block->free_next;

    block->free_next = NULL;
    block->free_prev = NULL;
}
