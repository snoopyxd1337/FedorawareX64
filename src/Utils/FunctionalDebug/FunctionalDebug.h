#pragma once

#include "../Minidump/Minidump.h"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <format>
#include <mutex>
#include <string>

#if defined(FW_DEBUG_DIAGNOSTICS) || defined(FW_FUNCTIONAL_DEBUG)
namespace FunctionalDebug
{
	inline constexpr size_t MAX_ENTRIES = 256;
	inline constexpr uint64_t DEFAULT_SLOW_US = 2000;
	inline constexpr uint64_t HOOK_SLOW_US = 5000;
	inline constexpr int FLUSH_MS = 1000;

	struct Entry
	{
		const char* name = nullptr;
		uint64_t slowThresholdUs = DEFAULT_SLOW_US;
		std::atomic<uint64_t> calls = 0;
		std::atomic<uint64_t> skipped = 0;
		std::atomic<uint64_t> errors = 0;
		std::atomic<uint64_t> totalUs = 0;
		std::atomic<uint64_t> maxUs = 0;
		std::atomic<uint64_t> slowCalls = 0;
		char lastReason[160] = {};
	};

	inline std::array<Entry, MAX_ENTRIES> g_Entries = {};
	inline std::mutex g_RegisterMutex;
	inline std::atomic<int64_t> g_LastFlushMs = 0;

	inline int64_t NowMs()
	{
		return GetTickCount64();
	}

	inline uint64_t NowUs()
	{
		static LARGE_INTEGER frequency = [] {
			LARGE_INTEGER value = {};
			QueryPerformanceFrequency(&value);
			return value;
		}();

		LARGE_INTEGER now = {};
		QueryPerformanceCounter(&now);
		return static_cast<uint64_t>((now.QuadPart * 1000000) / frequency.QuadPart);
	}

	inline void SetReason(Entry* entry, const char* reason)
	{
		if (!entry || !reason || !*reason)
			return;

		strncpy_s(entry->lastReason, reason, _TRUNCATE);
	}

	inline void SetReason(Entry* entry, const std::string& reason)
	{
		SetReason(entry, reason.c_str());
	}

	inline Entry* Register(const char* name, uint64_t slowThresholdUs = DEFAULT_SLOW_US)
	{
		if (!name || !*name)
			name = "<unnamed>";

		for (auto& entry : g_Entries)
		{
			if (entry.name && strcmp(entry.name, name) == 0)
				return &entry;
		}

		std::lock_guard lock(g_RegisterMutex);
		for (auto& entry : g_Entries)
		{
			if (entry.name && strcmp(entry.name, name) == 0)
				return &entry;

			if (!entry.name)
			{
				entry.name = name;
				entry.slowThresholdUs = slowThresholdUs;
				FW_TRACE(std::format("[FUNC] registered name={} slow_us={}", entry.name, entry.slowThresholdUs).c_str());
				return &entry;
			}
		}

		FW_TRACE(std::format("[ERROR] Functional debug registry full | name={} reason=max-entries-reached", name).c_str());
		return nullptr;
	}

	inline void UpdateMax(std::atomic<uint64_t>& target, uint64_t value)
	{
		uint64_t current = target.load(std::memory_order_relaxed);
		while (value > current && !target.compare_exchange_weak(current, value, std::memory_order_relaxed))
		{
		}
	}

	inline void Record(Entry* entry, uint64_t elapsedUs)
	{
		if (!entry)
			return;

		entry->calls.fetch_add(1, std::memory_order_relaxed);
		entry->totalUs.fetch_add(elapsedUs, std::memory_order_relaxed);
		UpdateMax(entry->maxUs, elapsedUs);

		if (elapsedUs >= entry->slowThresholdUs)
		{
			entry->slowCalls.fetch_add(1, std::memory_order_relaxed);
			SetReason(entry, std::format("slow-call {}us >= {}us", elapsedUs, entry->slowThresholdUs));
		}
	}

	inline void Record(const char* name, uint64_t elapsedUs, uint64_t slowThresholdUs = DEFAULT_SLOW_US)
	{
		Record(Register(name, slowThresholdUs), elapsedUs);
	}

	inline void Skip(Entry* entry, const char* reason)
	{
		if (!entry)
			return;

		entry->skipped.fetch_add(1, std::memory_order_relaxed);
		SetReason(entry, reason);
	}

	inline void Skip(const char* name, const char* reason)
	{
		Skip(Register(name), reason);
	}

	inline void Skip(const char* name, const std::string& reason)
	{
		Skip(name, reason.c_str());
	}

	inline void Skip(Entry* entry, const std::string& reason)
	{
		Skip(entry, reason.c_str());
	}

	inline void Error(Entry* entry, const char* reason)
	{
		if (!entry)
			return;

		entry->errors.fetch_add(1, std::memory_order_relaxed);
		SetReason(entry, reason);
	}

	inline void Error(const char* name, const char* reason)
	{
		Error(Register(name), reason);
	}

	inline void Error(const char* name, const std::string& reason)
	{
		Error(name, reason.c_str());
	}

