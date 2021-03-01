#include "extension.h"
#include "CDetour/detours.h"

CSurvivorObjectCollectionExt g_SurvivorObjectCollectionExt;
CGlobalVars *gpGlobals = NULL;

SMEXT_LINK(&g_SurvivorObjectCollectionExt);

static int GetMaxCarryOfWeapon(const char *pWeaponName)
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

	if (!V_strcmp(pWeaponName, "weapon_smg") || !V_strcmp(pWeaponName, "weapon_smg_silenced") || !V_strcmp(pWeaponName, "weapon_smg_mp5"))
	{
		return r_ammo_smg_max.GetInt();
	}

	if (!V_strcmp(pWeaponName, "weapon_pumpshotgun") || !V_strcmp(pWeaponName, "weapon_shotgun_chrome"))
	{
		return r_ammo_shotgun_max.GetInt();
	}

	if (!V_strcmp(pWeaponName, "weapon_autoshotgun") || !V_strcmp(pWeaponName, "weapon_shotgun_spas"))
	{
		return r_ammo_autoshotgun_max.GetInt();
	}

	if (!V_strcmp(pWeaponName, "weapon_rifle") || !V_strcmp(pWeaponName, "weapon_rifle_ak47") 
		|| !V_strcmp(pWeaponName, "weapon_rifle_desert") || !V_strcmp(pWeaponName, "weapon_rifle_sg552"))
	{
		return r_ammo_assaultrifle_max.GetInt();
	}

	if (!V_strcmp(pWeaponName, "weapon_rifle_m60"))
	{
		return r_ammo_m60_max.GetInt();
	}

	if (!V_strcmp(pWeaponName, "weapon_hunting_rifle"))
	{
		return r_ammo_huntingrifle_max.GetInt();
	}

	if (!V_strcmp(pWeaponName, "weapon_sniper_military") || !V_strcmp(pWeaponName, "weapon_sniper_awp") 
		|| !V_strcmp(pWeaponName, "weapon_sniper_scout"))
	{
		return r_ammo_sniperrifle_max.GetInt();
	}

	if (!V_strcmp(pWeaponName, "weapon_grenade_launcher"))
	{
		return r_ammo_grenadelauncher_max.GetInt();
	}

	if (!V_strcmp(pWeaponName, "weapon_pistol") || !V_strcmp(pWeaponName, "weapon_pistol_magnum"))
	{
		return r_ammo_pistol_max.GetInt();
	}

	return -1;
}

DETOUR_DECL_MEMBER1(DetourFunc_SurvivorCollectObject_ShouldGiveUp, bool, CBaseEntity *, pBot)
{
	SurvivorTeamSituation *pSituation = g_SurvivorObjectCollectionExt.GetTeamSituation(pBot);

	if (g_SurvivorObjectCollectionExt.GetTonguedFriend(pSituation))
	{
		return true;
	}

	if (g_SurvivorObjectCollectionExt.GetPouncedFriend(pSituation))
	{
		return true;
	}

	if (g_SurvivorObjectCollectionExt.GetPummeledFriend(pSituation))
	{
		return true;
	}

	if (g_SurvivorObjectCollectionExt.GetFriendIntrouble(pSituation))
	{
		return true;
	}

	Action<CBaseEntity> *pAction = reinterpret_cast<Action<CBaseEntity> *>(this);

	if (g_SurvivorObjectCollectionExt.GetTankCount() > 0 || g_SurvivorObjectCollectionExt.GetTimeSinceAttackedByEnemy(pBot) < 2.0f)
	{
		CBaseEntity *pEntity = g_SurvivorObjectCollectionExt.GetUseObject(pAction);

		if (pEntity)
		{
			const char *pClassname = gamehelpers->GetEntityClassname(pEntity);

			if (!V_strcmp(pClassname, "weapon_ammo_spawn"))
			{
				CBaseEntity *pWeapon = g_SurvivorObjectCollectionExt.Weapon_GetSlot(pBot, WEAPON_SLOT_RIFLE);

				if (pWeapon) 
				{
					const char *pWeaponName = gamehelpers->GetEntityClassname(pWeapon);

					if (g_SurvivorObjectCollectionExt.GetAmmoCount(pBot, g_SurvivorObjectCollectionExt.GetPrimaryAmmoType(pWeapon)) >= (GetMaxCarryOfWeapon(pWeaponName) * 0.4f))
					{
						return true;
					}

					return false;
				}
			}
		}
	}

	return g_SurvivorObjectCollectionExt.ShouldGiveUp(pAction, pBot);
}

