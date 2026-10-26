#include "xalloc.h"

/**
 * @brief Initialize the physical sentinel.
 */
void	phys_list_init(t_mallblock *root)
{
	root->phys_next = root;
	root->phys_prev = root;
}
