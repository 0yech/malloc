#include "xalloc.h"

pthread_mutex_t	g_malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Schizo fake global variable with extra steps 
 *
 * @returns the static state of the malloc implementation
 */
t_MallocState	*get_malloc_state(void)
{
	static t_MallocState	state;

	return (&state);
}

/**
 * @brief Initiate the allocator on first malloc call.
 *
 * skips any call if already initialized once.
 */
void	allocator_init(void)
{
	t_MallocState	*state;
	size_t			i;

	state = get_malloc_state();
	if (state->initialized)
		return ;

	i = 0;
	while (i < 128)
	{
		free_list_init(&state->freelists[i]);
		i++;
	}
	phys_list_init(&state->all_blocks);
	returnable_list_init(&state->returnable);
	state->initialized = true;
    state->active_allocations = 0;
}