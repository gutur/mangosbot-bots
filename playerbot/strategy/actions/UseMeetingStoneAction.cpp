#include "botpch.h"
#include "../../playerbot.h"
#include "UseMeetingStoneAction.h"
#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

#include "../values/PositionValue.h"
#include "Entities/Transports.h"

using namespace MaNGOS;

bool UseMeetingStoneAction::Execute(Event& event)
{
    Player* master = GetMaster();
    if (!master)
        return false;

    WorldPacket p(event.getPacket());
    p.rpos(0);
    ObjectGuid guid;
    p >> guid;

	if (master->GetSelectionGuid() && master->GetSelectionGuid() != bot->GetObjectGuid())
		return false;

	if (!master->GetSelectionGuid() && master->GetGroup() != bot->GetGroup())
		return false;

    if (master->IsBeingTeleported())
        return false;

    if (sServerFacade.IsInCombat(bot))
    {
        ai->TellError("我正在战斗中");
        return false;
    }

    Map* map = master->GetMap();
    if (!map)
        return false;

    GameObject *gameObject = map->GetGameObject(guid);
    if (!gameObject)
        return false;

	const GameObjectInfo* goInfo = gameObject->GetGOInfo();
	if (!goInfo || goInfo->type != GAMEOBJECT_TYPE_SUMMONING_RITUAL)
        return false;

    return Teleport(master, bot);
}

class AnyGameObjectInObjectRangeCheck
{
public:
    AnyGameObjectInObjectRangeCheck(WorldObject const* obj, float range) : i_obj(obj), i_range(range) {}
    WorldObject const& GetFocusObject() const { return *i_obj; }
    bool operator()(GameObject* u)
    {
        if (u && i_obj->IsWithinDistInMap(u, i_range) && sServerFacade.isSpawned(u) && u->GetGOInfo())
            return true;

        return false;
    }

private:
    WorldObject const* i_obj;
    float i_range;
};


bool SummonAction::Execute(Event& event)
{
    Player* master = GetMaster();
    if (!master || master->IsBeingTeleported())
        return false;

    if (master->GetSession()->GetSecurity() > SEC_PLAYER || sPlayerbotAIConfig.nonGmFreeSummon)
        return Teleport(master, bot);

    if(bot->GetMapId() == master->GetMapId() && !WorldPosition(bot).canPathTo(master,bot) && bot->GetDistance(master) < sPlayerbotAIConfig.sightDistance) //We can't walk to master so fine to short-range teleport.
        return Teleport(master, bot);

    if (bot->IsTaxiFlying())
        return false;

    if (SummonUsingGos(master, bot) || SummonUsingNpcs(master, bot))
    {
        ai->TellPlayerNoFacing(GetMaster(), BOT_TEXT("hello"));
        return true;
    }

    if (SummonUsingGos(bot, master) || SummonUsingNpcs(bot, master))
    {
        ai->TellPlayerNoFacing(GetMaster(), "欢迎!");
        return true;
    }

    return false;
}

bool SummonAction::SummonUsingGos(Player *summoner, Player *player)
{
    list<GameObject*> targets;
    AnyGameObjectInObjectRangeCheck u_check(summoner, sPlayerbotAIConfig.sightDistance);
    GameObjectListSearcher<AnyGameObjectInObjectRangeCheck> searcher(targets, u_check);
    Cell::VisitAllObjects((const WorldObject*)summoner, searcher, sPlayerbotAIConfig.sightDistance);

    for(list<GameObject*>::iterator tIter = targets.begin(); tIter != targets.end(); ++tIter)
    {
        GameObject* go = *tIter;
        if (go && sServerFacade.isSpawned(go) && go->GetGoType() == GAMEOBJECT_TYPE_MEETINGSTONE)
            return Teleport(summoner, player);
    }

    ai->TellError(summoner == bot ? "附近没有集合石." : "附近没有集合石.");
    return false;
}