	inline void Error(Entry* entry, const std::string& reason)
	{
		Error(entry, reason.c_str());
	}

	class Scope
	{
		Entry* m_Entry = nullptr;
		uint64_t m_StartUs = 0;
		bool m_Active = true;

	public:
		Scope(Entry* entry) :
			m_Entry(entry), m_StartUs(NowUs())
		{
		}

		Scope(const char* name, uint64_t slowUs = DEFAULT_SLOW_US) :
			m_Entry(Register(name, slowUs)), m_StartUs(NowUs())
		{
		}

		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;

		~Scope()
		{
			if (m_Active)
				Record(m_Entry, NowUs() - m_StartUs);
		}

		void Cancel()
		{
			m_Active = false;
		}
	};

	inline void Flush(const char* source)
	{
		for (auto& entry : g_Entries)
		{
			if (!entry.name)
				continue;

			const auto calls = entry.calls.exchange(0, std::memory_order_relaxed);
			const auto skipped = entry.skipped.exchange(0, std::memory_order_relaxed);
			const auto errors = entry.errors.exchange(0, std::memory_order_relaxed);
			const auto totalUs = entry.totalUs.exchange(0, std::memory_order_relaxed);
			const auto maxUs = entry.maxUs.exchange(0, std::memory_order_relaxed);
			const auto slowCalls = entry.slowCalls.exchange(0, std::memory_order_relaxed);

			if (!calls && !skipped && !errors)
				continue;

			const auto avgUs = calls ? totalUs / calls : 0;
			FW_TRACE(std::format(
				"[FUNC] source={} name={} calls={} skipped={} errors={} avg_us={} max_us={} slow={} last={}",
				source ? source : "<unknown>",
				entry.name,
				calls,
				skipped,
				errors,
				avgUs,
				maxUs,
				slowCalls,
				entry.lastReason[0] ? entry.lastReason : "ok"
			).c_str());
		}
	}

	inline void MaybeFlush(const char* source)
	{
		const auto now = NowMs();
		auto last = g_LastFlushMs.load(std::memory_order_relaxed);
		if (last && now - last < FLUSH_MS)
			return;

		if (g_LastFlushMs.compare_exchange_strong(last, now, std::memory_order_relaxed))
			Flush(source);
	}
}

#define FDBG_CONCAT_INNER(a, b) a##b
#define FDBG_CONCAT(a, b) FDBG_CONCAT_INNER(a, b)
#define FDBG_SCOPE(name) static auto* FDBG_CONCAT(fdbg_entry_, __LINE__) = FunctionalDebug::Register(name); FunctionalDebug::Scope FDBG_CONCAT(fdbg_scope_, __LINE__)(FDBG_CONCAT(fdbg_entry_, __LINE__))
#define FDBG_SCOPE_SLOW(name, slowUs) static auto* FDBG_CONCAT(fdbg_entry_, __LINE__) = FunctionalDebug::Register(name, slowUs); FunctionalDebug::Scope FDBG_CONCAT(fdbg_scope_, __LINE__)(FDBG_CONCAT(fdbg_entry_, __LINE__))
#define FDBG_CALL(name, ...) do { static auto* FDBG_CONCAT(fdbg_entry_, __LINE__) = FunctionalDebug::Register(name); FunctionalDebug::Scope FDBG_CONCAT(fdbg_scope_, __LINE__)(FDBG_CONCAT(fdbg_entry_, __LINE__)); __VA_ARGS__; } while (0)
#define FDBG_CALL_SLOW(name, slowUs, ...) do { static auto* FDBG_CONCAT(fdbg_entry_, __LINE__) = FunctionalDebug::Register(name, slowUs); FunctionalDebug::Scope FDBG_CONCAT(fdbg_scope_, __LINE__)(FDBG_CONCAT(fdbg_entry_, __LINE__)); __VA_ARGS__; } while (0)
#define FDBG_SKIP(name, reason) do { static auto* FDBG_CONCAT(fdbg_entry_, __LINE__) = FunctionalDebug::Register(name); FunctionalDebug::Skip(FDBG_CONCAT(fdbg_entry_, __LINE__), reason); } while (0)
#define FDBG_ERROR(name, reason) do { static auto* FDBG_CONCAT(fdbg_entry_, __LINE__) = FunctionalDebug::Register(name); FunctionalDebug::Error(FDBG_CONCAT(fdbg_entry_, __LINE__), reason); } while (0)
#define FDBG_FLUSH(source) FunctionalDebug::MaybeFlush(source)
#else
#define FDBG_SCOPE(name) ((void)0)
#define FDBG_SCOPE_SLOW(name, slowUs) ((void)0)
#define FDBG_CALL(name, ...) do { __VA_ARGS__; } while (0)
#define FDBG_CALL_SLOW(name, slowUs, ...) do { __VA_ARGS__; } while (0)
#define FDBG_SKIP(name, reason) ((void)0)
#define FDBG_ERROR(name, reason) ((void)0)
#define FDBG_FLUSH(source) ((void)0)
#endif
