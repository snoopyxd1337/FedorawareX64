#pragma once
#include "../../Includes/Includes.h"

class CClientEntityList
{
public:
	bool IsEntityIndexValid(int nEntityIndex)
	{
		return nEntityIndex > 0 && nEntityIndex < MAX_EDICTS;
	}

	bool IsHandleValid(int hEntity)
	{
		if (hEntity == INVALID_EHANDLE_INDEX || hEntity == 0)
		{
			return false;
		}

		return IsEntityIndexValid(hEntity & ENT_ENTRY_MASK);
	}

	bool IsHandleValid(const CBaseHandle& hEntity)
	{
		return IsHandleValid(hEntity.ToInt());
	}

	CBaseEntity *GetClientEntity(int nEntityIndex)
	{
		typedef CBaseEntity *(__thiscall *FN)(PVOID, int);
		return GetVFunc<FN>(this, 3)(this, nEntityIndex);
	}

	CBaseEntity *GetClientEntityFromHandle(int hEntity)
	{
		typedef CBaseEntity *(__thiscall *FN)(PVOID, int);
		return GetVFunc<FN>(this, 4)(this, hEntity);
	}

	CBaseEntity* GetClientEntityFromHandle(const CBaseHandle& hEntity)
	{
		return GetClientEntityFromHandleSafe(hEntity);
	}

	CBaseEntity* GetClientEntitySafe(int nEntityIndex)
	{
		return IsEntityIndexValid(nEntityIndex) ? GetClientEntity(nEntityIndex) : nullptr;
	}

	CBaseEntity* GetClientEntityFromHandleSafe(int hEntity)
	{
		return GetClientEntityFromHandleSafe(CBaseHandle(static_cast<unsigned long>(hEntity)));
	}

	CBaseEntity* GetClientEntityFromHandleSafe(const CBaseHandle& hEntity)
	{
		if (!IsHandleValid(hEntity))
		{
			return nullptr;
		}

		CBaseEntity* pEntity = GetClientEntitySafe(hEntity.GetEntryIndex());
		if (!pEntity)
		{
			return nullptr;
		}

		const auto& entityHandle = reinterpret_cast<IHandleEntity*>(pEntity)->GetRefEHandle();
		return entityHandle == hEntity ? pEntity : nullptr;
	}

	int GetHighestEntityIndex()
	{
		typedef int(__thiscall *FN)(PVOID);
		return GetVFunc<FN>(this, 6)(this);
	}
};

inline IHandleEntity* CBaseHandle::Get() const
{
	return reinterpret_cast<IHandleEntity*>(I::ClientEntityList->GetClientEntityFromHandleSafe(*this));
}

#define VCLIENTENTITYLIST_INTERFACE_VERSION	"VClientEntityList003"
