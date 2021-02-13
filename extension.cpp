#include "extension.h"
#include "CDetour/detours.h"

CSurvivorObjectCollectionExt g_SurvivorObjectCollectionExt;
CGlobalVars *gpGlobals = NULL;

SMEXT_LINK(&g_SurvivorObjectCollectionExt);

static int GetMaxCarryOfWeapon(const char *wpnName)
{
	static ConVarRef r_ammo_assaultrifle_max("ammo_assaultrifle_max"),
			r_ammo_smg_max("ammo_smg_max"),
			r_ammo_autoshotgun_max("ammo_autoshotgun_max"),
			r_ammo_grenadelauncher_max("ammo_grenadelauncher_max"),
			r_ammo_huntingrifle_max("ammo_huntingrifle_max"),
			r_ammo_m60_max("ammo_m60_max"),
			r_ammo_shotgun_max("ammo_shotgun_max"),
			r_ammo_sniperrifle_max("ammo_sniperrifle_max"),
			r_ammo_pistol_max("ammo_pistol_max");

	if (!V_strcmp(wpnName, "weapon_smg") || !V_strcmp(wpnName, "weapon_smg_silenced") || !V_strcmp(wpnName, "weapon_smg_mp5"))
	{
		return r_ammo_smg_max.GetInt();
	}

	if (!V_strcmp(wpnName, "weapon_pumpshotgun") || !V_strcmp(wpnName, "weapon_shotgun_chrome"))
	{
		return r_ammo_shotgun_max.GetInt();
	}

	if (!V_strcmp(wpnName, "weapon_autoshotgun") || !V_strcmp(wpnName, "weapon_shotgun_spas"))
	{
		return r_ammo_autoshotgun_max.GetInt();
	}

	if (!V_strcmp(wpnName, "weapon_rifle") || !V_strcmp(wpnName, "weapon_rifle_ak47") 
		|| !V_strcmp(wpnName, "weapon_rifle_desert") || !V_strcmp(wpnName, "weapon_rifle_sg552"))
	{
		return r_ammo_assaultrifle_max.GetInt();
	}

	if (!V_strcmp(wpnName, "weapon_rifle_m60"))
	{
		return r_ammo_m60_max.GetInt();
	}

	if (!V_strcmp(wpnName, "weapon_hunting_rifle"))
	{
		return r_ammo_huntingrifle_max.GetInt();
	}

	if (!V_strcmp(wpnName, "weapon_sniper_military") || !V_strcmp(wpnName, "weapon_sniper_awp") 
		|| !V_strcmp(wpnName, "weapon_sniper_scout"))
	{
		return r_ammo_sniperrifle_max.GetInt();
	}

	if (!V_strcmp(wpnName, "weapon_grenade_launcher"))
	{
		return r_ammo_grenadelauncher_max.GetInt();
	}

	if (!V_strcmp(wpnName, "weapon_pistol") || !V_strcmp(wpnName, "weapon_pistol_magnum"))
	{
		return r_ammo_pistol_max.GetInt();
	}

	return -1;
}

DETOUR_DECL_MEMBER1(Handler_SurvivorCollectObject_ShouldGiveUp, bool, CBaseEntity *, me)
{
	SurvivorTeamSituation *situation = g_SurvivorObjectCollectionExt.GetTeamSituation(me);

	if (g_SurvivorObjectCollectionExt.GetTonguedFriend(situation))
	{
		return true;
	}

	if (g_SurvivorObjectCollectionExt.GetPouncedFriend(situation))
	{
		return true;
	}

	if (g_SurvivorObjectCollectionExt.GetPummeledFriend(situation))
	{
		return true;
	}

	if (g_SurvivorObjectCollectionExt.GetFriendIntrouble(situation))
	{
		return true;
	}

	Action<CBaseEntity> *action = reinterpret_cast<Action<CBaseEntity> *>(this);

	if (g_SurvivorObjectCollectionExt.GetTankCount() > 0 || g_SurvivorObjectCollectionExt.GetTimeSinceAttackedByEnemy(me) < 2.0f)
	{
		CBaseEntity *obj = g_SurvivorObjectCollectionExt.GetUseObject(action);

		if (obj)
		{
			const char *cls = gamehelpers->GetEntityClassname(obj);

			if (!V_strcmp(cls, "weapon_ammo_spawn"))
			{
				CBaseEntity *wpn = g_SurvivorObjectCollectionExt.Weapon_GetSlot(me, WEAPON_SLOT_RIFLE);

				if (wpn) 
				{
					const char *wpnName = gamehelpers->GetEntityClassname(wpn);

					if (g_SurvivorObjectCollectionExt.GetAmmoCount(me, g_SurvivorObjectCollectionExt.GetPrimaryAmmoType(wpn)) >= 
						(GetMaxCarryOfWeapon(wpnName) * 0.4f))	// 40% of max carry is vanilla behavior
					{
						return true;
					}

					return false;
				}
			}
		}

		// return true;	// vanilla behavior
	}

	return g_SurvivorObjectCollectionExt.ShouldGiveUp(action, me);
}

