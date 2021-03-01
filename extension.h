#ifndef _INCLUDE_SURVIVOR_OBJECT_COLLECTION_EXTENSION_PROPER_H_
#define _INCLUDE_SURVIVOR_OBJECT_COLLECTION_EXTENSION_PROPER_H_
 
#include "smsdk_ext.h"
#include <IBinTools.h>

#define WEAPON_SLOT_RIFLE		0	// (primary slot)

class CDetour;
class SurvivorTeamSituation;

// forward declaration
template < typename Actor > class Action;

inline CBaseEntity *EntityFromBaseHandle(void *pAddress, int offset)
{
	CBaseHandle &hndl = *reinterpret_cast<CBaseHandle *>(reinterpret_cast<byte *>(pAddress) + offset);
	CBaseEntity *pHandleEntity = gamehelpers->ReferenceToEntity(hndl.GetEntryIndex());

	if (!pHandleEntity || hndl != reinterpret_cast<IHandleEntity *>(pHandleEntity)->GetRefEHandle())
	{
		return NULL;
	}
	
	return pHandleEntity;
}

class CSurvivorObjectCollectionExt : public SDKExtension
{
public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlen	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);

	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	virtual void SDK_OnAllLoaded();
#ifdef SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);
#endif
	/**
	 * @brief Return false to tell Core that your extension should be considered unusable.
	 *
	 * @param error				Error buffer.
	 * @param maxlength			Size of error buffer.
	 * @return					True on success, false otherwise.
	 */
	virtual bool QueryRunning(char *error, size_t maxlen);

	/**
	 * @brief Asks the extension whether it's safe to remove an external 
	 * interface it's using.  If it's not safe, return false, and the 
	 * extension will be unloaded afterwards.
	 *
	 * NOTE: It is important to also hook NotifyInterfaceDrop() in order to clean 
	 * up resources.
	 *
	 * @param pInterface		Pointer to interface being dropped.  This 
	 * 							pointer may be opaque, and it should not 
	 *							be queried using SMInterface functions unless 
	 *							it can be verified to match an existing 
	 *							pointer of known type.
	 * @return					True to continue, false to unload this 
	 * 							extension afterwards.
	 */
	virtual bool QueryInterfaceDrop(SMInterface *pInterface);

	/**
	 * @brief Notifies the extension that an external interface it uses is being removed.
	 *
	 * @param pInterface		Pointer to interface being dropped.  This
	 * 							pointer may be opaque, and it should not 
	 *							be queried using SMInterface functions unless 
	 *							it can be verified to match an existing 
	 */
	virtual void NotifyInterfaceDrop(SMInterface *pInterface);

	int GetAmmoCount(CBaseEntity *pPlayer, int iAmmo) { return *reinterpret_cast<int *>(reinterpret_cast<byte *>(pPlayer) + m_iSendProp_CTerrorPlayer_m_iAmmo + (iAmmo * 4)); }
	int GetPrimaryAmmoType(CBaseEntity *pWeapon);
	int GetMaxCarry(const char *pWeaponName);

	int GetTankCount() { return *reinterpret_cast<int *>(reinterpret_cast<byte *>(m_pObj_CDirector_TheDirector) + m_iCDirector_m_iTankCount); }

	float GetTimeSinceAttackedByEnemy(CBaseEntity *pPlayer);

	CBaseEntity *GetFriendIntrouble(SurvivorTeamSituation *pSituation) { return EntityFromBaseHandle(pSituation, m_iSurvivorTeamSituation_m_friendInTrouble); }
	CBaseEntity *GetTonguedFriend(SurvivorTeamSituation *pSituation) { return EntityFromBaseHandle(pSituation, m_iSurvivorTeamSituation_m_tonguedFriend); }
	CBaseEntity *GetPouncedFriend(SurvivorTeamSituation *pSituation) { return EntityFromBaseHandle(pSituation, m_iSurvivorTeamSituation_m_pouncedFriend); }
	CBaseEntity *GetPummeledFriend(SurvivorTeamSituation *pSituation) { return EntityFromBaseHandle(pSituation, m_iSurvivorTeamSituation_m_pummeledFriend); }

	CBaseEntity *GetUseObject(Action<CBaseEntity> *pAction) { return EntityFromBaseHandle(pAction, m_iSurvivorCollectObject_m_useObject); }

	CBaseEntity *Weapon_GetSlot(CBaseEntity *pPlayer, int slot);

	SurvivorTeamSituation *GetTeamSituation(CBaseEntity *pBot);

 	bool ShouldGiveUp(Action<CBaseEntity> *pAction, CBaseEntity *pBot);

private:
 	IBinTools *m_pBinTools = NULL;

	int m_iSendProp_CTerrorPlayer_m_iAmmo = -1;

	int m_iSurvivorTeamSituation_m_friendInTrouble = -1;
	int m_iSurvivorTeamSituation_m_tonguedFriend = -1;
	int m_iSurvivorTeamSituation_m_pouncedFriend = -1;
	int m_iSurvivorTeamSituation_m_pummeledFriend = -1;
	int m_iSurvivorCollectObject_m_useObject = -1;
	int m_iCTerrorPlayer_m_timeSinceAttackedByEnemyTimer = -1;
	int m_iCDirector_m_iTankCount = -1;

	int m_iVtblIndex_CBaseCombatCharacter_Weapon_GetSlot = -1;

	CDetour *m_pDetour_SurvivorCollectObject_ShouldGiveUp = NULL;

	void *m_pfn_SurvivorBot_GetTeamSituation = NULL;
	void *m_pfn_SurvivorUseObject_ShouldGiveUp = NULL;

	void *m_pObj_CDirector_TheDirector = NULL;

	ICallWrapper *m_pCallWrap_CBaseCombatCharacter_Weapon_GetSlot = NULL;
	ICallWrapper *m_pCallWrap_SurvivorBot_GetTeamSituation = NULL;
	ICallWrapper *m_pCallWrap_SurvivorUseObject_ShouldGiveUp = NULL;
};

#endif // _INCLUDE_SURVIVOR_OBJECT_COLLECTION_EXTENSION_PROPER_H_
