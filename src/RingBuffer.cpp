/*
 * STMicroelectronics simple Ring Buffer for Direct Report Channel
 *
 * Copyright 2017 STMicroelectronics Inc.
 * Author:
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <stdlib.h>
#include <string.h>

#include <cutils/ashmem.h>
#if ST_HAL_ANDROID_VERSION >= ST_HAL_OREO_VERSION
#include <log/log.h>
#else
#include <cutils/log.h>
#endif /* use log/log.h start from android 8 major version */
#include <sys/mman.h>

#include "RingBuffer.h"

bool DirectChannelBase::isValid()
{
    return mBuffer != nullptr;
}

int DirectChannelBase::getError()
{
    return mError;
}

/* Write one event into Ring Buffer. */
void DirectChannelBase::write(const sensors_event_t * ev)
{
    if (isValid()) {
        mBuffer->write(ev, 1);
    }
}

RingBuffer::RingBuffer(void* buf, size_t size) : mData((sensors_event_t *)buf),
                mSize(size/sizeof(sensors_event_t)), mWritePos(0), mCounter(1)
{
    memset(mData, 0, size);
}

RingBuffer::~RingBuffer()
{
    memset(mData, 0, mSize*sizeof(sensors_event_t));
}

void RingBuffer::write(const sensors_event_t *ev, size_t size)
{
    if (!mSize)
        return;

    while(size--) {
        mData[mWritePos] = *(ev++);
        mData[mWritePos].reserved0 = mCounter++;

        /* Override old samples. */
        if (++mWritePos >= mSize)
            mWritePos = 0;
    }
}

AshmemDirectChannel::AshmemDirectChannel(const struct sensors_direct_mem_t *mem) : mAshmemFd(0)
{
    if (mem == nullptr) {
        ALOGE("Sensor: Invalid memory area");
        mError = android::BAD_VALUE;
        return;
    }

    mAshmemFd = mem->handle->data[0];

    if (!::ashmem_valid(mAshmemFd)) {
        ALOGE("Sensor: Invalid shared memory file descriptor");
        mError = android::BAD_VALUE;
        return;
    }

    /* Check if shmem region has correct size. */
    if ((size_t)::ashmem_get_size_region(mAshmemFd) != mem->size) {
        ALOGE("Sensor: Invalid shared memory size (shuold be %d)", mem->size);
        mError = android::BAD_VALUE;
        return;
    }

    mSize = mem->size;

    mBase = ::mmap(NULL, mem->size, PROT_WRITE, MAP_SHARED, mAshmemFd, 0);
    if (mBase == nullptr) {
        ALOGE("Sensor: Unable to map shared memory");
        mError = android::NO_MEMORY;
        return;
    }

    mBuffer = new RingBuffer(mBase, mSize);
    if (!mBuffer) {
        ALOGE("Sensor: Unable to allocate mBuffer");
        mError = android::NO_MEMORY;
    }
}

AshmemDirectChannel::~AshmemDirectChannel()
{
    if (mBase) {
        mBuffer = nullptr;
        ::munmap(mBase, mSize);
        mBase = nullptr;
    }
    ::close(mAshmemFd);
}
