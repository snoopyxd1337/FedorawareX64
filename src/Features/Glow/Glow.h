#pragma once
#include "../Feature.h"
#include "../Colour.h"

class CGlowEffect
{
private:
	IMaterial* m_pMatGlowColor = nullptr;
	ITexture* m_pRtFullFrame = nullptr;
	ITexture* m_pRtQuarterSize1 = nullptr;
	ITexture* m_pRenderBuffer1 = nullptr;
	ITexture* m_pRenderBuffer2 = nullptr;
	IMaterial* m_pMatBlurXwf = nullptr;
	IMaterial* m_pMatBlurX = nullptr;
	IMaterial* m_pMatBlurYwf = nullptr;
	IMaterial* m_pMatBlurY = nullptr;
	IMaterial* m_pMatHaloAddToScreen = nullptr;

	struct GlowEnt_t
	{
		CBaseEntity* m_pEntity;
		Color_t m_Color;
		float m_flAlpha;
	};

	std::vector<GlowEnt_t> m_vecGlowEntities;
	std::unordered_map<CBaseEntity*, bool> m_DrawnEntities;

private:
	void DrawModel(CBaseEntity* pEntity, int nFlags, bool bIsDrawingModels);
	void SetScale(int nScale, bool bReset);

public:
	void Init();
	void Render();
	void CreateMaterials();
	void DeleteMaterials();
	void Unload();

	bool HasDrawn(CBaseEntity* pEntity)
	{
		return m_DrawnEntities.find(pEntity) != m_DrawnEntities.end();
	}

	bool IsGlowMaterial(IMaterial* pMat)
	{
		return pMat == m_pMatGlowColor;
	}

public:
	bool m_bDrawingGlow = false;
	bool m_bRendering = false;
};

ADD_FEATURE(CGlowEffect, Glow)