bool CSurvivorObjectCollectionExt::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	sm_sendprop_info_t info;
	if (gamehelpers->FindSendPropInfo("CTerrorPlayer", "m_iAmmo", &info))
	{
		m_iSendProp_CTerrorPlayer_m_iAmmo = info.actual_offset;
	}
	else
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find send prop info for \"CTerrorPlayer::m_iAmmo\"");

		return false;
	}

	IGameConfig *pGameConfig = NULL;

	if (!gameconfs->LoadGameConfigFile("sdktools.games", &pGameConfig, error, maxlen)) 
	{
		ke::SafeStrcpy(error, maxlen, "Unable to load gamedata file \"sdktools.games.txt\"");

		return false;
	}

	if (!pGameConfig->GetOffset("Weapon_GetSlot", &m_iVtblIndex_CBaseCombatCharacter_Weapon_GetSlot))
	{
		ke::SafeStrcpy(error, maxlen, "Unable to get offset for \"Weapon_GetSlot\"");

		gameconfs->CloseGameConfigFile(pGameConfig);

		return false;
	}

	gameconfs->CloseGameConfigFile(pGameConfig);
	#define GAMEDATA_FILE	"survivor_object_collection"
	if (!gameconfs->LoadGameConfigFile(GAMEDATA_FILE, &pGameConfig, error, sizeof(error))) 
	{
		ke::SafeStrcpy(error, maxlen, "Unable to load gamedata file \"" GAMEDATA_FILE ".txt\"");

		return false;
	}

	if (!pGameConfig->GetAddress("CDirector", &m_pObj_CDirector_TheDirector))
	{
		ke::SafeStrcpy(error, maxlen, "Unable to get address of CDirector instance");

		gameconfs->CloseGameConfigFile(pGameConfig);

		return false;
	}

	if (!pGameConfig->GetMemSig("SurvivorBot::GetTeamSituation", &m_pfn_SurvivorBot_GetTeamSituation))
	{
		ke::SafeStrcpy(error, maxlen, "Unable to get mem sig for \"SurvivorBot::GetTeamSituation\"");

		gameconfs->CloseGameConfigFile(pGameConfig);

		return false;
	}

	if (!pGameConfig->GetMemSig("SurvivorUseObject::ShouldGiveUp", &m_pfn_SurvivorUseObject_ShouldGiveUp))
	{
		ke::SafeStrcpy(error, maxlen, "Unable to get mem sig for \"SurvivorUseObject::ShouldGiveUp\"");

		gameconfs->CloseGameConfigFile(pGameConfig);

		return false;
	}

	static const struct 
	{
		const char* key;
		int& offset;
	}
	s_offsets[] = 
	{
		{ "SurvivorTeamSituation::m_friendInTrouble", m_iSurvivorTeamSituation_m_friendInTrouble },
		{ "SurvivorTeamSituation::m_tonguedFriend", m_iSurvivorTeamSituation_m_tonguedFriend },
		{ "SurvivorTeamSituation::m_pouncedFriend", m_iSurvivorTeamSituation_m_pouncedFriend },
		{ "SurvivorTeamSituation::m_pummeledFriend", m_iSurvivorTeamSituation_m_pummeledFriend },
		{ "SurvivorCollectObject::m_useObject", m_iSurvivorCollectObject_m_useObject },
		{ "CTerrorPlayer::m_timeSinceAttackedByEnemyTimer", m_iCTerrorPlayer_m_timeSinceAttackedByEnemyTimer },
		{ "CDirector::m_iTankCount", m_iCDirector_m_iTankCount },
	};

	for (auto&& el : s_offsets) 
	{
		if (!pGameConfig->GetOffset(el.key, &el.offset)) 
		{
			ke::SafeSprintf(error, maxlen, "Unable to get offset for \"%s\"", el.key);

			gameconfs->CloseGameConfigFile(pGameConfig);

			return false;
		}
	}

	CDetourManager::Init(smutils->GetScriptingEngine(), pGameConfig);

	m_pDetour_SurvivorCollectObject_ShouldGiveUp = DETOUR_CREATE_MEMBER(DetourFunc_SurvivorCollectObject_ShouldGiveUp, "SurvivorCollectObject::ShouldGiveUp");

	if (m_pDetour_SurvivorCollectObject_ShouldGiveUp)
	{
		m_pDetour_SurvivorCollectObject_ShouldGiveUp->EnableDetour();
	}
	else
	{
		ke::SafeStrcpy(error, maxlen, "Failed to initialize detour SurvivorCollectObject::ShouldGiveUp");

		gameconfs->CloseGameConfigFile(pGameConfig);

		return false;
	}

	gameconfs->CloseGameConfigFile(pGameConfig);

	sharesys->AddDependency(myself, "bintools.ext", true, true);
	
	return true;
}

