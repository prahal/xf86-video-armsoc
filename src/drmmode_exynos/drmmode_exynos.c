/*
 * Copyright © 2013 ARM Limited.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "../drmmode_driver.h"
#include <stddef.h>
#include <xf86drmMode.h>
#include <xf86drm.h>
#include <sys/ioctl.h>

enum e_drm_exynos_gem_mem_type {
	/* Physically Continuous memory and used as default. */
	EXYNOS_BO_CONTIG	= 0 << 0,
	/* Physically Non-Continuous memory. */
	EXYNOS_BO_NONCONTIG	= 1 << 0,
	/* non-cachable mapping and used as default. */
	EXYNOS_BO_NONCACHABLE	= 0 << 1,
	/* cachable mapping. */
	EXYNOS_BO_CACHABLE	= 1 << 1,
	/* write-combine mapping. */
	EXYNOS_BO_WC		= 1 << 2,
	EXYNOS_BO_MASK		= EXYNOS_BO_NONCONTIG | EXYNOS_BO_CACHABLE |
					EXYNOS_BO_WC
};

struct drm_exynos_gem_create {
	uint64_t size;
	unsigned int flags;
	unsigned int handle;
};

#define DRM_EXYNOS_GEM_CREATE 0x00
#define DRM_IOCTL_EXYNOS_GEM_CREATE DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_CREATE, struct drm_exynos_gem_create)

/* Cursor dimensions
 * Technically we probably don't have any size limit.. since we
 * are just using an overlay... but xserver will always create
 * cursor images in the max size, so don't use width/height values
 * that are too big
 */
#define CURSORW  (64)
#define CURSORH  (64)

/*
 * Padding added down each side of cursor image. This is a workaround for a bug
 * causing corruption when the cursor reaches the screen edges.
 */
#define CURSORPAD (16)

#define ALIGN(val, align)	(((val) + (align) - 1) & ~((align) - 1))

static int create_custom_gem(int fd, struct armsoc_create_gem *create_gem)
{
	struct drm_exynos_gem_create create_exynos;
	int ret;
	unsigned int pitch;

	/* make pitch a multiple of 64 bytes for best performance */
	pitch = ALIGN(create_gem->width * ((create_gem->bpp + 7) / 8), 64);
	memset(&create_exynos, 0, sizeof(create_exynos));
	create_exynos.size = create_gem->height * pitch;

	assert((create_gem->buf_type == ARMSOC_BO_SCANOUT) ||
			(create_gem->buf_type == ARMSOC_BO_NON_SCANOUT));

	// TODO: use exynos_bo_create here

	/* Contiguous allocations are not supported in some exynos drm versions.
	 * When they are supported all allocations are effectively contiguous
	 * anyway, so for simplicity we always request non contiguous buffers.
	 */
	create_exynos.flags = EXYNOS_BO_NONCONTIG | EXYNOS_BO_WC;

	ret = drmIoctl(fd, DRM_IOCTL_EXYNOS_GEM_CREATE, &create_exynos);
	if (ret)
		return ret;

	/* Convert custom create_exynos to generic create_gem */
	create_gem->handle = create_exynos.handle;
	create_gem->pitch = pitch;
	create_gem->size = create_exynos.size;

	return 0;
}

struct drmmode_interface exynos_interface = {
	1                     /* use_page_flip_events */,
	1                     /* use_early_display */,
	CURSORW               /* cursor width */,
	CURSORH               /* cursor_height */,
	CURSORPAD             /* cursor padding */,
	HWCURSOR_API_PLANE    /* cursor_api */,
	0                     /* vblank_query_supported */,
	create_custom_gem     /* create_custom_gem */,
};

struct drmmode_interface *drmmode_interface_get_implementation(int drm_fd)
{
	return &exynos_interface;
}
