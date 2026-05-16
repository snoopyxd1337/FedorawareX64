#pragma once
#include "../Interfaces/Interfaces.h"
#include "../../Utils/Minidump/Minidump.h"

#include <format>
#include <string>

class CNetVars
{
	struct node;
	using map_type = std::unordered_map<std::string, std::shared_ptr<node>>;

	struct node
	{
		node(DWORD offset) : offset(offset) {}
		map_type nodes;
		DWORD offset;
	};

	map_type nodes;

public:
	void Init();

private:
	void populate_nodes(class RecvTable* recv_table, map_type* map);

	DWORD get_offset_recursive(map_type& map, int acc, const std::string& path, const char* name)
	{
		if (!name)
		{
			FW_TRACE(("[ERROR] Dynamic netvar missing | path=" + path + " reason=null-prop-name").c_str());
			return 0;
		}

		const auto it = map.find(name);
		if (it == map.end() || !it->second)
		{
			FW_TRACE(("[ERROR] Dynamic netvar missing | path=" + path + "." + name + " reason=prop-not-found").c_str());
			return 0;
		}

		const auto result = acc + it->second->offset;
		FW_TRACE(std::format("[NETVAR] dynamic path={}.{} offset=0x{:x}", path, name, result).c_str());
		return result;
	}

	template<typename ...args_t>
	DWORD get_offset_recursive(map_type& map, int acc, const std::string& path, const char* name, args_t ...args)
	{
		if (!name)
		{
			FW_TRACE(("[ERROR] Dynamic netvar missing | path=" + path + " reason=null-prop-name").c_str());
			return 0;
		}

		const auto it = map.find(name);
		if (it == map.end() || !it->second)
		{
			FW_TRACE(("[ERROR] Dynamic netvar missing | path=" + path + "." + name + " reason=datatable-or-prop-not-found").c_str());
			return 0;
		}

		return get_offset_recursive(it->second->nodes, acc + it->second->offset, path + "." + name, args...);
	}

public:
	template<typename ...args_t>
	DWORD get_offset(const char* name, args_t ...args)
	{
		if (!name)
		{
			FW_TRACE("[ERROR] Dynamic netvar table missing | reason=null-table-name");
			return 0;
		}

		const auto it = nodes.find(name);
		if (it == nodes.end() || !it->second)
		{
			FW_TRACE((std::string("[ERROR] Dynamic netvar table missing | table=") + name + " reason=recv-table-not-found").c_str());
			return 0;
		}

		return get_offset_recursive(it->second->nodes, it->second->offset, name, args...);
	}
};

extern CNetVars g_NetVars;

template<typename T>
class CDynamicNetvar
{
private:
	DWORD dwOffset;
	bool m_ReadFailureLogged = false;
	bool m_WriteFailureLogged = false;

public:
	template<typename... args_t>
	CDynamicNetvar(args_t... a)
	{
		dwOffset = g_NetVars.get_offset(a...);
	}

	template<typename... args_t>
	CDynamicNetvar(int nOffset, args_t... a)
	{
		dwOffset = g_NetVars.get_offset(a...) + nOffset;
	}

	T GetValue(void* base)
	{
		if (!base)
		{
			if (!m_ReadFailureLogged)
			{
				FW_TRACE("[ERROR] Dynamic netvar read failed | reason=base-null");
				m_ReadFailureLogged = true;
			}
			return T{};
		}

		if (!dwOffset)
		{
			if (!m_ReadFailureLogged)
			{
				FW_TRACE(std::format("[ERROR] Dynamic netvar read failed | base={} reason=offset-zero-or-missing", Minidump::DescribeAddress(reinterpret_cast<uintptr_t>(base))).c_str());
				m_ReadFailureLogged = true;
			}
			return T{};
		}

		const auto address = reinterpret_cast<uintptr_t>(base) + dwOffset;
		return *reinterpret_cast<T*>(address);
	}

	void SetValue(void* base, T val)
	{
		if (!base)
		{
			if (!m_WriteFailureLogged)
			{
				FW_TRACE("[ERROR] Dynamic netvar write failed | reason=base-null");
				m_WriteFailureLogged = true;
			}
			return;
		}

		if (!dwOffset)
		{
			if (!m_WriteFailureLogged)
			{
				FW_TRACE(std::format("[ERROR] Dynamic netvar write failed | base={} reason=offset-zero-or-missing", Minidump::DescribeAddress(reinterpret_cast<uintptr_t>(base))).c_str());
				m_WriteFailureLogged = true;
			}
			return;
		}

		*reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(base) + dwOffset) = val;
	}
};

#define DYNVAR(name, type, ...) static CDynamicNetvar<type> name(__VA_ARGS__)
#define DYNVAR_RETURN(type, base, ...) DYNVAR(n, type, __VA_ARGS__); return n.GetValue(base)
#define DYNVAR_SET(type, base, value, ...) DYNVAR(n, type, __VA_ARGS__); n.SetValue(base,value)

#define M_DYNVARGET(name, type, base, ...) __inline type Get##name() \
{ \
	static CDynamicNetvar<type>  Var##name( __VA_ARGS__ ); \
	return Var##name.GetValue(base); \
}

#define M_OFFSETGET(name, type, offset) __inline type Get##name() \
{ \
	return *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + offset); \
}

#define M_FLAGGET(name, flags, flag) __inline bool Is##name() \
{ \
	return (flags & flag); \
}

#define M_CONDGET(name, conditions, cond) __inline bool Is##name() \
{ \
	return (conditions(cond)); \
}

// lol

int GetOffset(RecvTable* pTable, const char* szNetVar);
int GetNetVar(const char* szClass, const char* szNetVar);

#define NETVAR(_name, type, table, name) inline type &_name() \
{ \
	static int offset = GetNetVar(table, name); \
	static bool loggedMissing = false; \
	if (!offset) \
	{ \
		if (!loggedMissing) \
		{ \
			FW_TRACE(std::format("[ERROR] NETVAR accessor failed | accessor={} table={} prop={} reason=offset-zero-or-missing", #_name, table, name).c_str()); \
			loggedMissing = true; \
		} \
		static type fallback{}; \
		return fallback; \
	} \
	return *reinterpret_cast<type *>(reinterpret_cast<uintptr_t>(this) + offset); \
}

#define NETVAR_OFF(_name, type, table, name, off) inline type &_name() \
{ \
	static int netvarOffset = GetNetVar(table, name); \
	static int offset = netvarOffset + off; \
	static bool loggedMissing = false; \
	if (!netvarOffset) \
	{ \
		if (!loggedMissing) \
		{ \
			FW_TRACE(std::format("[ERROR] NETVAR_OFF accessor failed | accessor={} table={} prop={} extra_off={} reason=base-offset-zero-or-missing", #_name, table, name, off).c_str()); \
			loggedMissing = true; \
		} \
		static type fallback{}; \
		return fallback; \
	} \
	return *reinterpret_cast<type *>(reinterpret_cast<uintptr_t>(this) + offset); \
}

#define NETVAR_PTR(_name, type, table, name) inline type _name() \
{ \
	static int offset = GetNetVar(table, name); \
	static bool loggedMissing = false; \
	if (!offset) \
	{ \
		if (!loggedMissing) \
		{ \
			FW_TRACE(std::format("[ERROR] NETVAR_PTR accessor failed | accessor={} table={} prop={} reason=offset-zero-or-missing", #_name, table, name).c_str()); \
			loggedMissing = true; \
		} \
		return nullptr; \
	} \
	return reinterpret_cast<type>(reinterpret_cast<uintptr_t>(this) + offset); \
}