void CSurvivorObjectCollectionExt::SDK_OnUnload()
{
	if (m_pDetour_SurvivorCollectObject_ShouldGiveUp)
	{
		m_pDetour_SurvivorCollectObject_ShouldGiveUp->Destroy();
		m_pDetour_SurvivorCollectObject_ShouldGiveUp = NULL;
	}

	if (m_pCallWrap_CBaseCombatCharacter_Weapon_GetSlot)
	{
		m_pCallWrap_CBaseCombatCharacter_Weapon_GetSlot->Destroy();
		m_pCallWrap_CBaseCombatCharacter_Weapon_GetSlot = NULL;
	}

	if (m_pCallWrap_SurvivorBot_GetTeamSituation)
	{
		m_pCallWrap_SurvivorBot_GetTeamSituation->Destroy();
		m_pCallWrap_SurvivorBot_GetTeamSituation = NULL;
	}
}

void CSurvivorObjectCollectionExt::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, m_pBinTools);

	PassInfo passInfo_CBaseCombatCharacter_Weapon_GetSlot[] =
	{
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(int), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(CBaseEntity *), NULL, 0 }
	};

	PassInfo passInfo_SurvivorUseObject_ShouldGiveUp[] =
	{
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(CBaseEntity *), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(bool), NULL, 0 }
	};

	PassInfo passInfo_SurvivorBot_GetTeamSituation =
	{
		PassType_Basic, PASSFLAG_BYVAL, sizeof(SurvivorTeamSituation *), NULL, 0
	};

	m_pCallWrap_CBaseCombatCharacter_Weapon_GetSlot = m_pBinTools->CreateVCall(m_iVtblIndex_CBaseCombatCharacter_Weapon_GetSlot, 0, 0, 
		&passInfo_CBaseCombatCharacter_Weapon_GetSlot[1], &passInfo_CBaseCombatCharacter_Weapon_GetSlot[0], 1);

	if (!m_pCallWrap_CBaseCombatCharacter_Weapon_GetSlot)
	{
		smutils->LogError(myself, "Unable to create ICallWrapper for \"CBaseCombatCharacter::Weapon_GetSlot\"");
	}

	m_pCallWrap_SurvivorBot_GetTeamSituation = m_pBinTools->CreateCall(m_pfn_SurvivorBot_GetTeamSituation, CallConv_ThisCall, &passInfo_SurvivorBot_GetTeamSituation, NULL, 0);

	if (!m_pCallWrap_SurvivorBot_GetTeamSituation)
	{
		smutils->LogError(myself, "Unable to create ICallWrapper for \"SurvivorBot::GetTeamSituation\"");
	}

	m_pCallWrap_SurvivorUseObject_ShouldGiveUp = m_pBinTools->CreateCall(m_pfn_SurvivorUseObject_ShouldGiveUp, CallConv_ThisCall, 
		&passInfo_SurvivorUseObject_ShouldGiveUp[1], &passInfo_SurvivorUseObject_ShouldGiveUp[0], 1);

	if (!m_pCallWrap_SurvivorUseObject_ShouldGiveUp)
	{
		smutils->LogError(myself, "Unable to create ICallWrapper for \"SurvivorUseObject::ShouldGiveUp\"");
	}
}

