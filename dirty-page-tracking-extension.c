/*
 * Copyright (c) 2011-2012
 * National Institute of Advanced Industrial Science and Technology
 *
 * Author: Takahiro Hirofuchi
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"
#include "qemu-common.h"

#include <sys/mman.h>

#include "memory.h"
#include "exec-memory.h"
#include "error.h"
#define WANT_EXEC_OBSOLETE
#include "exec-obsolete.h"

#include "qemu-timer.h"


/* bitmap operations */

/* some of the below definitions are from Linux kernel */
#define DIV_ROUND_UP(n,d)       (((n) + (d) - 1) / (d))
#define BITS_PER_BYTE           8
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))
#define BITS_PER_LONG           (sizeof(unsigned long) * BITS_PER_BYTE)

static size_t bitmap_length(size_t nbits)
{
	unsigned long narrays = BITS_TO_LONGS(nbits);
	size_t buflen = sizeof(unsigned long) * narrays;

	return buflen;
}

static void bitmap_on(unsigned long *bitmap_array, unsigned long block_index)
{
	unsigned long bitmap_index = block_index / BITS_PER_LONG;
	unsigned long *bitmap = &(bitmap_array[bitmap_index]);

	*bitmap |= (1UL << (block_index % BITS_PER_LONG));
}

static unsigned long *create_bm(const char *bmpath, size_t nbits, size_t *bmlen)
{
	size_t buflen = bitmap_length(nbits);

	int bmfd = open(bmpath, O_CREAT | O_RDWR | O_EXCL, 0644);
	if (bmfd < 0) {
		fprintf(stderr, "open bmpath failed, %s\n", bmpath);
		exit(1);
	}

	int ret = ftruncate(bmfd, buflen);
	if (ret < 0) {
		fprintf(stderr, "ftruncate failed\n");
		exit(1);
	}

	unsigned long *ptr = mmap(NULL, buflen, PROT_READ | PROT_WRITE, MAP_SHARED, bmfd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "bm mmap failed\n");
		exit(1);
	}

	close(bmfd);

	*bmlen = buflen;

	return ptr;
}

static void close_bm(unsigned long *bm, size_t bmlen)
{
	int ret;
	ret = msync(bm, bmlen, MS_SYNC);
	if (ret < 0) {
		fprintf(stderr, "msync failed\n");
		exit(1);
	}

	ret = munmap(bm, bmlen);
	if (ret < 0) {
		fprintf(stderr, "munmap failed\n");
		exit(1);
	}
}


#define DPT_INTERVAL 1000

void dpt_bitmap_write(time_t timestamp)
{
	ram_addr_t qemu_bmsize = qemu_last_ram_offset() >> TARGET_PAGE_BITS;


	const char *bmpath_prefix = "/tmp/qemu.bm";
	char *bmpath = g_strdup_printf("%s-%d.%lld", bmpath_prefix, getpid(), (long long) timestamp);

	size_t bmlen;
	unsigned long *bm = create_bm(bmpath, qemu_bmsize, &bmlen);



	unsigned long ndirty = 0;

	RAMBlock *block;
	QLIST_FOREACH(block, &ram_list.blocks, next) {
		ram_addr_t offset;

		assert((block->offset % TARGET_PAGE_SIZE) == 0);
		assert((block->length % TARGET_PAGE_SIZE) == 0);

		for (offset = 0; offset < block->length; offset += TARGET_PAGE_SIZE) {
			if (memory_region_get_dirty(block->mr, offset, TARGET_PAGE_SIZE, DIRTY_MEMORY_MIGRATION)) {
				bitmap_on(bm, (block->offset + offset) / TARGET_PAGE_SIZE);
				ndirty += 1;
			}
		}

	}

	fprintf(stderr, "create bitmap %s (%zu bytes), dirty %lu / %zu (%.3f MB/s)\n", bmpath, bmlen, ndirty, qemu_bmsize, 1.0 * ndirty * 4096 / 1024 / 1024 / (0.001 * DPT_INTERVAL));

	close_bm(bm, bmlen);
	g_free(bmpath);
}



void qmp_dpt_sync(Error **errp);

static QEMUTimer *dpt_timer = NULL;

void timer_update(void *opaque)
{
	// fprintf(stderr, "timer called\n");
	qmp_dpt_sync(NULL);
	qemu_mod_timer(dpt_timer, qemu_get_clock_ms(rt_clock) + DPT_INTERVAL);
}

static void private_memory_region_set_dirty(MemoryRegion *mr, target_phys_addr_t addr,
		target_phys_addr_t size, unsigned client)
{
	assert(mr->terminates);
	return cpu_physical_memory_set_dirty_range(mr->ram_addr + addr, size, 1 << client);
}

static void make_all_pages_dirty(void)
{
	RAMBlock *block;

	QLIST_FOREACH(block, &ram_list.blocks, next) {
		private_memory_region_set_dirty(block->mr, 0, block->length, DIRTY_MEMORY_MIGRATION);
	}
}

static void make_all_pages_clean(void)
{
	RAMBlock *block;

	QLIST_FOREACH(block, &ram_list.blocks, next) {
		/* block->{offset,length} show the view from the host. */
		memory_region_reset_dirty(block->mr, 0, block->length, DIRTY_MEMORY_MIGRATION);
	}
}


void qmp_dpt_start(Error **errp)
{
        /* mark all pages dirty */
	make_all_pages_dirty();
	time_t timestamp = time(NULL);
	dpt_bitmap_write(timestamp);
	make_all_pages_clean();

        memory_global_dirty_log_start();

	/* create timer */
	dpt_timer = qemu_new_timer_ms(rt_clock, timer_update, NULL);
	qemu_mod_timer(dpt_timer, qemu_get_clock_ms(rt_clock) + DPT_INTERVAL);

	fprintf(stderr, "start dirty page tracking\n");
}


/*
 * Before we stop dirty page tracking, the VM must be stopped at the source
 * host, so that the final bitmap write captures all updates.
 **/
void qmp_dpt_stop(Error **errp)
{
	/* delete timer */
	qemu_del_timer(dpt_timer);
	qemu_free_timer(dpt_timer);
	dpt_timer = NULL;

	/*
	 * Final write. The increment makes sure we use a different bitmap
	 * path. This is because it is possible that the last timer happened
	 * less than 1 second ago.
	 *
	 * dpt_start must not be called just after here.
	 **/
	time_t timestamp = time(NULL) + 1;
	memory_global_sync_dirty_bitmap(get_system_memory());
	dpt_bitmap_write(timestamp);

	/* (NOT TRUE??) It looks log_stop clears KVM's dirty bitmaps. This must be called after the final write. */
	memory_global_dirty_log_stop();

	fprintf(stderr, "stop dirty page tracking\n");
}


// global_sync => get_dirty (i.e., check phys_dirty) => if dirty, reset_dirty
void qmp_dpt_sync(Error **errp)
{
	/* Copy KVM's dirty bits to phys_dirty. */
	memory_global_sync_dirty_bitmap(get_system_memory());

	/* Dump phys_dirty to a new bitmap file. */
	time_t timestamp = time(NULL);
	dpt_bitmap_write(timestamp);

	/* Clear all migration dirty bits in phys_dirty. */
	make_all_pages_clean();
}
