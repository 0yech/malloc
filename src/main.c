#include "xalloc.h"

int	main(void)
{
	void	*p2;
	void	*p3;

	p2 = xalloc(1);
	p3 = xalloc(1);

	printf("align = %zu\n", (size_t)_Alignof(max_align_t));
	printf("p2 mod = %zu\n",
		(size_t)((uintptr_t)p2 % _Alignof(max_align_t)));

    memset(p2, 'B', 16);

    show_alloc_mem_ex();

	xfree(p2);
	xfree(p3);

    show_alloc_mem_ex();
}