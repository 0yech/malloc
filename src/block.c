#include "xalloc.h"

size_t	header_size(void)
{
	return (align_up(sizeof(t_MallBlock), _Alignof(max_align_t)));
}

// return data from header
void *block_data(t_MallBlock *block)
{
    // return ((void *)(block + 1))
    return ((void *)((char *)block + header_size()));
}

// return header from data
t_MallBlock *data_block(void *ptr)
{
    return ((t_MallBlock *)((char *)ptr - header_size()));
}

//get next block header (cause align padding)
t_MallBlock *next_block_addr(t_MallBlock *block)
{
    size_t align;
    
    align = align_up(block->blocksize, _Alignof(max_align_t));
    return ((t_MallBlock *)((char *)block + header_size() + align));
}

// enough space remaining to add a block from requested size
bool	can_split(t_MallBlock *block, size_t requested)
{
    size_t align;

    align = align_up(requested, _Alignof(max_align_t));
    return (block->blocksize >= (align + header_size() + MIN_SPLIT_DATA));
}

// add a block in list
t_MallBlock	*split_block(t_MallBlock *block, size_t requested)
{
    size_t  align;
    size_t  old_size;
    size_t  new_size;
    bool    old_upper;
    t_MallBlock *new;
    t_MallBlock *tmp;

    old_size = block->blocksize;
    old_upper = block->is_upper;
    align = align_up(requested, _Alignof(max_align_t));

    if (!can_split(block, requested))
        return (NULL);

    new = (t_MallBlock *)((char *)block + header_size() + align);
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

bool	blocks_can_merge(t_MallBlock *lower, t_MallBlock *upper)
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

// merge two adjacent blocks in freelist into one.
t_MallBlock	*merge_blocks(t_MallBlock *lower, t_MallBlock *upper)
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

t_MallBlock	*grow_block_into_next(t_MallBlock *block)
{
	t_MallBlock	*next;

	next = block->phys_next;

	free_list_remove(next);

	block->blocksize += header_size() + next->blocksize;

	block->phys_next = next->phys_next;
	next->phys_next->phys_prev = block;

	block->is_upper = next->is_upper;

	return (block);
}

bool	block_is_free(t_MallBlock *block)
{
	return (block->free_next != NULL && block->free_prev != NULL);
}