bool CSurvivorObjectCollectionExt::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);

	gpGlobals = ismm->GetCGlobals();

	return true;
}

bool CSurvivorObjectCollectionExt::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, m_pBinTools);

	return true;
}

bool CSurvivorObjectCollectionExt::QueryInterfaceDrop(SMInterface *pInterface)
{
	if (pInterface == m_pBinTools)
	{
		return false;
	}

	return true;
}

void CSurvivorObjectCollectionExt::NotifyInterfaceDrop(SMInterface *pInterface)
{
	SDK_OnUnload();
}

float CSurvivorObjectCollectionExt::GetTimeSinceAttackedByEnemy(CBaseEntity *pPlayer)
{
	float timestamp = *reinterpret_cast<float *>(reinterpret_cast<byte *>(pPlayer) + m_iCTerrorPlayer_m_timeSinceAttackedByEnemyTimer + 4);

	if (timestamp > 0.0f)
	{
		return (gpGlobals->curtime - timestamp);
	}

	return 99999.898f;
}

int CSurvivorObjectCollectionExt::GetPrimaryAmmoType(CBaseEntity *pWeapon)
{
	static int m_iPrimaryAmmoType = -1;

	if (m_iPrimaryAmmoType == -1)
	{
		datamap_t *pMap = gamehelpers->GetDataMap(pWeapon);

		sm_datatable_info_t info;
		if (gamehelpers->FindDataMapInfo(pMap, "m_iPrimaryAmmoType", &info))
		{
			m_iPrimaryAmmoType = info.actual_offset;
		}
	}

	return *reinterpret_cast<int *>(reinterpret_cast<byte *>(pWeapon) + m_iPrimaryAmmoType);
}

CBaseEntity *CSurvivorObjectCollectionExt::Weapon_GetSlot(CBaseEntity *pPlayer, int slot)
{
	struct 
	{
		CBaseEntity *pPlayer;
		int slot;
	} stack = 
	{
		pPlayer,
		slot
	};

	CBaseEntity *result = NULL;
	m_pCallWrap_CBaseCombatCharacter_Weapon_GetSlot->Execute(&stack, &result);

	return result;
}

SurvivorTeamSituation *CSurvivorObjectCollectionExt::GetTeamSituation(CBaseEntity *pBot)
{
	SurvivorTeamSituation *result = NULL;
	m_pCallWrap_SurvivorBot_GetTeamSituation->Execute(&pBot, &result);

	return result;
}

bool CSurvivorObjectCollectionExt::ShouldGiveUp(Action<CBaseEntity> *pAction, CBaseEntity *pBot)
{
	struct 
	{
		Action<CBaseEntity> *pAction;
		CBaseEntity *pBot;
	} stack = 
	{
		pAction,
		pBot
	};

	bool result;
	m_pCallWrap_SurvivorUseObject_ShouldGiveUp->Execute(&stack, &result);

	return result;
}
