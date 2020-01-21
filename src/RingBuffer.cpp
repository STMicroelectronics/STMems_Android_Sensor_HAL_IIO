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
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <cutils/native_handle.h>
#include <cutils/ashmem.h>
#if CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_OREO_VERSION
#include <log/log.h>
#else
#include <cutils/log.h>
#endif /* use log/log.h start from android 8 major version */
#include <sys/mman.h>
#include <memory>
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
        ALOGE("Sensor: Invalid shared memory size (shuold be %zu)", mem->size);
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

ANDROID_SINGLETON_STATIC_INSTANCE(GrallocHalWrapper);

GrallocHalWrapper::GrallocHalWrapper()
        : mError(android::NO_INIT), mVersion(-1),
          mGrallocModule(nullptr), mAllocDevice(nullptr), mGralloc1Device(nullptr),
          mPfnRetain(nullptr), mPfnRelease(nullptr), mPfnLock(nullptr), mPfnUnlock(nullptr) {
    const hw_module_t *module;
    android::status_t err = ::hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    ALOGE_IF(err, "couldn't load %s module (%s)", GRALLOC_HARDWARE_MODULE_ID, strerror(-err));

    if (module == nullptr) {
        mError = (err < 0) ? err : android::NO_INIT;
    }

    switch ((module->module_api_version >> 8) & 0xFF) {
        case 0:
            err = ::gralloc_open(module, &mAllocDevice);
            if (err != android::NO_ERROR) {
                ALOGE("cannot open alloc device (%s)", strerror(-err));
                break;
            }

            if (mAllocDevice == nullptr) {
                ALOGE("gralloc_open returns no error, but result is nullptr");
                err = android::INVALID_OPERATION;
                break;
            }

            // successfully initialized gralloc
            mGrallocModule = (gralloc_module_t *)module;
            mVersion = 0;
            break;
        case 1:
            err = ::gralloc1_open(module, &mGralloc1Device);
            if (err != android::NO_ERROR) {
                ALOGE("cannot open gralloc1 device (%s)", strerror(-err));
                break;
            }

            if (mGralloc1Device == nullptr || mGralloc1Device->getFunction == nullptr) {
                ALOGE("gralloc1_open returns no error, but result is nullptr");
                err = android::INVALID_OPERATION;
                break;
            }

            mPfnRetain = (GRALLOC1_PFN_RETAIN)(mGralloc1Device->getFunction(mGralloc1Device,
                                                      GRALLOC1_FUNCTION_RETAIN));
            mPfnRelease = (GRALLOC1_PFN_RELEASE)(mGralloc1Device->getFunction(mGralloc1Device,
                                                       GRALLOC1_FUNCTION_RELEASE));
            mPfnLock = (GRALLOC1_PFN_LOCK)(mGralloc1Device->getFunction(mGralloc1Device,
                                                    GRALLOC1_FUNCTION_LOCK));
            mPfnUnlock = (GRALLOC1_PFN_UNLOCK)(mGralloc1Device->getFunction(mGralloc1Device,
                                                      GRALLOC1_FUNCTION_UNLOCK));
            if (mPfnRetain == nullptr || mPfnRelease == nullptr
                    || mPfnLock == nullptr || mPfnUnlock == nullptr) {
                ALOGE("Function pointer for retain, release, lock and unlock are %p, %p, %p, %p",
                      mPfnRetain, mPfnRelease, mPfnLock, mPfnUnlock);
                err = android::BAD_VALUE;
                break;
            }

            // successfully initialized gralloc1
            mGrallocModule = (gralloc_module_t *)module;
            mVersion = 1;
            break;
        default:
            ALOGE("Unknown version, not supported");
            break;
    }
    mError = err;
}

GrallocHalWrapper::~GrallocHalWrapper() {
    if (mAllocDevice != nullptr) {
        ::gralloc_close(mAllocDevice);
    }
}

int GrallocHalWrapper::registerBuffer(const native_handle_t *handle) {
    switch (mVersion) {
        case 0:
            return mGrallocModule->registerBuffer(mGrallocModule, handle);
        case 1:
            return mapGralloc1Error(mPfnRetain(mGralloc1Device, handle));
        default:
            return android::NO_INIT;
    }
}

