#pragma once

#include <cstdint>

#define KIERO_VERSION "1.3.0"

#ifndef KIERO_INCLUDE_D3D9   
    #define KIERO_INCLUDE_D3D9   0 // 1 if you need D3D9 hook
#endif

#ifndef KIERO_INCLUDE_D3D10
    #define KIERO_INCLUDE_D3D10  0 // 1 if you need D3D10 hook
#endif

#ifndef KIERO_INCLUDE_D3D11
    #define KIERO_INCLUDE_D3D11  0 // 1 if you need D3D11 hook
#endif

#ifndef KIERO_INCLUDE_D3D12
    #define KIERO_INCLUDE_D3D12  0 // 1 if you need D3D12 hook
#endif

#ifndef KIERO_INCLUDE_OPENGL
    #define KIERO_INCLUDE_OPENGL 0 // 1 if you need OpenGL hook
#endif

#ifndef KIERO_INCLUDE_VULKAN
    #define KIERO_INCLUDE_VULKAN 0 // 1 if you need Vulkan hook
#endif

#ifndef KIERO_USE_MINHOOK
    #define KIERO_USE_MINHOOK    0 // 1 if you will use kiero::bind function
#endif

namespace kiero
{
	enum class Status
	{
		UnknownError = -1,
		NotSupportedError = -2,
		ModuleNotFoundError = -3,

		AlreadyInitializedError = -4,
		NotInitializedError = -5,

		Success = 0,
	};

	enum class RenderType
	{
		None,

		D3D9,
		D3D10,
		D3D11,
		D3D12,

		OpenGL,
		Vulkan,

		Auto
	};

	Status init(const RenderType renderType);
	void shutdown();

	Status bind(const ::std::uint16_t index, void** const original, void* const function);
	void unbind(const ::std::uint16_t index);

	[[nodiscard]] RenderType getRenderType() noexcept;
	[[nodiscard]] void** getMethodsTable() noexcept;
}
