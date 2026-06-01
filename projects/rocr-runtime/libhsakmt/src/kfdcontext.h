/*
 * Copyright © Advanced Micro Devices, Inc., or its affiliates. 
 * 
 * SPDX-License-Identifier: MIT
 */

#ifndef _KFDCONTEXT_H_
#define _KFDCONTEXT_H_

#include <stdint.h>
#include <stdbool.h>

struct hsa_kfd_topology_context;
struct hsa_kfd_queue_context;
struct hsa_kfd_fmm_context;
struct hsa_kfd_event_context;
struct hsa_kfd_debug_context;
struct hsa_kfd_perf_context;

/*
 * HsaKFDContext
 *
 * Represents the execution context for a connection to the Kernel Fusion Driver (KFD).
 *
 * This structure encapsulates all state required to manage a KFD session, including:
 *   - The file descriptor associated with the open KFD device
 *   - Related resources tied to this file descriptor
 *
 * Multiple HsaKFDContext instances can coexist simultaneously, each maintaining its own
 * independent set of resources. These contexts are fully isolated from one another and
 * must not have their resources mixed. For example, memory resources created in
 * context A cannot be used in context B directly. If resources need to be shared between
 * contexts, they must be explicitly exported and imported using the appropriate APIs.
 */
typedef struct _HsaKFDContext
{
    /* File descriptor for the KFD device */
    int fd;
    /*
     * Primary kfd context flag.
     * There is only one primary context per-process.
     */
    bool hsakmt_is_primary_ctx;

    /*
     * Indicates whether the SVM API is available for use
     * in this context. True only if HSA_USE_SVM is not
     * explicitly disabled (via env var) and all dGPUs in
     * the topology report SVM API capability.
     */
    bool hsakmt_is_svm_api_supported;

    /* Topology context for managing system topology information */
    struct hsa_kfd_topology_context *topology_context;

    /* Queue context for managing user queues */
    struct hsa_kfd_queue_context *queue_context;

    /* Memory management context for managing memory */
    struct hsa_kfd_fmm_context *fmm_context;

    /* Event context for managing events */
    struct hsa_kfd_event_context *event_context;

    /* Debug context for managing debug operations */
    struct hsa_kfd_debug_context *debug_context;

    /* perf context for managing perf operations */
    struct hsa_kfd_perf_context *perf_context;
} HsaKFDContext;

/* Initialize a pre-allocated HsaKFDContext with the given fd.
 * Returns 0 on success, -1 on allocation failure.
 */
int hsakmt_kfdcontext_init_context(int fd, HsaKFDContext *ctx);

/*
 * Free all sub-context allocations. Also resets fd to -1 to
 * mark the context as invalid.
 * Does NOT free the HsaKFDContext struct itself;
 * the caller retains ownership.
 */
void hsakmt_kfdcontext_clear_context(HsaKFDContext *ctx);

int hsakmt_kfdcontext_init_fmm_context(HsaKFDContext *ctx);
int hsakmt_kfdcontext_init_topology_context(HsaKFDContext *ctx);
int hsakmt_kfdcontext_init_queue_context(HsaKFDContext *ctx);
int hsakmt_kfdcontext_init_event_context(HsaKFDContext *ctx);
int hsakmt_kfdcontext_init_debug_context(HsaKFDContext *ctx);
int hsakmt_kfdcontext_init_perf_context(HsaKFDContext *ctx);
#endif /* _KFDCONTEXT_H_ */
