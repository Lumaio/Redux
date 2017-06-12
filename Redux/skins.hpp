#pragma once

#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <map>
#include <memory>
#include "SDK\InterfaceManager\InterfaceManager.hpp"
#include "SDK\SourceEngine\SDK.hpp"
#include "SDK\SourceEngine\IInputSystem.h"
#include "SDK\CSGOStructs.hpp"
#include "SDK\VFTableHook.hpp"
#include "SDK\NetvarManager.hpp"
#include "SDK\Utils.hpp"
#include "interfaces.hpp"
#include "CBaseAttributeItem.hpp"

struct CSWeapon {
	int kit = 0;
	int seed = 0;
	int stattrack = -1;
	int quality = 4;
	char* name = nullptr;
	float wear = 0.0003f;
};

static std::map<int, CSWeapon> skinconfig;
static std::map<int, const char*> viewmodel_config;
static std::map<const char*, const char*> killicon_config;

//inline void set_skins(int AK, int A1, int A4, int DEAG, int USPS)
inline void set_skins(SourceEngine::EItemDefinitionIndex id, int skin)
{
	// Custom skins
	skinconfig[id].kit = skin;
	skinconfig[id].quality = 6;
}

inline void set_model(const char* file, const char* simple)
{
	int knife_ct_o = SourceEngine::Interfaces::ModelInfo()->GetModelIndex("models/weapons/v_knife_default_ct.mdl");
	int knife_t_o = SourceEngine::Interfaces::ModelInfo()->GetModelIndex("models/weapons/v_knife_default_t.mdl");


	if (strcmp(file, "DEFAULT") != 0) {
		viewmodel_config[knife_ct_o] = file;
		viewmodel_config[knife_t_o] = file;

		killicon_config["knife_default_ct"] = simple;
		killicon_config["knife_default_t"] = simple;
	}
	else
	{
		viewmodel_config[knife_ct_o] = "models/weapons/v_knife_default_ct.mdl";
		viewmodel_config[knife_t_o] = "models/weapons/v_knife_default_t.mdl";

		killicon_config["knife_default_ct"] = "knife_default_ct";
		killicon_config["knife_default_t"] = "knife_default_t";
	}

}

inline bool apply_skin(DWORD dwWeapon)
{
	CBaseAttributeItem* weapon = (CBaseAttributeItem*)EntityList->GetClientEntityFromHandle(dwWeapon);
	if (!weapon)
		return false;

	int index = *weapon->GetItemDefinitionIndex();
	if (skinconfig.find(index) == skinconfig.end())
		return false;

	*weapon->GetFallbackPaintKit() = skinconfig[index].kit;
	*weapon->GetEntityQuality() = skinconfig[index].quality;
	*weapon->GetFallbackSeed() = skinconfig[index].seed;
	*weapon->GetFallbackStatTrak() = skinconfig[index].stattrack;
	*weapon->GetFallbackWear() = skinconfig[index].wear;

	if (skinconfig[index].name) {
		sprintf(weapon->GetCustomName(), "%s", skinconfig[index].name);
	}

	*weapon->GetItemIDHigh() = -1;

	return true;
}

inline bool apply_model(C_CSPlayer* local, CBaseAttributeItem* pweapon, int index)
{
	if (!pweapon) return false;

	CBaseViewModel* viewmodel = local->GetViewModel();

	if (!viewmodel) return false;

	DWORD viewmodel_weapon = viewmodel->GetWeapon();
	CBaseAttributeItem* weapon = (CBaseAttributeItem*)EntityList->GetClientEntityFromHandle(viewmodel_weapon);

	if (weapon != pweapon) return false;

	int nindex = viewmodel->GetModelIndex();

	if (viewmodel_config.find(nindex) == viewmodel_config.end()) return false;

	viewmodel->SetWeaponModel(viewmodel_config[nindex], pweapon);
	return true;


}
