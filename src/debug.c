#include "xalloc.h"

const char	*zone_name(t_zone_type type)
{
	if (type == ZONE_TINY)
		return ("TINY");
	if (type == ZONE_SMALL)
		return ("SMALL");
	return ("LARGE");
}

void	print_block_summary(t_MallBlock *block)
{
	printf(
		"block=%p size=%zu zone=%s free=%d lower=%d upper=%d\n",
		(void *)block,
		block->blocksize,
		zone_name(block->zone_type),
		block_is_free(block),
		block->is_lower,
		block->is_upper
	);
}

void	print_hex_dump(void *ptr, size_t size)
{
	unsigned char	*data;
	size_t			i;

	data = (unsigned char *)ptr;
	i = 0;

	while (i < size)
	{
		if (i % 16 == 0)
			printf("%p : ", (void *)(data + i));

		printf("%02x ", data[i]);

		i++;

		if (i % 16 == 0 || i == size)
			printf("\n");
	}
}

void	print_allocated_hex(void)
{
	t_MallocState	*state;
	t_MallBlock		*cur;

	state = get_malloc_state();
	cur = state->all_blocks.phys_next;

	printf("\nHEX DUMP\n");

	while (cur != &state->all_blocks)
	{
		if (!block_is_free(cur))
		{
			printf(
				"\n%s block %p - size=%zu\n",
				zone_name(cur->zone_type),
				block_data(cur),
				cur->blocksize
			);

			print_hex_dump(
				block_data(cur),
				cur->blocksize
			);
		}

		cur = cur->phys_next;
	}
}

void	print_phys_list(void)
{
	t_MallocState	*state;
	t_MallBlock		*cur;
	size_t			count;

	state = get_malloc_state();
	cur = state->all_blocks.phys_next;
	count = 0;

	printf("\nPHYSICAL LIST\n");

	while (cur != &state->all_blocks)
	{
		print_block_summary(cur);
		count++;
		cur = cur->phys_next;
	}

	printf("total physical blocks: %zu\n", count);
}

void	print_free_lists(void)
{
	t_MallocState	*state;
	t_MallBlock		*root;
	t_MallBlock		*cur;
	size_t			i;
	size_t			count;

	state = get_malloc_state();

	printf("\nFREE LISTS\n");

	i = 0;
	while (i < 128)
	{
		root = &state->freelists[i];

		if (!free_list_empty(root))
		{
			printf("bucket[%zu]:\n", i);

			cur = root->free_next;
			count = 0;

			while (cur != root)
			{
				printf("  ");
				print_block_summary(cur);
				count++;
				cur = cur->free_next;
			}

			printf("  count = %zu\n", count);
		}
		i++;
	}
}

void	print_returnable_list(void)
{
	t_MallocState	*state;
	t_MallBlock		*cur;
	size_t			count;

	state = get_malloc_state();
	cur = state->returnable.returnable_next;
	count = 0;

	printf("\nRETURNABLE LIST\n");

	while (cur != &state->returnable)
	{
		print_block_summary(cur);
		count++;
		cur = cur->returnable_next;
	}

	printf("total returnable blocks: %zu\n", count);
}

size_t	print_allocated_zone(t_zone_type type)
{
	t_MallocState	*state;
	t_MallBlock		*cur;
	void			*start;
	void			*end;
	size_t			total;
	bool			header_printed;

	state = get_malloc_state();
	cur = state->all_blocks.phys_next;
	total = 0;
	header_printed = false;

	while (cur != &state->all_blocks)
	{
		if (cur->zone_type == type && !block_is_free(cur))
		{
			if (!header_printed)
			{
				printf("%s :\n", zone_name(type));
				header_printed = true;
			}

			start = block_data(cur);
			end = (void *)((char *)start + cur->blocksize);

			printf("%p - %p : %zu bytes\n",
				start,
				end,
				cur->blocksize);

			total += cur->blocksize;
		}

		cur = cur->phys_next;
	}

	return (total);
}

void	show_alloc_mem(void)
{
	size_t	total;

    pthread_mutex_lock(&g_malloc_mutex);

	total = 0;

	total += print_allocated_zone(ZONE_TINY);
	total += print_allocated_zone(ZONE_SMALL);
	total += print_allocated_zone(ZONE_LARGE);

	printf("Total : %zu bytes\n", total);

    pthread_mutex_unlock(&g_malloc_mutex);
}

void	show_alloc_mem_ex(void)
{
    pthread_mutex_lock(&g_malloc_mutex);

	printf("\nMALLOC EXTENDED STATE\n");

	print_phys_list();
	print_free_lists();
	print_returnable_list();
	print_allocated_hex();

	printf("\n");

    pthread_mutex_unlock(&g_malloc_mutex);
}

void	show_malloc_state(void)
{
    pthread_mutex_lock(&g_malloc_mutex);

	printf("\n\nMALLOC STATE\n\n");

	print_phys_list();
	print_free_lists();
	print_returnable_list();

	printf("\n");

    pthread_mutex_unlock(&g_malloc_mutex);
}

void	print_block_info(void *ptr)
{
	t_MallBlock	*block;

	block = data_block(ptr);

	printf("user ptr      = %p\n", ptr);
	printf("header        = %p\n", (void *)block);
	printf("blocksize     = %zu\n", block->blocksize);

	printf("phys_prev     = %p\n", (void *)block->phys_prev);
	printf("phys_next     = %p\n", (void *)block->phys_next);

	printf("free_prev     = %p\n", (void *)block->free_prev);
	printf("free_next     = %p\n", (void *)block->free_next);

	printf("return_prev   = %p\n", (void *)block->returnable_prev);
	printf("return_next   = %p\n", (void *)block->returnable_next);

	printf("is_lower      = %d\n", block->is_lower);
	printf("is_upper      = %d\n", block->is_upper);

	printf("alignment mod = %zu\n",
		(size_t)((uintptr_t)ptr % _Alignof(max_align_t)));

	printf("\n");
}