/*
The MIT License (MIT)

Copyright (c) 2014-2015 CSAIL, MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/*
#include <linux/module.h>
#include <linux/slab.h>
*/
#include <stdint.h>
#include "types.h"
#include "dev_dragonfire.h"

extern unsigned long long int gc_phy_addr[70];
extern uint8_t* ls2_virt_addr[5];
#if 0
void* bdbm_malloc_atomic(uint32_t a) {
	void* ptr = malloc(a);
	if (ptr == NULL) {
		printf ( "ptr is NULL");
	}
	return ptr;
}
#endif
void bdbm_alloc_pv (local_mem_t *lmem, size_t size)
{
#if THIS_IS_SIMULATION
	lmem->phy = lmem->virt = bdbm_zmalloc (size);
#else
	lmem->phy = (uint8_t *)gc_phy_addr[1];
	lmem->virt = (uint8_t *)ls2_virt_addr[1];
	memset(lmem->virt, 0xff, size);
#endif
}

void bdbm_free_pv (local_mem_t *lmem)
{
#if THIS_IS_SIMULATION
	bdbm_free (lmem->virt);
#else
	/*TODO*/
#endif
}
