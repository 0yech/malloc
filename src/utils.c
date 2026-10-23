#include "xalloc.h"

/**
 * @brief Rounds up size to the next multiple of alignment.
 *
 * @returns false if the operation would overflow size_t
 */
bool	align_up_safe(size_t size, size_t alignment, size_t *result)
{
	size_t	rem;
	size_t	add;

	if (alignment == 0)
		return (false);

	rem = size % alignment;
	if (rem == 0)
	{
		*result = size;
		return (true);
	}

	add = alignment - rem;

	if (size > SIZE_MAX - add)
		return (false);

	*result = size + add;
	return (true);
}

/**
 * @brief Rounds up size to the next multiple of alignment.
 *
 * @returns new size with alignement
 */
size_t align_up(size_t size, size_t alignment)
{
    size_t ret;

    ret = size % alignment;
    if (ret == 0)
        return (size);
    ret = alignment - ret;
    return (size + ret);
}

/**
 * @brief Adds two sizes and checks for overflows on size_t's max
 *
 * @returns true if it overflows
 */
bool	size_add_overflow(size_t a, size_t b, size_t *result)
{
	if (a > SIZE_MAX - b)
		return (true);
	*result = a + b;
	return (false);
}

/**
 * @brief Multiplies two sizes and checks for overflows on size_t's max
 *
 * Used to check if ZONE_MIN_ALLOCS blocks fits in a zone
 *
 * @returns true if it overflows
 */
bool	size_mul_overflow(size_t a, size_t b, size_t *result)
{
	if (a != 0 && b > SIZE_MAX / a)
		return (true);
	*result = a * b;
	return (false);
}