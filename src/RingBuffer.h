/*
 * STMicroelectronics simple Ring Buffer for Direct Report Channel
 *
 * Copyright 2017 STMicroelectronics Inc.
 * Author:
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#ifndef S_RING_BUFFER_H_
#define S_RING_BUFFER_H_

#include <media/stagefright/foundation/ABase.h>
#include <hardware/sensors.h>
#include <utils/threads.h>
#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>
#include <utils/Singleton.h>

struct RingBuffer {
	RingBuffer(void* buf, size_t size);
	~RingBuffer();

	void write(const sensors_event_t *ev, size_t size);

private:
	sensors_event_t *mData;
	size_t mSize;
	size_t mWritePos;
	int32_t mCounter;

	DISALLOW_EVIL_CONSTRUCTORS(RingBuffer);
};

class DirectChannelBase {
public:
    DirectChannelBase() : mError(android::NO_INIT), mSize(0), mBase(nullptr) { }
    virtual ~DirectChannelBase() {}

    bool isValid();
    int getError();
    void write(const sensors_event_t * ev);

protected:
    int mError;
    RingBuffer *mBuffer;

    /* Internal size of ring buffer. */
    size_t mSize;

    /* Ring buffer base memory pointer. */
    void *mBase;
};

class AshmemDirectChannel : public DirectChannelBase {
public:
    AshmemDirectChannel(const struct sensors_direct_mem_t *mem);
    virtual ~AshmemDirectChannel();

private:
    int mAshmemFd;
};

class GrallocHalWrapper : public android::Singleton<GrallocHalWrapper> {
public:
    int registerBuffer(const native_handle_t *handle);
    int unregisterBuffer(const native_handle_t *handle);
    int lock(const native_handle_t *handle, int usage, int l, int t, int w, int h, void **vaddr);
    int unlock(const native_handle_t *handle);
private:
    friend class android::Singleton<GrallocHalWrapper>;
    GrallocHalWrapper();
    ~GrallocHalWrapper();
    static int mapGralloc1Error(int grallocError);

    int mError;
    int mVersion;
    gralloc_module_t *mGrallocModule;
    // gralloc
    alloc_device_t *mAllocDevice;

    // gralloc1
    gralloc1_device_t *mGralloc1Device;
    GRALLOC1_PFN_RETAIN mPfnRetain;
    GRALLOC1_PFN_RELEASE mPfnRelease;
    GRALLOC1_PFN_LOCK mPfnLock;
    GRALLOC1_PFN_UNLOCK mPfnUnlock;
};

class GrallocDirectChannel : public DirectChannelBase {
public:
    GrallocDirectChannel(const struct sensors_direct_mem_t *mem);
    virtual ~GrallocDirectChannel();
private:
    native_handle_t *mNativeHandle;
};

#endif  // S_RING_BUFFER_H_
