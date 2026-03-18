/*
 * Copyright © 2025 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including
 * the next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "hsakmt/hsakmtmodel.h"
#include "libhsakmt.h"
#include "hsakmt/hsakmttypes.h"
#include "hsakmt/hsakmtmodeliface.h"
#define _GNU_SOURCE
#define __USE_GNU
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <fcntl.h>

bool hsakmt_use_model;
char *hsakmt_model_topology;

static pthread_mutex_t model_ioctl_mutex = PTHREAD_MUTEX_INITIALIZER;
static void *model_library;
static const struct hsakmt_model_functions *model_functions;

HSAKMT_STATUS HSAKMTAPI hsaKmtModelEnabled(bool* enable)
{
	*enable = hsakmt_use_model;
	return HSAKMT_STATUS_SUCCESS;
}

void model_init_env_vars(void)
{
	/* Check whether to use a model instead of real hardware */
	hsakmt_model_topology = getenv("HSA_MODEL_TOPOLOGY");
	if (hsakmt_model_topology)
		hsakmt_use_model = true;
	if (hsakmt_use_model)
	{
		/* Load model library first to get interface functions */
		const char *libname = getenv("HSA_MODEL_LIB");
		if (!libname)
		{
			fprintf(stderr, "model: HSA_MODEL_LIB environment variable must be set to FFM .so\n");
			abort();
		}
		// model_library = dlmopen(LM_ID_NEWLM, libname, RTLD_NOW);
		model_library = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
		if (!model_library)
		{
			fprintf(stderr, "model: failed to load %s: %s\n", libname, dlerror());
			abort();
		}
		get_hsakmt_model_functions_t getter = dlsym(model_library, "get_hsakmt_model_functions");
		if (!getter)
		{
			fprintf(stderr, "model: Failed to get hsakmt_model_functions\n");
			abort();
		}
		model_functions = getter();
		const uint32_t expected_version_major = (uint32_t)HSAKMT_MODEL_INTERFACE_VERSION_MAJOR;
		const uint32_t expected_version_minor = (uint32_t)HSAKMT_MODEL_INTERFACE_VERSION_MINOR;

		if (model_functions->version_major != expected_version_major ||
			model_functions->version_minor < expected_version_minor)
		{
			fprintf(stderr, "[MODEL] FATAL: Version mismatch!\n");
			fprintf(stderr, "[MODEL]   Model file: %s\n", libname);
			fprintf(stderr, "[MODEL]   Model version: %u.%u\n", model_functions->version_major, model_functions->version_minor);
			fprintf(stderr, "[MODEL]   Expected version: %u.%u or higher\n", expected_version_major, expected_version_minor);
			if (model_functions->version_major != expected_version_major) {
				fprintf(stderr, "[MODEL]   MAJOR version mismatch (breaking API change)\n");
			} else {
				fprintf(stderr, "[MODEL]   Minor version too old (missing required features)\n");
			}
			abort();
		}

		/* Let FFM create the memfd - it owns sizing and lifecycle.
		 *
		 * As of interface v1.0 this is mandatory for correct model operation.
		 */
		if (!model_functions->create_memfd) {
			fprintf(stderr, "[MODEL] FATAL: Model library does not provide create_memfd (required for v%u.%u)\n",
					HSAKMT_MODEL_INTERFACE_VERSION_MAJOR,
					HSAKMT_MODEL_INTERFACE_VERSION_MINOR);
			abort();
		}

		int fd = model_functions->create_memfd();
		if (fd < 0) {
			fprintf(stderr, "model: FFM failed to create memfd\n");
			abort();
		}

		assert(hsakmt_primary_kfd_ctx.fd < 0);
		hsakmt_kfdcontext_init_context(fd, &hsakmt_primary_kfd_ctx);
	}
}

void model_init(void)
{
	// Don't need to do anything here. This can probably be removed.
}

/* Model implementation of KFD ioctl. */

static int model_kfd_ioctl_dispatch(unsigned long request, void *arg)
{
	assert(_IOC_TYPE(request) == AMDKFD_IOCTL_BASE);

	/* Delegate all KFD IOCTL handling to the model backend, including
	 * AMDKFD_IOC_SVM requests with variable encoded sizes. */
	return model_functions->handle_ioctl(request, arg);
}

static bool is_wait_events_ioctl(unsigned long request)
{
	return (_IOC_TYPE(request) == AMDKFD_IOCTL_BASE) &&
	       (_IOC_NR(request) == _IOC_NR(AMDKFD_IOC_WAIT_EVENTS));
}

int model_kfd_ioctl(unsigned long request, void *arg)
{
	/* WAIT_EVENTS can block for long periods. Holding the global model IOCTL
	 * mutex across a blocking wait prevents other threads from issuing IOCTLs
	 * like SET_EVENT that are required to wake the wait, which can deadlock
	 * user-space event tests under the model.
	 *
	 * Keep the conservative serialization for all other IOCTLs.
	 */
	if (is_wait_events_ioctl(request))
		return model_kfd_ioctl_dispatch(request, arg);

	/* Use a very simple locking strategy for correctness. IOCTLs should
	 * be rare anyway and not contended considering the cost of running
	 * the model itself.
	 *
	 * The bulk of model execution happens in a separate thread *without*
	 * holding the IOCTL mutex. */
	pthread_mutex_lock(&model_ioctl_mutex);
	int ret = model_kfd_ioctl_dispatch(request, arg);
	pthread_mutex_unlock(&model_ioctl_mutex);
	return ret;
}
