#include "xalloc.h"

/**
 * @brief Helper for debug prints.
 *
 * @returns a string of the zone type name.
 */
const char	*zone_name(t_zone_type type)
{
	if (type == ZONE_TINY)
		return ("TINY");
	if (type == ZONE_SMALL)
		return ("SMALL");
	return ("LARGE");
}

/**
 * @brief Prints out a string through write
 */
static void	debug_putstr(const char *str)
{
	size_t	len;

	len = 0;
	while (str[len] != '\0')
		len++;
	write(1, str, len);
}

/**
 * @brief Prints out a size_t through write
 */
static void	debug_putnbr_size(size_t n)
{
	char	buf[32];
	size_t	i;

	if (n == 0)
	{
		write(1, "0", 1);
		return ;
	}
	i = 0;
	while (n > 0)
	{
		buf[i++] = '0' + (n % 10);
		n /= 10;
	}
	while (i > 0)
		write(1, &buf[--i], 1);
}

/**
 * @brief Prints out a number through write
 */
static void	debug_putnbr_int(int n)
{
	char	c;

	if (n < 0)
	{
		write(1, "-", 1);
		n = -n;
	}
	if (n >= 10)
		debug_putnbr_int(n / 10);
	c = '0' + (n % 10);
	write(1, &c, 1);
}

/**
 * @brief Prints out a pointer in hex format
 */
static void	debug_putptr(const void *ptr)
{
	uintptr_t	n;
	char		buf[2 * sizeof(uintptr_t)];
	char		*base;
	size_t		i;

	base = "0123456789abcdef";
	n = (uintptr_t)ptr;
	write(1, "0x", 2);
	if (n == 0)
	{
		write(1, "0", 1);
		return ;
	}
	i = 0;
	while (n > 0)
	{
		buf[i++] = base[n % 16];
		n /= 16;
	}
	while (i > 0)
		write(1, &buf[--i], 1);
}

/**
 * @brief Prints out a byte in hex format
 */
static void	debug_puthex_byte(unsigned char byte)
{
	char	*base;
	char	out[2];

	base = "0123456789abcdef";
	out[0] = base[byte / 16];
	out[1] = base[byte % 16];
	write(1, out, 2);
}

/**
 * @brief Prints out a simple debug on malloc call
 */
void	debug_malloc(size_t size, void *ptr)
{
	debug_putstr("malloc(");
	debug_putnbr_size(size);
	debug_putstr(") - ");
	debug_putptr(ptr);
	debug_putstr("\n");
}

/**
 * @brief Prints out a simple debug on free call
 */
void	debug_free(void *ptr)
{
	debug_putstr("free(");
	debug_putptr(ptr);
	debug_putstr(")\n");
}

void	debug_realloc(void *old_ptr, size_t size, void *new_ptr)
{
	debug_putstr("realloc(");
	debug_putptr(old_ptr);
	debug_putstr(", ");
	debug_putnbr_size(size);
	debug_putstr(") - ");
	debug_putptr(new_ptr);
	debug_putstr("\n");
}

/**
 * @brief Prints out a summary of a single block.
 */
void	print_block_summary(t_mallblock *block)
{
	debug_putstr("block=");
	debug_putptr(block);
	debug_putstr(" size=");
	debug_putnbr_size(block->blocksize);
	debug_putstr(" zone=");
	debug_putstr(zone_name(block->zone_type));
	debug_putstr(" free=");
	debug_putnbr_int(block_is_free(block));
	debug_putstr(" lower=");
	debug_putnbr_int(block->is_lower);
	debug_putstr(" upper=");
	debug_putnbr_int(block->is_upper);
	debug_putstr("\n");
}

/**
 * @brief Helper, prints out an hexadecimal dump of a pointer.
 */
void	print_hex_dump(void *ptr, size_t size)
{
	unsigned char	*data;
	size_t			i;

	data = (unsigned char *)ptr;
	i = 0;
	while (i < size)
	{
		if (i % 16 == 0)
		{
			debug_putptr(data + i);
			debug_putstr(" : ");
		}
		debug_puthex_byte(data[i]);
		debug_putstr(" ");
		i++;
		if (i % 16 == 0 || i == size)
			debug_putstr("\n");
	}
}

/**
 * @brief Prints out an hexadecimal dump of allocated blocks.
 */
void	print_allocated_hex(void)
{
	t_mallocstate	*state;
	t_mallblock		*cur;

	state = get_malloc_state();
	cur = state->all_blocks.phys_next;
	debug_putstr("\nHEX DUMP\n");
	while (cur != &state->all_blocks)
	{
		if (!block_is_free(cur))
		{
			debug_putstr("\n");
			debug_putstr(zone_name(cur->zone_type));
			debug_putstr(" block ");
			debug_putptr(block_data(cur));
			debug_putstr(" - size=");
			debug_putnbr_size(cur->blocksize);
			debug_putstr("\n");
			print_hex_dump(block_data(cur), cur->blocksize);
		}
		cur = cur->phys_next;
	}
}

/**
 * @brief Prints out blocks in physical lists.
 */
void	print_phys_list(void)
{
	t_mallocstate	*state;
	t_mallblock		*cur;
	size_t			count;

	state = get_malloc_state();
	cur = state->all_blocks.phys_next;
	count = 0;
	debug_putstr("\nPHYSICAL LIST\n");
	while (cur != &state->all_blocks)
	{
		print_block_summary(cur);
		count++;
		cur = cur->phys_next;
	}
	debug_putstr("total physical blocks: ");
	debug_putnbr_size(count);
	debug_putstr("\n");
}

