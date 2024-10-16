/*
 * Copyright (C) 2018-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/helpers/compiler_product_helper.h"

#include "opencl/source/command_queue/command_queue_hw.h"
#include "opencl/source/helpers/mipmap.h"
#include "opencl/source/mem_obj/buffer.h"
#include "opencl/source/mem_obj/image.h"
#include "opencl/source/memory_manager/mem_obj_surface.h"

namespace NEO {

template <typename GfxFamily>
cl_int CommandQueueHw<GfxFamily>::enqueueCopyBufferToImage(
    Buffer *srcBuffer,
    Image *dstImage,
    size_t srcOffset,
    const size_t *dstOrigin,
    const size_t *region,
    cl_uint numEventsInWaitList,
    const cl_event *eventWaitList,
    cl_event *event) {

    bool isStateless = device->getCompilerProductHelper().isForceToStatelessRequired();
    if (srcBuffer->getSize() >= 4ull * MemoryConstants::gigaByte) {
        isStateless = true;
    }
    const bool useHeapless = false;
    auto builtInType = EBuiltInOps::adjustBuiltinType<EBuiltInOps::copyBufferToImage3d>(isStateless, useHeapless);

    auto &builder = BuiltInDispatchBuilderOp::getBuiltinDispatchInfoBuilder(builtInType,
                                                                            this->getClDevice());
    BuiltInOwnershipWrapper builtInLock(builder, this->context);

    MemObjSurface srcBufferSurf(srcBuffer);
    MemObjSurface dstImgSurf(dstImage);
    Surface *surfaces[] = {&srcBufferSurf, &dstImgSurf};

    BuiltinOpParams dc;
    dc.srcMemObj = srcBuffer;
    dc.dstMemObj = dstImage;
    dc.srcOffset = {srcOffset, 0, 0};
    dc.dstOffset = dstOrigin;
    dc.size = region;
    if (isMipMapped(dstImage->getImageDesc())) {
        dc.dstMipLevel = findMipLevel(dstImage->getImageDesc().image_type, dstOrigin);
    }

    MultiDispatchInfo dispatchInfo(dc);
    builder.buildDispatchInfos(dispatchInfo);

    return enqueueHandler<CL_COMMAND_COPY_BUFFER_TO_IMAGE>(
        surfaces,
        false,
        dispatchInfo,
        numEventsInWaitList,
        eventWaitList,
        event);
}

} // namespace NEO
