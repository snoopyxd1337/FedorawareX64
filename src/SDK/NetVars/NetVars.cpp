#include "NetVars.h"

#include <format>
#include <string_view>

#undef GetProp

void CNetVars::Init()
{
	if (!I::BaseClientDLL)
	{
		FW_TRACE("[ERROR] NetVars::Init failed | reason=BaseClientDLL-null");
		return;
	}

	const auto* client_class = I::BaseClientDLL->GetAllClasses();
	if (!client_class)
	{
		FW_TRACE("[ERROR] NetVars::Init failed | reason=GetAllClasses-returned-null");
		return;
	}

	while (client_class != nullptr)
	{
		const auto class_info = std::make_shared<node>(0);
		RecvTable* recv_table = reinterpret_cast<RecvTable*>(client_class->m_pRecvTable);
		if (!class_info || !recv_table)
		{
			FW_TRACE(std::format("[WARN] NetVars::Init skipped class | class={} reason=recv-table-null", client_class->m_pNetworkName ? client_class->m_pNetworkName : "<null>").c_str());
			client_class = client_class->m_pNext;
			continue;
		}

		populate_nodes(recv_table, &class_info->nodes);
		nodes.emplace(recv_table->GetName(), class_info);

		client_class = client_class->m_pNext;
	}

	FW_TRACE(std::format("[NETVAR] initialized recv tables={}", nodes.size()).c_str());
}

void CNetVars::populate_nodes(RecvTable* recv_table, map_type* map)
{
	if (!recv_table || !map)
	{
		FW_TRACE("[ERROR] NetVars::populate_nodes failed | reason=null-table-or-map");
		return;
	}

	for (auto i = 0; i < recv_table->GetNumProps(); i++)
	{
		const auto* prop = recv_table->GetProp(i);
		if (!prop || !prop->GetName())
		{
			FW_TRACE(std::format("[WARN] NetVars::populate_nodes skipped prop | table={} index={} reason=prop-null", recv_table->GetName() ? recv_table->GetName() : "<null>", i).c_str());
			continue;
		}

		const auto prop_info = std::make_shared<node>(prop->GetOffset());

		if (prop->GetType() == DPT_DataTable)
		{
			if (auto* data_table = prop->GetDataTable())
			{
				populate_nodes(data_table, &prop_info->nodes);
			}
			else
			{
				FW_TRACE(std::format("[WARN] NetVars::populate_nodes datatable prop has null table | table={} prop={}", recv_table->GetName() ? recv_table->GetName() : "<null>", prop->GetName()).c_str());
			}
		}

		map->emplace(prop->GetName(), prop_info);
	}
}

int GetOffset(RecvTable* pTable, const char* szNetVar)
{
	if (!pTable || !szNetVar)
		return 0;

	for (int i = 0; i < pTable->m_nProps; i++)
	{
		RecvProp Prop = pTable->m_pProps[i];
		if (!Prop.m_pVarName)
			continue;

		if (std::string_view(Prop.m_pVarName).compare(szNetVar) == 0)
			return Prop.GetOffset();

		if (auto DataTable = Prop.GetDataTable())
		{
			if (auto nOffset = GetOffset(DataTable, szNetVar))
				return nOffset + Prop.GetOffset();
		}
	}

	return 0;
}

int GetNetVar(const char* szClass, const char* szNetVar)
{
	if (!I::BaseClientDLL)
	{
		FW_TRACE(std::format("[ERROR] NetVar lookup failed | class={} prop={} reason=BaseClientDLL-null", szClass ? szClass : "<null>", szNetVar ? szNetVar : "<null>").c_str());
		return 0;
	}

	if (!szClass || !szNetVar)
	{
		FW_TRACE(std::format("[ERROR] NetVar lookup failed | class={} prop={} reason=null-argument", szClass ? szClass : "<null>", szNetVar ? szNetVar : "<null>").c_str());
		return 0;
	}

	CClientClass* pClasses = I::BaseClientDLL->GetAllClasses();
	if (!pClasses)
	{
		FW_TRACE(std::format("[ERROR] NetVar lookup failed | class={} prop={} reason=GetAllClasses-returned-null", szClass, szNetVar).c_str());
		return 0;
	}

	for (auto pCurrNode = pClasses; pCurrNode; pCurrNode = pCurrNode->m_pNext)
	{
		if (!pCurrNode->m_pNetworkName)
			continue;

		if (std::string_view(szClass).compare(pCurrNode->m_pNetworkName) == 0)
		{
			const auto offset = GetOffset(pCurrNode->m_pRecvTable, szNetVar);
			if (!offset)
				FW_TRACE(std::format("[ERROR] NetVar prop missing | class={} prop={} reason=prop-not-found-or-zero-offset", szClass, szNetVar).c_str());
			else
				FW_TRACE(std::format("[NETVAR] class={} prop={} offset=0x{:x}", szClass, szNetVar, offset).c_str());
			return offset;
		}
	}

	FW_TRACE(std::format("[ERROR] NetVar class missing | class={} prop={} reason=client-class-not-found", szClass, szNetVar).c_str());
	return 0;
}

CNetVars g_NetVars;