bool CSurvivorObjectCollectionExt::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	sm_sendprop_info_t info;
	if (gamehelpers->FindSendPropInfo("CTerrorPlayer", "m_iAmmo", &info))
	{
		sendprop_m_iAmmo = info.actual_offset;
	}
	else
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find send prop info \"CTerrorPlayer::m_iAmmo\"");

		return false;
	}

	IGameConfig *gc;
	if (!gameconfs->LoadGameConfigFile("sdktools.games", &gc, error, maxlen)) 
	{
		ke::SafeStrcpy(error, maxlen, "Unable to load gamedata file \"sdktools.games.txt\"");

		return false;
	}

	if (!gc->GetOffset("Weapon_GetSlot", &vtblindex_CBaseCombatCharacter_Weapon_GetSlot))
	{
		ke::SafeStrcpy(error, maxlen, "Unable to get offset for \"Weapon_GetSlot\"");

		gameconfs->CloseGameConfigFile(gc);

		return false;
	}

	gameconfs->CloseGameConfigFile(gc);
	#define GAMEDATA_FILE	"survivor_object_collection"
	if (!gameconfs->LoadGameConfigFile(GAMEDATA_FILE, &gc, error, sizeof(error))) 
	{
		ke::SafeStrcpy(error, maxlen, "Unable to load gamedata file \"" GAMEDATA_FILE ".txt\"");

		return false;
	}

	if (!gc->GetAddress("CDirector", &addr_TheDirector))
	{
		ke::SafeStrcpy(error, maxlen, "Unable to get address of CDirector instance");

		gameconfs->CloseGameConfigFile(gc);

		return false;
	}

	if (!gc->GetMemSig("SurvivorBot::GetTeamSituation", &pfn_SurvivorBot_GetTeamSituation))
	{
		ke::SafeStrcpy(error, maxlen, "Unable to get mem sig for \"SurvivorBot::GetTeamSituation\"");

		gameconfs->CloseGameConfigFile(gc);

		return false;
	}

	if (!gc->GetMemSig("SurvivorUseObject::ShouldGiveUp", &pfn_SurvivorUseObject_ShouldGiveUp))
	{
		ke::SafeStrcpy(error, maxlen, "Unable to get mem sig for \"SurvivorUseObject::ShouldGiveUp\"");

		gameconfs->CloseGameConfigFile(gc);

		return false;
	}

	static const struct 
	{
		const char* key;
		int& offset;
	}
	s_offsets[] = 
	{
		{ "SurvivorTeamSituation::m_friendInTrouble", offset_SurvivorTeamSituation_m_friendInTrouble },
		{ "SurvivorTeamSituation::m_tonguedFriend", offset_SurvivorTeamSituation_m_tonguedFriend },
		{ "SurvivorTeamSituation::m_pouncedFriend", offset_SurvivorTeamSituation_m_pouncedFriend },
		{ "SurvivorTeamSituation::m_pummeledFriend", offset_SurvivorTeamSituation_m_pummeledFriend },
		{ "SurvivorCollectObject::m_useObject", offset_SurvivorCollectObject_m_useObject },
		{ "CTerrorPlayer::m_timeSinceAttackedByEnemyTimer", offset_CTerrorPlayer_m_timeSinceAttackedByEnemyTimer },
		{ "CDirector::m_iTankCount", offset_CDirector_m_iTankCount },
	};

	for (auto&& el : s_offsets) 
	{
		if (!gc->GetOffset(el.key, &el.offset)) 
		{
			ke::SafeSprintf(error, maxlen, "Unable to get offset for \"%s\"", el.key);

			gameconfs->CloseGameConfigFile(gc);

			return false;
		}
	}

	CDetourManager::Init(smutils->GetScriptingEngine(), gc);

	detour_SurvivorCollectObject_ShouldGiveUp = DETOUR_CREATE_MEMBER(Handler_SurvivorCollectObject_ShouldGiveUp, "SurvivorCollectObject::ShouldGiveUp");

	if (detour_SurvivorCollectObject_ShouldGiveUp)
	{
		detour_SurvivorCollectObject_ShouldGiveUp->EnableDetour();
	}
	else
	{
		ke::SafeStrcpy(error, maxlen, "Failed to initialize detour SurvivorCollectObject::ShouldGiveUp");

		gameconfs->CloseGameConfigFile(gc);

		return false;
	}

	gameconfs->CloseGameConfigFile(gc);

	sharesys->AddDependency(myself, "bintools.ext", true, true);
	
	return true;
}

