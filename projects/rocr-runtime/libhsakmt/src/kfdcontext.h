/*
 * Copyright © Advanced Micro Devices, Inc., or its affiliates. 
 * 
 * SPDX-License-Identifier: MIT
 */

#ifndef _KFDCONTEXT_H_
#define _KFDCONTEXT_H_

#include <stdint.h>

struct hsa_kfd_queue_context;
struct hsa_kfd_fmm_context;
struct hsa_kfd_event_context;


typedef struct _HsaKFDContext
{
    /* File descriptor for the KFD device */
    int fd;

    /* Queue context for managing user queues */
    struct hsa_kfd_queue_context *queue_context;

    /* Memory management context for managing memory */
    struct hsa_kfd_fmm_context *fmm_context;

    /* Event context for managing events */
    struct hsa_kfd_event_context *event_context;
} HsaKFDContext;

// Initialize a pre-allocated HsaKFDContext with the given file descriptor
void hsakmt_kfdcontext_init_context(int fd, HsaKFDContext *ctx);
// Release all resources associated with the given KFD context
void hsakmt_kfdcontext_clear_context(HsaKFDContext *ctx);

struct hsa_kfd_fmm_context *hsakmt_kfdcontext_get_fmm_context(HsaKFDContext *ctx);
struct hsa_kfd_queue_context *hsakmt_kfdcontext_get_queue_context(HsaKFDContext *ctx);
struct hsa_kfd_event_context *hsakmt_kfdcontext_get_event_context(HsaKFDContext *ctx);

#endif /* _KFDCONTEXT_H_ */
