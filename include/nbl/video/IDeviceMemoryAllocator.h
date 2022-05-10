#ifndef _NBL_VIDEO_I_DEVICE_MEMORY_ALLOCATOR_H_INCLUDED_
#define _NBL_VIDEO_I_DEVICE_MEMORY_ALLOCATOR_H_INCLUDED_

#include "IDriverMemoryAllocation.h"
#include "IDriverMemoryBacked.h"

namespace nbl::video
{

class IDeviceMemoryAllocator
{
public:
	static constexpr size_t InvalidMemoryOffset = 0xdeadbeefBadC0ffeull;

	struct SMemoryOffset
	{
		core::smart_refctd_ptr<IDriverMemoryAllocation> memory = nullptr;
		size_t offset = InvalidMemoryOffset;
	};

	struct SAllocateInfo
	{
		size_t size : 54 = 0ull;
		size_t flags : 5 = 0u; // IDriverMemoryAllocation::E_MEMORY_ALLOCATE_FLAGS
		size_t memoryTypeIndex : 5 = 0u;
		IDriverMemoryBacked* dedication = nullptr; // if you make the info have a `dedication` the memory will be bound right away, also it will use VK_KHR_dedicated_allocation on vulkan
		// size_t opaqueCaptureAddress = 0u; Note that this mechanism is intended only to support capture/replay tools, and is not recommended for use in other applications.
	};

	virtual SMemoryOffset allocate(const SAllocateInfo& info) = 0;
};

}

#endif