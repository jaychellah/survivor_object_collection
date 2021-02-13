#ifndef _INCLUDE_SURVIVOR_OBJECT_COLLECTION_EXTENSION_PROPER_H_
#define _INCLUDE_SURVIVOR_OBJECT_COLLECTION_EXTENSION_PROPER_H_
 
#include "smsdk_ext.h"
#include <IBinTools.h>

#define WEAPON_SLOT_RIFLE		0	// (primary slot)

class CDetour;
class SurvivorTeamSituation;

// forward declaration
template < typename Actor > class Action;

inline CBaseEntity *EntityFromBaseHandle(void *addr, int offset)
{
    CBaseHandle &hndl = *reinterpret_cast<CBaseHandle *>(reinterpret_cast<char *>(addr) + offset);
    CBaseEntity *handleEntity = gamehelpers->ReferenceToEntity(hndl.GetEntryIndex());

    if (!handleEntity || hndl != reinterpret_cast<IHandleEntity *>(handleEntity)->GetRefEHandle())
    {
        return NULL;
    }
	
    return handleEntity;
}

class CSurvivorObjectCollectionExt : public SDKExtension
{
public:
    virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);
    virtual void SDK_OnUnload();
    virtual void SDK_OnAllLoaded();
    virtual bool QueryRunning(char *error, size_t maxlen);
#ifdef SMEXT_CONF_METAMOD
    virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);
#endif
    int GetAmmoCount(CBaseEntity *player, int ammoIndex) { return *reinterpret_cast<int *>(reinterpret_cast<char *>(player) + sendprop_m_iAmmo + (ammoIndex * 4)); }
    int GetPrimaryAmmoType(CBaseEntity *weapon);
    int GetMaxCarry(const char *wpnName);

    int GetTankCount() { return *reinterpret_cast<int *>(reinterpret_cast<char *>(addr_TheDirector) + offset_CDirector_m_iTankCount); }

    float GetTimeSinceAttackedByEnemy(CBaseEntity *player);

    CBaseEntity *GetFriendIntrouble(SurvivorTeamSituation *situation) { return EntityFromBaseHandle(situation, offset_SurvivorTeamSituation_m_friendInTrouble); }
    CBaseEntity *GetTonguedFriend(SurvivorTeamSituation *situation) { return EntityFromBaseHandle(situation, offset_SurvivorTeamSituation_m_tonguedFriend); }
    CBaseEntity *GetPouncedFriend(SurvivorTeamSituation *situation) { return EntityFromBaseHandle(situation, offset_SurvivorTeamSituation_m_pouncedFriend); }
    CBaseEntity *GetPummeledFriend(SurvivorTeamSituation *situation) { return EntityFromBaseHandle(situation, offset_SurvivorTeamSituation_m_pummeledFriend); }

    CBaseEntity *GetUseObject(Action<CBaseEntity> *action) { return EntityFromBaseHandle(action, offset_SurvivorCollectObject_m_useObject); }

    CBaseEntity *Weapon_GetSlot(CBaseEntity *player, int slot);

    SurvivorTeamSituation *GetTeamSituation(CBaseEntity *bot);

    bool ShouldGiveUp(Action<CBaseEntity> *action, CBaseEntity *bot);

private:
    IBinTools *bintools = NULL;

    int sendprop_m_iAmmo = -1;

    int offset_SurvivorTeamSituation_m_friendInTrouble = -1;
    int offset_SurvivorTeamSituation_m_tonguedFriend = -1;
    int offset_SurvivorTeamSituation_m_pouncedFriend = -1;
    int offset_SurvivorTeamSituation_m_pummeledFriend = -1;
    int offset_SurvivorCollectObject_m_useObject = -1;
    int offset_CTerrorPlayer_m_timeSinceAttackedByEnemyTimer = -1;
    int offset_CDirector_m_iTankCount = -1;

    int vtblindex_CBaseCombatCharacter_Weapon_GetSlot = -1;

    CDetour *detour_SurvivorCollectObject_ShouldGiveUp = NULL;

    void *pfn_SurvivorBot_GetTeamSituation = NULL;
    void *pfn_SurvivorUseObject_ShouldGiveUp = NULL;

    void *addr_TheDirector = NULL;

    ICallWrapper *vcall_CBaseCombatCharacter_Weapon_GetSlot = NULL;
    ICallWrapper *call_SurvivorBot_GetTeamSituation = NULL;
    ICallWrapper *call_SurvivorUseObject_ShouldGiveUp = NULL;
};

#endif // _INCLUDE_SURVIVOR_OBJECT_COLLECTION_EXTENSION_PROPER_H_