
#include "playerbot/playerbot.h"
#include "MoveToRpgTargetAction.h"
#include "ChooseRpgTargetAction.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/ServerFacade.h"
#include "playerbot/strategy/values/PossibleRpgTargetsValue.h"
#include "playerbot/TravelMgr.h"

using namespace ai;

bool MoveToRpgTargetAction::Execute(Event& event)
{
    GuidPosition guidP = AI_VALUE(GuidPosition, "rpg target");
    Unit* unit = ai->GetUnit(guidP);
    GameObject* go = ai->GetGameObject(guidP);
    Player* player = guidP.GetPlayer();

    WorldObject* wo;
    if (unit)
    {
        wo = unit;
    }
    else if(go)
        wo = go;
    else
        return false;

    if (guidP.IsPlayer())
    {
        Player* player = guidP.GetPlayer();

        if (player && player->GetPlayerbotAI())
        {
            GuidPosition guidPP = PAI_VALUE(GuidPosition, "rpg target");

            if (guidPP.IsPlayer())
            {
                AI_VALUE(std::set<ObjectGuid>&,"ignore rpg target").insert(AI_VALUE(GuidPosition, "rpg target"));

                RESET_AI_VALUE(GuidPosition, "rpg target");

                if (ai->HasStrategy("debug rpg", BotState::BOT_STATE_NON_COMBAT))
                {
                    ai->TellPlayerNoFacing(GetMaster(), "玩家目标正在瞄准我.取消目标");
                }
                return false;
            }
        }
    }

    if (unit && unit->IsMoving() && !urand(0, 20) && guidP.sqDistance2d(bot) < INTERACTION_DISTANCE * INTERACTION_DISTANCE * 2)
    {
        AI_VALUE(std::set<ObjectGuid>&,"ignore rpg target").insert(AI_VALUE(GuidPosition, "rpg target"));

        RESET_AI_VALUE(GuidPosition,"rpg target");

        if (ai->HasStrategy("debug rpg", BotState::BOT_STATE_NON_COMBAT))
        {
            ai->TellPlayerNoFacing(GetMaster(), "目标正在移动.随机取消目标.");
        }
        return false;
    }

    if (!AI_VALUE2(bool, "can free move to", GuidPosition(wo).to_string()))
    {
        AI_VALUE(std::set<ObjectGuid>&, "ignore rpg target").insert(AI_VALUE(GuidPosition, "rpg target"));

        RESET_AI_VALUE(GuidPosition, "rpg target");

        if (ai->HasStrategy("debug rpg", BotState::BOT_STATE_NON_COMBAT))
        {
            ai->TellPlayerNoFacing(GetMaster(), "目标远离主人.随机取消目标.");
        }
        return false;
    }

    if (guidP.distance(bot) > sPlayerbotAIConfig.reactDistance * 2)
    {
        AI_VALUE(std::set<ObjectGuid>&, "ignore rpg target").insert(AI_VALUE(GuidPosition, "rpg target"));

        RESET_AI_VALUE(GuidPosition, "rpg target");

        if (ai->HasStrategy("debug rpg", BotState::BOT_STATE_NON_COMBAT))
        {
            ai->TellPlayerNoFacing(GetMaster(), "目标超出反应距离.取消目标");
        }
        return false;
    }

    if (guidP.IsGameObject() && guidP.sqDistance2d(bot) < INTERACTION_DISTANCE * INTERACTION_DISTANCE && guidP.distance(bot) > INTERACTION_DISTANCE * 1.5 && !urand(0, 5))
    {
        AI_VALUE(std::set<ObjectGuid>&, "ignore rpg target").insert(AI_VALUE(GuidPosition, "rpg target"));

        RESET_AI_VALUE(GuidPosition, "rpg target");

        if (ai->HasStrategy("debug rpg", BotState::BOT_STATE_NON_COMBAT))
        {
            ai->TellPlayerNoFacing(GetMaster(), "在对象上方/下方取消Rpg目标");
        }
        return false;
    }

    if (!urand(0, 50))
    {
        AI_VALUE(std::set<ObjectGuid>&, "ignore rpg target").insert(AI_VALUE(GuidPosition, "rpg target"));

        RESET_AI_VALUE(GuidPosition, "rpg target");

        if (ai->HasStrategy("debug rpg", BotState::BOT_STATE_NON_COMBAT))
        {
            ai->TellPlayerNoFacing(GetMaster(), "随机取消Rpg目标");
        }
        return false;
    }

    float x = wo->GetPositionX();
    float y = wo->GetPositionY();
    float z = wo->GetPositionZ();
    float mapId = wo->GetMapId();

    if (ai->HasStrategy("debug move", BotState::BOT_STATE_NON_COMBAT))
    {
        std::string name = chat->formatWorldobject(wo);

        ai->Poi(x, y, name);
    }
	
	if (sPlayerbotAIConfig.RandombotsWalkingRPG)
        if (!bot->GetTerrain()->IsOutdoors(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ()))
            bot->m_movementInfo.AddMovementFlag(MOVEFLAG_WALK_MODE);

    float angle;
    float distance = 1.0f;
    
    if (bot->IsWithinLOS(x, y, z, true))
    {
        if (!unit || !unit->IsMoving())
            angle = wo->GetAngle(bot) + (M_PI * irand(-25, 25) / 100.0); //Closest 45 degrees towards the target
        else if (!unit->HasInArc(bot))
            angle = wo->GetOrientation() + (M_PI * irand(-10, 10) / 100.0); //20 degrees infront of target (leading it's movement)
        else
            angle = wo->GetAngle(bot); //Current approuch angle.

        if (guidP.sqDistance2d(bot) < INTERACTION_DISTANCE * INTERACTION_DISTANCE)
            distance = sqrt(guidP.sqDistance2d(bot)); //Stay at this distance.
        else if(unit || !urand(0, 5)) //Stay futher away from npc's and sometimes gameobjects (for large hitbox objects).
            distance = frand(0.5, 1);
        else
            distance = frand(0, 0.5);
    }
    else
        angle = 2 * M_PI * urand(0, 100) / 100.0; //A circle around the target.

    x += cos(angle) * INTERACTION_DISTANCE * distance;
    y += sin(angle) * INTERACTION_DISTANCE * distance;
    
    //WaitForReach(distance);

    bool couldMove;

    if (false && unit && unit->IsMoving() && bot->GetDistance(unit) < INTERACTION_DISTANCE  * 2 && unit->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
        couldMove = Follow(unit, INTERACTION_DISTANCE * distance, unit->GetOrientation());
    else    
        couldMove = MoveTo(mapId, x, y, z, false, false);

    if (!couldMove && WorldPosition(mapId,x,y,z).distance(bot) > INTERACTION_DISTANCE)
    {
        AI_VALUE(std::set<ObjectGuid>&,"ignore rpg target").insert(AI_VALUE(GuidPosition, "rpg target"));

        RESET_AI_VALUE(GuidPosition, "rpg target");

        if (ai->HasStrategy("debug rpg", BotState::BOT_STATE_NON_COMBAT))
        {
            ai->TellPlayerNoFacing(GetMaster(), "无法移动到Rpg目标.取消Rpg目标");
        }

        return false;
    }

    if (ai->HasStrategy("debug rpg", BotState::BOT_STATE_NON_COMBAT) && guidP.GetWorldObject())
    {
        if (couldMove)
        {
            std::ostringstream out;
            out << "前往: ";
            out << chat->formatWorldobject(guidP.GetWorldObject());
            ai->TellPlayerNoFacing(GetMaster(), out);
        }
        else
        {
            std::ostringstream out;
            out << "靠近: ";
            out << chat->formatWorldobject(guidP.GetWorldObject());
            ai->TellPlayerNoFacing(GetMaster(), out);
        }
    }

    return couldMove;
}

bool MoveToRpgTargetAction::isUseful()
{
    GuidPosition guidP = AI_VALUE(GuidPosition, "rpg target"), p=guidP;

    if (!guidP)
        return false;

    WorldObject* wo = guidP.GetWorldObject();

    if (!wo)
    {
        RESET_AI_VALUE(GuidPosition, "rpg target");

        if (ai->HasStrategy("debug rpg", BotState::BOT_STATE_NON_COMBAT))
        {
            ai->TellPlayerNoFacing(GetMaster(), "无法找到目标.取消Rpg目标");
        }
    }

    if(MEM_AI_VALUE(WorldPosition, "current position")->LastChangeDelay() < 60)
        if (bot->IsMoving() && bot->GetMotionMaster() && bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
            return false;

    TravelTarget* travelTarget = AI_VALUE(TravelTarget*, "travel target");

    if (travelTarget->isTraveling() && AI_VALUE2(bool, "can free move to", travelTarget->GetPosStr()))
        return false;

    guidP.updatePosition();

    if(WorldPosition(p) != WorldPosition(guidP))
        SET_AI_VALUE(GuidPosition, "rpg target", guidP);

    if (guidP.distance(bot) < INTERACTION_DISTANCE)
        return false;

    if (!AI_VALUE(bool, "can move around"))
        return false;

    return true;
}


