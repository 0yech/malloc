#include "xalloc.h"

void	*xalloc_impl(size_t size)
{
	t_MallocState	*state;
	t_MallBlock		*block;
	t_MallBlock		*rest;
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
		if (ptr != NULL)
			state->active_allocations++;
		return (ptr);
	}

	block = find_free_block(size, type);

	if (block == NULL)
	{
		pagesize = sysconf(_SC_PAGESIZE);
		if (pagesize == (size_t)-1)
			return (NULL);

		if (type == ZONE_TINY)
			zone_size = tiny_zone_size(pagesize);
		else
			zone_size = small_zone_size(pagesize);

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

	return (block_data(block));
}

void	*xalloc(size_t size)
{
	void	*ptr;

	pthread_mutex_lock(&g_malloc_mutex);

	ptr = xalloc_impl(size);

	pthread_mutex_unlock(&g_malloc_mutex);

	return (ptr);
}