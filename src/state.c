#include "xalloc.h"
#include <stdlib.h>

pthread_mutex_t	g_malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Schizo fake global variable with extra steps.
 *
 * @returns the static state of the malloc implementation.
 */
t_mallocstate	*get_malloc_state(void)
{
	static t_mallocstate	state;

	return (&state);
}

static bool	env_enabled(const char *name)
{
	char	*value;

	value = getenv(name);
	if (value == NULL)
		return (false);
	return (value[0] == '1');
}

/**
 * @brief Initiate the allocator on first malloc call.
 *
 * skips any call if already initialized once.
 */
void	allocator_init(void)
{
	t_mallocstate	*state;
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
	state->debug = env_enabled("MALLOC_DEBUG");
	state->scribble = env_enabled("MALLOC_SCRIBBLE");
	state->show_state = env_enabled("MALLOC_SHOW_STATE");
}
