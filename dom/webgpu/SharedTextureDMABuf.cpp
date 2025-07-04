/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SharedTextureDMABuf.h"

#include "mozilla/gfx/Logging.h"
#include "mozilla/layers/ImageDataSerializer.h"
#include "mozilla/webgpu/WebGPUParent.h"
#include "mozilla/widget/DMABufSurface.h"
#include "mozilla/widget/DMABufDevice.h"
#include <gbm.h>

namespace mozilla::webgpu {

// static
UniquePtr<SharedTextureDMABuf> SharedTextureDMABuf::Create(
    WebGPUParent* aParent, const ffi::WGPUDeviceId aDeviceId,
    const uint32_t aWidth, const uint32_t aHeight,
    const struct ffi::WGPUTextureFormat aFormat,
    const ffi::WGPUTextureUsages aUsage) {
  if (aFormat.tag != ffi::WGPUTextureFormat_Bgra8Unorm) {
    gfxCriticalNoteOnce << "Non supported format: " << aFormat.tag;
    return nullptr;
  }

  auto* context = aParent->GetContext();
  uint64_t memorySize = 0;
  ffi::WGPUVkImageHandle* vkImage = wgpu_vkimage_create_with_dma_buf(
      context, aDeviceId, aWidth, aHeight, &memorySize);
  if (!vkImage) {
    gfxCriticalNoteOnce << "Failed to create VkImage";
    return nullptr;
  }
  UniquePtr<VkImageHandle> handle =
      MakeUnique<VkImageHandle>(aParent, aDeviceId, vkImage);

  const auto dmaBufInfo = wgpu_vkimage_get_dma_buf_info(vkImage);
  if (!dmaBufInfo.is_valid) {
    gfxCriticalNoteOnce << "Invalid DMABufInfo";
    return nullptr;
  }

  MOZ_ASSERT(dmaBufInfo.plane_count <= 3);

  if (dmaBufInfo.plane_count > 3) {
    gfxCriticalNoteOnce << "Invalid plane count";
    return nullptr;
  }

  auto rawFd = wgpu_vkimage_get_file_descriptor(context, aDeviceId, vkImage);
  if (rawFd < 0) {
    gfxCriticalNoteOnce << "Failed to get fd fom VkDeviceMemory";
    return nullptr;
  }

  RefPtr<gfx::FileHandleWrapper> fd =
      new gfx::FileHandleWrapper(UniqueFileHandle(rawFd));

  RefPtr<DMABufSurface> surface = DMABufSurfaceRGBA::CreateDMABufSurface(
      std::move(fd), dmaBufInfo, aWidth, aHeight);
  if (!surface) {
    MOZ_ASSERT_UNREACHABLE("unexpected to be called");
    return nullptr;
  }

  layers::SurfaceDescriptor desc;
  if (!surface->Serialize(desc)) {
    MOZ_ASSERT_UNREACHABLE("unexpected to be called");
    return nullptr;
  }

  const auto sdType = desc.type();
  if (sdType != layers::SurfaceDescriptor::TSurfaceDescriptorDMABuf) {
    MOZ_ASSERT_UNREACHABLE("unexpected to be called");
    return nullptr;
  }

  return MakeUnique<SharedTextureDMABuf>(
      aParent, aDeviceId, std::move(handle), aWidth, aHeight, aFormat, aUsage,
      std::move(surface), desc.get_SurfaceDescriptorDMABuf());
}

SharedTextureDMABuf::SharedTextureDMABuf(
    WebGPUParent* aParent, const ffi::WGPUDeviceId aDeviceId,
    UniquePtr<VkImageHandle>&& aVkImageHandle, const uint32_t aWidth,
    const uint32_t aHeight, const struct ffi::WGPUTextureFormat aFormat,
    const ffi::WGPUTextureUsages aUsage, RefPtr<DMABufSurface>&& aSurface,
    const layers::SurfaceDescriptorDMABuf& aSurfaceDescriptor)
    : SharedTexture(aWidth, aHeight, aFormat, aUsage),
      mParent(aParent),
      mDeviceId(aDeviceId),
      mVkImageHandle(std::move(aVkImageHandle)),
      mSurface(std::move(aSurface)),
      mSurfaceDescriptor(aSurfaceDescriptor) {}

SharedTextureDMABuf::~SharedTextureDMABuf() {}

void SharedTextureDMABuf::CleanForRecycling() {
  mSemaphoreFds.Clear();
  mVkSemaphoreHandles.Clear();
}

Maybe<layers::SurfaceDescriptor> SharedTextureDMABuf::ToSurfaceDescriptor() {
  layers::SurfaceDescriptor sd;
  if (!mSurface->Serialize(sd)) {
    return Nothing();
  }

  if (sd.type() != layers::SurfaceDescriptor::TSurfaceDescriptorDMABuf) {
    return Nothing();
  }

  auto& sdDMABuf = sd.get_SurfaceDescriptorDMABuf();
  sdDMABuf.semaphoreFd() = mSemaphoreFds.LastElement();

  return Some(sd);
}

void SharedTextureDMABuf::GetSnapshot(const ipc::Shmem& aDestShmem,
                                      const gfx::IntSize& aSize) {
  const RefPtr<gfx::SourceSurface> surface = mSurface->GetAsSourceSurface();
  if (!surface) {
    MOZ_ASSERT_UNREACHABLE("unexpected to be called");
    gfxCriticalNoteOnce << "Failed to get SourceSurface from DMABufSurface";
    return;
  }

  const RefPtr<gfx::DataSourceSurface> dataSurface = surface->GetDataSurface();
  if (!dataSurface) {
    MOZ_ASSERT_UNREACHABLE("unexpected to be called");
    return;
  }

  gfx::DataSourceSurface::ScopedMap map(dataSurface,
                                        gfx::DataSourceSurface::READ);
  if (!map.IsMapped()) {
    MOZ_ASSERT_UNREACHABLE("unexpected to be called");
    return;
  }

  const uint32_t stride = layers::ImageDataSerializer::ComputeRGBStride(
      gfx::SurfaceFormat::B8G8R8A8, aSize.width);
  uint8_t* src = static_cast<uint8_t*>(map.GetData());
  uint8_t* dst = aDestShmem.get<uint8_t>();

  MOZ_ASSERT(stride * aSize.height <= aDestShmem.Size<uint8_t>());
  MOZ_ASSERT(static_cast<uint32_t>(map.GetStride()) >= stride);

  for (int y = 0; y < aSize.height; y++) {
    memcpy(dst, src, stride);
    src += map.GetStride();
    dst += stride;
  }
}

UniqueFileHandle SharedTextureDMABuf::CloneDmaBufFd() {
  return mSurfaceDescriptor.fds()[0]->ClonePlatformHandle();
}

const ffi::WGPUVkImageHandle* SharedTextureDMABuf::GetHandle() {
  return mVkImageHandle->Get();
}

void SharedTextureDMABuf::onBeforeQueueSubmit(RawId aQueueId) {
  if (!mParent) {
    return;
  }

  auto* context = mParent->GetContext();
  if (!context) {
    return;
  }

  ffi::WGPUVkSemaphoreHandle* vkSemaphore =
      wgpu_vksemaphore_create_signal_semaphore(context, aQueueId);
  if (!vkSemaphore) {
    gfxCriticalNoteOnce << "Failed to create VkSemaphore";
    return;
  }

  auto rawFd =
      wgpu_vksemaphore_get_file_descriptor(context, mDeviceId, vkSemaphore);
  if (rawFd < 0) {
    gfxCriticalNoteOnce << "Failed to get fd from VkSemaphore";
    return;
  }

  mVkSemaphoreHandles.AppendElement(
      MakeUnique<VkSemaphoreHandle>(mParent, mDeviceId, vkSemaphore));
  mSemaphoreFds.AppendElement(
      new gfx::FileHandleWrapper(UniqueFileHandle(rawFd)));
}

}  // namespace mozilla::webgpu
