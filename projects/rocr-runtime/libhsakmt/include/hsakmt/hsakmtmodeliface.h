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

#ifndef _HSAKMTMODELIFACE_H_
#define _HSAKMTMODELIFACE_H_

#include <inttypes.h>
#include <stdbool.h>

// Changelog:
//  1.0: Breaking ABI cleanup: remove unused entry points (create/destroy/
//       set_global_aperture/init_from_topology). Model interface is now
//       create_memfd + handle_ioctl.
#define HSAKMT_MODEL_INTERFACE_VERSION_MAJOR 1
#define HSAKMT_MODEL_INTERFACE_VERSION_MINOR 0

typedef struct hsakmt_model hsakmt_model_t;
typedef struct hsakmt_model_queue hsakmt_model_queue_t;

// Pointer to a "set event" function.
//
// data is a user-provided opaque pointer.
// event_id is the ID of the event to set (as in amd_signal_s::event_id).
typedef void (*hsakmt_model_set_event_fn)(void *data, unsigned event_id);

// Interface provided by the software model implementation.
//
// Queried from a shared library by calling an export called
// `get_hsakmt_model_functions`
//
// Interface versioning follows the semantic versioning model: clients that
// know about interface version X.Y can use any implementation that provides
// version X.Z with Z >= Y.
//
// The model is designed to support only one VMID space.
struct hsakmt_model_functions {
	uint32_t version_major; // HSAKMT_MODEL_INTERFACE_VERSION_MAJOR
	uint32_t version_minor; // HSAKMT_MODEL_INTERFACE_VERSION_MINOR

	// Create memfd for model use (v0.7+)
	// FFM owns memfd creation and sizing logic
	// Returns: File descriptor on success, -1 on error (errno set)
	int (*create_memfd)(void);

	// Unified IOCTL handler - FFM owns all IOCTL dispatch logic
	// Returns 0 on success, -1 on error (with errno set)
	int (*handle_ioctl)(unsigned long request, void *arg);
};

// Type of a shared library export called `get_hsakmt_model_functions`.
typedef const struct hsakmt_model_functions *(*get_hsakmt_model_functions_t)(void);

#endif // _HSAKMTMODELIFACE_H_