void CSurvivorObjectCollectionExt::SDK_OnUnload()
{
	if (detour_SurvivorCollectObject_ShouldGiveUp)
	{
		detour_SurvivorCollectObject_ShouldGiveUp->Destroy();
	}

	if (vcall_CBaseCombatCharacter_Weapon_GetSlot)
	{
		vcall_CBaseCombatCharacter_Weapon_GetSlot->Destroy();
	}

	if (call_SurvivorBot_GetTeamSituation)
	{
		call_SurvivorBot_GetTeamSituation->Destroy();
	}
}

void CSurvivorObjectCollectionExt::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, bintools);

	PassInfo params_vcall_Weapon_GetSlot[] =
	{
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(int), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(CBaseEntity *), NULL, 0 }
	};

	PassInfo params_call_ShouldGiveUp[] =
	{
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(CBaseEntity *), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(bool), NULL, 0 }
	};

	PassInfo param_call_GetTeamSituation =
	{
		PassType_Basic, PASSFLAG_BYVAL, sizeof(SurvivorTeamSituation *), NULL, 0
	};

	vcall_CBaseCombatCharacter_Weapon_GetSlot = bintools->CreateVCall(vtblindex_CBaseCombatCharacter_Weapon_GetSlot, 0, 0, &params_vcall_Weapon_GetSlot[1], &params_vcall_Weapon_GetSlot[0], 1);

	if (!vcall_CBaseCombatCharacter_Weapon_GetSlot)
	{
		smutils->LogError(myself, "Unable to create ICallWrapper for \"CBaseCombatCharacter::Weapon_GetSlot\"");
	}

	call_SurvivorBot_GetTeamSituation = bintools->CreateCall(pfn_SurvivorBot_GetTeamSituation, CallConv_ThisCall, &param_call_GetTeamSituation, NULL, 0);

	if (!call_SurvivorBot_GetTeamSituation)
	{
		smutils->LogError(myself, "Unable to create ICallWrapper for \"SurvivorBot::GetTeamSItuation\"");
	}

	call_SurvivorUseObject_ShouldGiveUp = bintools->CreateCall(pfn_SurvivorUseObject_ShouldGiveUp, CallConv_ThisCall, &params_call_ShouldGiveUp[1], &params_call_ShouldGiveUp[0], 1);

	if (!call_SurvivorUseObject_ShouldGiveUp)
	{
		smutils->LogError(myself, "Unable to create ICallWrapper for \"SurvivorUseObject::ShouldGiveUp\"");
	}
}

bool CSurvivorObjectCollectionExt::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, bintools);

	return true;
}

bool CSurvivorObjectCollectionExt::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);

	gpGlobals = ismm->GetCGlobals();

	return true;
}

float CSurvivorObjectCollectionExt::GetTimeSinceAttackedByEnemy(CBaseEntity *player)
{
	float timestamp = *reinterpret_cast<float *>(reinterpret_cast<char *>(player) + offset_CTerrorPlayer_m_timeSinceAttackedByEnemyTimer + 4);

	if (timestamp > 0.0f)
	{
		return (gpGlobals->curtime - timestamp);
	}

	return 99999.898f;
}

int CSurvivorObjectCollectionExt::GetPrimaryAmmoType(CBaseEntity *weapon)
{
	static int offset = -1;

	if (offset == -1)
	{
		datamap_t *pMap = gamehelpers->GetDataMap(weapon);

		sm_datatable_info_t info;
		if (gamehelpers->FindDataMapInfo(pMap, "m_iPrimaryAmmoType", &info))
		{
			offset = info.actual_offset;
		}
	}

	return *reinterpret_cast<int *>(reinterpret_cast<char *>(weapon) + offset);
}

CBaseEntity *CSurvivorObjectCollectionExt::Weapon_GetSlot(CBaseEntity *player, int slot)
{
	struct 
	{
		CBaseEntity *player;
		int slot;
	} stack = 
	{
		player,
		slot
	};

	CBaseEntity *result = NULL;
	vcall_CBaseCombatCharacter_Weapon_GetSlot->Execute(&stack, &result);

	return result;
}

SurvivorTeamSituation *CSurvivorObjectCollectionExt::GetTeamSituation(CBaseEntity *bot)
{
	SurvivorTeamSituation *result = NULL;
	call_SurvivorBot_GetTeamSituation->Execute(&bot, &result);

	return result;
}

bool CSurvivorObjectCollectionExt::ShouldGiveUp(Action<CBaseEntity> *action, CBaseEntity *bot)
{
	struct 
	{
		Action<CBaseEntity> *action;
		CBaseEntity *bot;
	} stack = 
	{
		action,
		bot
	};

	bool result;
	call_SurvivorUseObject_ShouldGiveUp->Execute(&stack, &result);

	return result;
}