int GrallocHalWrapper::unregisterBuffer(const native_handle_t *handle) {
    switch (mVersion) {
        case 0:
            return mGrallocModule->unregisterBuffer(mGrallocModule, handle);
        case 1:
            return mapGralloc1Error(mPfnRelease(mGralloc1Device, handle));
        default:
            return android::NO_INIT;
    }
}

int GrallocHalWrapper::lock(const native_handle_t *handle,
                           int usage, int l, int t, int w, int h, void **vaddr) {
    switch (mVersion) {
        case 0:
            return mGrallocModule->lock(mGrallocModule, handle, usage, l, t, w, h, vaddr);
        case 1: {
            const gralloc1_rect_t rect = {
                .left = l,
                .top = t,
                .width = w,
                .height = h
            };
            return mapGralloc1Error(mPfnLock(mGralloc1Device, handle,
                                             GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN,
                                             GRALLOC1_CONSUMER_USAGE_NONE,
                                             &rect, vaddr, -1));
        }
        default:
            return android::NO_INIT;
    }
}

int GrallocHalWrapper::unlock(const native_handle_t *handle) {
    switch (mVersion) {
        case 0:
            return mGrallocModule->unlock(mGrallocModule, handle);
        case 1: {
            int32_t dummy;
            return mapGralloc1Error(mPfnUnlock(mGralloc1Device, handle, &dummy));
        }
        default:
            return android::NO_INIT;
    }
}

int GrallocHalWrapper::mapGralloc1Error(int grallocError) {
    switch (grallocError) {
        case GRALLOC1_ERROR_NONE:
            return android::NO_ERROR;
        case GRALLOC1_ERROR_BAD_DESCRIPTOR:
        case GRALLOC1_ERROR_BAD_HANDLE:
        case GRALLOC1_ERROR_BAD_VALUE:
            return android::BAD_VALUE;
        case GRALLOC1_ERROR_NOT_SHARED:
        case GRALLOC1_ERROR_NO_RESOURCES:
            return android::NO_MEMORY;
        case GRALLOC1_ERROR_UNDEFINED:
        case GRALLOC1_ERROR_UNSUPPORTED:
            return android::INVALID_OPERATION;
        default:
            return android::UNKNOWN_ERROR;
    }
}

GrallocDirectChannel::GrallocDirectChannel(const struct sensors_direct_mem_t *mem)
        : mNativeHandle(nullptr) {
    if (mem->handle == nullptr) {
        ALOGE("mem->handle == nullptr");
        mError = android::BAD_VALUE;
        return;
    }

    mNativeHandle = ::native_handle_clone(mem->handle);
    if (mNativeHandle == nullptr) {
        ALOGE("clone mem->handle failed...");
        mError = android::NO_MEMORY;
        return;
    }

    mError = GrallocHalWrapper::getInstance().registerBuffer(mNativeHandle);
    if (mError != android::NO_ERROR) {
        ALOGE("registerBuffer failed");
        return;
    }

    mError = GrallocHalWrapper::getInstance().lock(mNativeHandle,
            GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0, mem->size, 1, &mBase);
    if (mError != android::NO_ERROR) {
        ALOGE("lock buffer failed");
        return;
    }

    if (mBase == nullptr) {
        ALOGE("lock buffer => nullptr");
        mError = android::NO_MEMORY;
        return;
    }

    mSize = mem->size;
    mBuffer = new RingBuffer(mBase, mSize);
    if (!mBuffer) {
        mError = android::NO_MEMORY;
        return;
    }

    mError = android::NO_ERROR;
}

GrallocDirectChannel::~GrallocDirectChannel() {
    if (mNativeHandle != nullptr) {
        if (mBase) {
            mBuffer = nullptr;
            GrallocHalWrapper::getInstance().unlock(mNativeHandle);
            mBase = nullptr;
        }
        GrallocHalWrapper::getInstance().unregisterBuffer(mNativeHandle);
        ::native_handle_close(mNativeHandle);
        ::native_handle_delete(mNativeHandle);
        mNativeHandle = nullptr;
    }
}