bool SummonAction::SummonUsingNpcs(Player *summoner, Player *player)
{
    if (!sPlayerbotAIConfig.summonAtInnkeepersEnabled)
        return false;

    list<Unit*> targets;
    AnyUnitInObjectRangeCheck u_check(summoner, sPlayerbotAIConfig.sightDistance);
    UnitListSearcher<AnyUnitInObjectRangeCheck> searcher(targets, u_check);
    Cell::VisitAllObjects(summoner, searcher, sPlayerbotAIConfig.sightDistance);
    for(list<Unit*>::iterator tIter = targets.begin(); tIter != targets.end(); ++tIter)
    {
        Unit* unit = *tIter;
        if (unit && unit->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_INNKEEPER))
        {
            if (!player->HasItemCount(6948, 1, false))
            {
                ai->TellError(player == bot ? "附近没有集合石." : "你没有炉石.");
                return false;
            }

            if (!sServerFacade.IsSpellReady(player, 8690))
            {
                ai->TellError(player == bot ? "我的炉石还未准备好." : "你的炉石还未准备好.");
                return false;
            }

            // Trigger cooldown
            SpellEntry const* spellInfo = sServerFacade.LookupSpellInfo(8690);
            if (!spellInfo)
                return false;
            Spell spell(player, spellInfo,
#ifdef MANGOS
                    0
#endif
#ifdef CMANGOS
                    TRIGGERED_OLD_TRIGGERED
#endif
                    );
            spell.SendSpellCooldown();

            return Teleport(summoner, player);
        }
    }

    ai->TellError(summoner == bot ? "附近没有旅店老板" : "你附近没有旅店老板");
    return false;
}

bool SummonAction::Teleport(Player *summoner, Player *player)
{
    Player* master = GetMaster();
    if (!summoner->IsBeingTeleported() && !player->IsBeingTeleported() && summoner != player)
    {
        float followAngle = GetFollowAngle();
        for (double angle = followAngle - M_PI; angle <= followAngle + M_PI; angle += M_PI / 4)
        {
            uint32 mapId = summoner->GetMapId();
            float x = summoner->GetPositionX() + cos(angle) * ai->GetRange("follow");
            float y = summoner->GetPositionY() + sin(angle) * ai->GetRange("follow");
            float z = summoner->GetPositionZ();
            summoner->UpdateGroundPositionZ(x, y, z);
            if (!summoner->IsWithinLOS(x, y, z + player->GetCollisionHeight(), true))
            {
                x = summoner->GetPositionX();
                y = summoner->GetPositionY();
                z = summoner->GetPositionZ();
            }
            if (summoner->IsWithinLOS(x, y, z + player->GetCollisionHeight(), true))
            {
                if (sServerFacade.UnitIsDead(player) && sServerFacade.IsAlive(summoner))
                {
                    player->ResurrectPlayer(1.0f, false);
                    player->SpawnCorpseBones();
                    ai->TellPlayerNoFacing(GetMaster(), "我又复活了!!!");
                }                

                if (player->IsTaxiFlying())
                {
                    player->TaxiFlightInterrupt();
                    player->GetMotionMaster()->MovementExpired();
                }
                player->GetMotionMaster()->Clear();
                player->TeleportTo(mapId, x, y, z, 0);
                player->SendHeartBeat();
                if (summoner->GetTransport())
                    summoner->GetTransport()->AddPassenger(player, false);
                    
                if(ai->HasStrategy("stay", BotState::BOT_STATE_NON_COMBAT))
                    SET_AI_VALUE2(PositionEntry, "pos", "stay", PositionEntry(x, y, z, mapId));
                if (ai->HasStrategy("guard", BotState::BOT_STATE_NON_COMBAT))
                    SET_AI_VALUE2(PositionEntry, "pos", "guard", PositionEntry(x, y, z, mapId));

                return true;
            }
        }
    }

    ai->TellError("地方太小了,不够召唤");
    return false;
}

bool AcceptSummonAction::Execute(Event& event)
{
    WorldPacket p(event.getPacket());
    p.rpos(0);
    ObjectGuid summonerGuid;
    p >> summonerGuid;

    WorldPacket response(CMSG_SUMMON_RESPONSE);
    response << summonerGuid;
#if defined(MANGOSBOT_ONE) || defined(MANGOSBOT_TWO)
    response << uint8(1);
#endif
    bot->GetSession()->HandleSummonResponseOpcode(response);
    
    return true;
}
