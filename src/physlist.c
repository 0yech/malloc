#include "xalloc.h"

void	phys_list_init(t_MallBlock *root)
{
    root->phys_next = root;
    root->phys_prev = root;
}
