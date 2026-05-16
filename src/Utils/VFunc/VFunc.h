#pragma once

#include <cstddef>
#include <cstdint>

inline void **&GetVTable(void* inst, size_t offset = 0)
{
	return *reinterpret_cast<void***>(reinterpret_cast<uintptr_t>(inst) + offset);
}

inline const void **GetVTable(const void* inst, size_t offset = 0)
{
	return *reinterpret_cast<const void***>(reinterpret_cast<uintptr_t>(inst) + offset);
}

template<typename T>
inline T GetVFunc(const void* inst, size_t index, size_t offset = 0)
{
	if (!inst)
		return T{};

	const auto vTable = *reinterpret_cast<const void***>(reinterpret_cast<uintptr_t>(inst) + offset);
	if (!vTable)
		return T{};

	return reinterpret_cast<T>(vTable[index]);
}
