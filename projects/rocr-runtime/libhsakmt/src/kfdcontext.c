/*
 * Copyright © Advanced Micro Devices, Inc., or its affiliates. 
 * 
 * SPDX-License-Identifier: MIT
 */

#include "kfdcontext.h"
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

void hsakmt_kfdcontext_init_context(int fd, HsaKFDContext *ctx)
{
    assert(fd >= 0);
    assert(ctx);

    ctx->fd = fd;
    ctx->topology_context = NULL;
    ctx->queue_context = NULL;
    ctx->fmm_context = NULL;
    ctx->event_context = NULL;
    ctx->debug_context = NULL;
    ctx->perf_context = NULL;
}

void hsakmt_kfdcontext_clear_context(HsaKFDContext *ctx)
{
    if (!ctx)
        return;

    if (ctx->topology_context) {
        free(ctx->topology_context);
        ctx->topology_context = NULL;
    }
    if (ctx->queue_context) {
        free(ctx->queue_context);
        ctx->queue_context = NULL;
    }
    if (ctx->fmm_context) {
        free(ctx->fmm_context);
        ctx->fmm_context = NULL;
    }
    if (ctx->event_context) {
        free(ctx->event_context);
        ctx->event_context = NULL;
    }
    if (ctx->debug_context) {
        free(ctx->debug_context);
        ctx->debug_context = NULL;
    }
    if (ctx->perf_context) {
        free(ctx->perf_context);
        ctx->perf_context = NULL;
    }
    ctx->fd = -1;
}
