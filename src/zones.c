#include "xalloc.h"

/**
 * @brief Returns the type of yone from its size in bytes
 */
t_zone_type	get_zone_type(size_t size)
{
	if (size <= TINY_MAX)
		return (ZONE_TINY);
	if (size <= SMALL_MAX)
		return (ZONE_SMALL);
	return (ZONE_LARGE);
}

/**
 * @brief Calculates the mmap size for a TINY/SMALL zone.
 *
 * The zone is large enough to hold at least ZONE_MIN_ALLOCS blocks
 * of the max payload size for the given zone type, including headers.
 *
 * Final size is rounded up to a multiple of the system page size.
 * Overflow checks are done to prevent size_t overflow (SIZE_MAX)
 */
size_t	zone_size_for(size_t max_payload, size_t pagesize)
{
	size_t	aligned_payload;
	size_t	one_block;
	size_t	raw_size;
	size_t	zone_size;

	aligned_payload = align_up(max_payload, _Alignof(max_align_t));
	if (size_add_overflow(header_size(), aligned_payload, &one_block))
		return (0);
	if (size_mul_overflow(ZONE_MIN_ALLOCS, one_block, &raw_size))
		return (0);
	if (!align_up_safe(raw_size, pagesize, &zone_size))
		return (0);

	return (zone_size);
}

// throw
size_t	tiny_zone_size(size_t pagesize)
{
	return (zone_size_for(TINY_MAX, pagesize));
}

size_t	small_zone_size(size_t pagesize)
{
	return (zone_size_for(SMALL_MAX, pagesize));
}

/**
 * @brief Creates a new mmap() zone with a given TINY/SMALL type
 *
 * "first" stores the first block before adding the new one from the
 * new zone in case blocks already exist
 *
 * sentinel-new-A-B-C-sentinel
 *
 * newly created block is added to the freelist for use.
 *
 * @returns a pointer to the header of the first block in the zone
 */
t_MallBlock	*create_zone(size_t zone_size, t_zone_type type)
{
	t_MallBlock		*block;
	t_MallocState	*state;
    t_MallBlock		*first;

	state = get_malloc_state();

	block = mmap(
		NULL,
		zone_size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS,
		-1,
		0
	);
	if (block == MAP_FAILED)
		return (NULL);

	block->blocksize = zone_size - header_size();
    block->zone_type = type;

	block->is_lower = true;
	block->is_upper = true;

    block->free_next = NULL;
    block->free_prev = NULL;

    block->returnable_next = NULL;
    block->returnable_prev = NULL;

    first = state->all_blocks.phys_next;

    block->phys_prev = &state->all_blocks;
    block->phys_next = first;

    state->all_blocks.phys_next = block;
    first->phys_prev = block;

    insert_free_block(block);

	return (block);
}

/**
 * @brief Creates a new mmap() zone for a specific LARGE size
 *
 * Acts similarly as create_zone but for an arbitrary size.
 * LARGE zones are immediatly munmap()'d when freed.
 *
 * @returns a pointer to the header of the block
 */
void	*alloc_large(size_t size)
{
	t_MallocState	*state;
	t_MallBlock		*block;
	t_MallBlock		*first;
	size_t			pagesize;
	size_t			mapping_size;
    size_t          raw_size;

	state = get_malloc_state();

	pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize == (size_t)-1)
		return (NULL);

    if (size_add_overflow(header_size(), size, &raw_size))
        return (NULL);

	if (!align_up_safe(raw_size, pagesize, &mapping_size))
        return (NULL);

	block = mmap(
			NULL,
			mapping_size,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS,
			-1,
			0
		);

	if (block == MAP_FAILED)
		return (NULL);

	block->blocksize = mapping_size - header_size();

	block->zone_type = ZONE_LARGE;

	block->is_lower = true;
	block->is_upper = true;

	block->free_next = NULL;
	block->free_prev = NULL;

	block->returnable_next = NULL;
	block->returnable_prev = NULL;

	first = state->all_blocks.phys_next;

	block->phys_prev = &state->all_blocks;
	block->phys_next = first;

	state->all_blocks.phys_next = block;
	first->phys_prev = block;

	return (block_data(block));
}