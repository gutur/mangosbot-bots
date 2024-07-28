
#include "playerbot/playerbot.h"
#include "AttackAction.h"
#include "MotionGenerators/MovementGenerator.h"
#include "AI/BaseAI/CreatureAI.h"
#include "playerbot/LootObjectStack.h"
#include "playerbot/ServerFacade.h"
#include "playerbot/strategy/generic/CombatStrategy.h"

using namespace ai;

bool AttackAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();

    Unit* target = GetTarget();
    if (target && target->IsInWorld() && target->GetMapId() == bot->GetMapId())
    {
        return Attack(requester, target);
    }

    return false;
}

bool AttackMyTargetAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    if(requester)
    {
        const ObjectGuid guid = requester->GetSelectionGuid();
        if (guid)
        {
            if (Attack(requester, ai->GetUnit(guid)))
            {
                SET_AI_VALUE(ObjectGuid, "attack target", guid);
                return true;
            }
        }
        else if (verbose)
        {
            ai->TellError(requester, "你没有目标");
        }
    }

    return false;
}

bool AttackRTITargetAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    Unit* rtiTarget = AI_VALUE(Unit*, "rti target");

    if (rtiTarget && rtiTarget->IsInWorld() && rtiTarget->GetMapId() == bot->GetMapId())
    {
        if (Attack(requester, rtiTarget))
        {
            SET_AI_VALUE(ObjectGuid, "attack target", rtiTarget->GetObjectGuid());
            return true;
        }
    }
    else
    {
        ai->TellError(requester, "我没有看到团队目标");
    }

    return false;
}

bool AttackMyTargetAction::isUseful()
{
    return !ai->ContainsStrategy(STRATEGY_TYPE_HEAL) || ai->HasStrategy("offdps", BotState::BOT_STATE_COMBAT);
}

bool AttackRTITargetAction::isUseful()
{
    return !ai->ContainsStrategy(STRATEGY_TYPE_HEAL) || ai->HasStrategy("offdps", BotState::BOT_STATE_COMBAT);
}

bool AttackAction::Attack(Player* requester, Unit* target)
{
    MotionMaster &mm = *bot->GetMotionMaster();
	if (mm.GetCurrentMovementGeneratorType() == TAXI_MOTION_TYPE || (bot->IsFlying() && WorldPosition(bot).currentHeight() > 10.0f))
    {
        if (verbose)
        {
            ai->TellPlayerNoFacing(requester, "我在飞行中不能攻击");
        }

        return false;
    }

    if (IsTargetValid(requester, target))
    {
        if (bot->IsMounted() && (sServerFacade.GetDistance2d(bot, target) < 40.0f || bot->IsFlying()))
        {
            ai->Unmount();
            
            if (bot->IsFlying())
            {
                return true;
            }
        }

        ObjectGuid guid = target->GetObjectGuid();
        bot->SetSelectionGuid(target->GetObjectGuid());

        Unit* oldTarget = AI_VALUE(Unit*, "current target");
        if(oldTarget)
        {
            SET_AI_VALUE(Unit*, "old target", oldTarget);
        }

        SET_AI_VALUE(Unit*, "current target", target);
        AI_VALUE(LootObjectStack*, "available loot")->Add(guid);

        const bool isWaitingForAttack = WaitForAttackStrategy::ShouldWait(ai);
        Pet* pet = bot->GetPet();
        if (pet)
        {
            UnitAI* creatureAI = ((Creature*)pet)->AI();
            if (creatureAI)
            {
                // Don't send the pet to attack if the bot is waiting for attack
                if (!isWaitingForAttack && (!ai->HasStrategy("stay", BotState::BOT_STATE_COMBAT) || AI_VALUE2(float, "distance", "current target") < ai->GetRange("spell")))
                {
                    // Reset the pet state if no master
                    if (creatureAI->GetReactState() == REACT_PASSIVE && !ai->GetMaster())
                    {
                        creatureAI->SetReactState(REACT_DEFENSIVE);
                    }

                    // Don't send the pet to attack if set to passive
                    if (creatureAI->GetReactState() != REACT_PASSIVE)
                    {
                        creatureAI->AttackStart(target);
                    }
                }
            }
        }

        if (IsMovingAllowed() && !sServerFacade.IsInFront(bot, target, sPlayerbotAIConfig.sightDistance, CAST_ANGLE_IN_FRONT))
        {
            sServerFacade.SetFacingTo(bot, target);
        }

        bool result = true;

        // Don't attack target if it is waiting for attack or in stealth
        if (!ai->HasStrategy("stealthed", BotState::BOT_STATE_COMBAT) && !isWaitingForAttack)
        {
            ai->PlayAttackEmote(1);
            result = bot->Attack(target, !ai->IsRanged(bot) || (sServerFacade.GetDistance2d(bot, target) < 5.0f));
        }

        if (result)
        {
            // Force change combat state to have a faster reaction time
            ai->OnCombatStarted();
        }

        return result;
    }

    return false;
}

bool AttackAction::IsTargetValid(Player* requester, Unit* target)
{
    if (!target)
    {
        if (verbose) 
        {
            ai->TellPlayerNoFacing(requester, "我没有目标");
        }

        return false;
    }
    else if (sServerFacade.IsFriendlyTo(bot, target))
    {
        if (verbose)
        {
            std::ostringstream msg;
            msg << target->GetName();
            msg << " 是友善目标";
            ai->TellPlayerNoFacing(requester, msg.str());
        }

        return false;
    }
    else if (sServerFacade.UnitIsDead(target))
    {
        if (verbose)
        {
            std::ostringstream msg;
            msg << target->GetName();
            msg << " 已经死了";
            ai->TellPlayerNoFacing(requester, msg.str());
        }

        return false;
    }
    else if (sServerFacade.GetDistance2d(bot, target) > sPlayerbotAIConfig.sightDistance)
    {
        if (verbose)
        {
            std::ostringstream msg;
            msg << target->GetName();
            msg << " 太远了";
            ai->TellPlayerNoFacing(requester, msg.str());
        }

        return false;
    }

    return true;
}

bool AttackDuelOpponentAction::isUseful()
{
    return AI_VALUE(Unit*, "duel target");
}

bool AttackDuelOpponentAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    return Attack(requester, AI_VALUE(Unit*, "duel target"));
}