/**
 * @brief Prints out freed blocks in freelists.
 */
void	print_free_lists(void)
{
	t_mallocstate	*state;
	t_mallblock		*root;
	t_mallblock		*cur;
	size_t			i;
	size_t			count;

	state = get_malloc_state();
	debug_putstr("\nFREE LISTS\n");
	i = 0;
	while (i < 128)
	{
		root = &state->freelists[i];
		if (!free_list_empty(root))
		{
			debug_putstr("bucket[");
			debug_putnbr_size(i);
			debug_putstr("]:\n");
			cur = root->free_next;
			count = 0;
			while (cur != root)
			{
				debug_putstr("  ");
				print_block_summary(cur);
				count++;
				cur = cur->free_next;
			}
			debug_putstr("  count = ");
			debug_putnbr_size(count);
			debug_putstr("\n");
		}
		i++;
	}
}

/**
 * @brief Prints out the zones in returnable lists.
 */
void	print_returnable_list(void)
{
	t_mallocstate	*state;
	t_mallblock		*cur;
	size_t			count;

	state = get_malloc_state();
	cur = state->returnable.returnable_next;
	count = 0;
	debug_putstr("\nRETURNABLE LIST\n");
	while (cur != &state->returnable)
	{
		print_block_summary(cur);
		count++;
		cur = cur->returnable_next;
	}
	debug_putstr("total returnable blocks: ");
	debug_putnbr_size(count);
	debug_putstr("\n");
}

/**
 * @brief Prints out zones maps per zone type.
 */
size_t	print_allocated_zone(t_zone_type type)
{
	t_mallocstate	*state;
	t_mallblock		*cur;
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
				debug_putstr(zone_name(type));
				debug_putstr(" :\n");
				header_printed = true;
			}
			start = block_data(cur);
			end = (void *)((char *)start + cur->blocksize);
			debug_putptr(start);
			debug_putstr(" - ");
			debug_putptr(end);
			debug_putstr(" : ");
			debug_putnbr_size(cur->blocksize);
			debug_putstr(" bytes\n");
			total += cur->blocksize;
		}
		cur = cur->phys_next;
	}
	return (total);
}

/**
 * @brief Prints out allocated zones/bytes.
 */
void	show_alloc_mem(void)
{
	size_t	total;

	pthread_mutex_lock(&g_malloc_mutex);

	allocator_init();

	total = 0;
	total += print_allocated_zone(ZONE_TINY);
	total += print_allocated_zone(ZONE_SMALL);
	total += print_allocated_zone(ZONE_LARGE);
	debug_putstr("Total : ");
	debug_putnbr_size(total);
	debug_putstr(" bytes\n");
	pthread_mutex_unlock(&g_malloc_mutex);
}

/**
 * @brief Prints out an extended report with an Hexadecimal view.
 */
void	show_alloc_mem_ex(void)
{
	pthread_mutex_lock(&g_malloc_mutex);

	allocator_init();

	debug_putstr("\nMALLOC EXTENDED STATE\n");

	print_phys_list();
	print_free_lists();
	print_returnable_list();
	print_allocated_hex();

	debug_putstr("\n");

	pthread_mutex_unlock(&g_malloc_mutex);
}

/**
 * @brief Prints out a report of all the allocations/lists.
 */
void	show_malloc_state(void)
{
	pthread_mutex_lock(&g_malloc_mutex);

	allocator_init();

	debug_putstr("\n\nMALLOC STATE\n\n");

	print_phys_list();
	print_free_lists();
	print_returnable_list();

	debug_putstr("\n");

	pthread_mutex_unlock(&g_malloc_mutex);
}

/**
 * @brief Prints out ALL information of a block.
 */
void	print_block_info(void *ptr)
{
	t_mallblock	*block;

	block = data_block(ptr);
	debug_putstr("user ptr      = ");
	debug_putptr(ptr);
	debug_putstr("\n");

	debug_putstr("header        = ");
	debug_putptr(block);
	debug_putstr("\n");

	debug_putstr("blocksize     = ");
	debug_putnbr_size(block->blocksize);
	debug_putstr("\n");

	debug_putstr("phys_prev     = ");
	debug_putptr(block->phys_prev);
	debug_putstr("\n");

	debug_putstr("phys_next     = ");
	debug_putptr(block->phys_next);
	debug_putstr("\n");

	debug_putstr("free_prev     = ");
	debug_putptr(block->free_prev);
	debug_putstr("\n");

	debug_putstr("free_next     = ");
	debug_putptr(block->free_next);
	debug_putstr("\n");

	debug_putstr("return_prev   = ");
	debug_putptr(block->returnable_prev);
	debug_putstr("\n");

	debug_putstr("return_next   = ");
	debug_putptr(block->returnable_next);
	debug_putstr("\n");

	debug_putstr("is_lower      = ");
	debug_putnbr_int(block->is_lower);
	debug_putstr("\n");

	debug_putstr("is_upper      = ");
	debug_putnbr_int(block->is_upper);
	debug_putstr("\n");

	debug_putstr("alignment mod = ");
	debug_putnbr_size(
		(size_t)((uintptr_t)ptr % _Alignof(max_align_t)));
	debug_putstr("\n\n");
}
