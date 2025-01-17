#include "botpch.h"
#include "../../playerbot.h"
#include "GrindTargetValue.h"
#include "../../PlayerbotAIConfig.h"
#include "../../RandomPlayerbotMgr.h"
#include "../../ServerFacade.h"
#include "AttackersValue.h"
#include "PossibleAttackTargetsValue.h"
#include "../actions/ChooseTargetActions.h"
#include "Formulas.h"

using namespace ai;

Unit* GrindTargetValue::Calculate()
{
    uint32 memberCount = 1;
    Group* group = bot->GetGroup();
    if (group)
        memberCount = group->GetMembersCount();

    Unit* target = NULL;
    uint32 assistCount = 0;
    while (!target && assistCount < memberCount)
    {
        target = FindTargetForGrinding(assistCount++);
    }

    return target;
}

Unit* GrindTargetValue::FindTargetForGrinding(int assistCount)
{
    uint32 memberCount = 1;
    Group* group = bot->GetGroup();
    Player* master = GetMaster();

    if (master && (master == bot || master->GetMapId() != bot->GetMapId() || master->IsBeingTeleported() || !master->GetPlayerbotAI()))
        master = nullptr;

    list<ObjectGuid> attackers = context->GetValue<list<ObjectGuid>>("possible attack targets")->Get();
    for (list<ObjectGuid>::iterator i = attackers.begin(); i != attackers.end(); i++)
    {
        Unit* unit = ai->GetUnit(*i);
        if (!unit || !sServerFacade.IsAlive(unit))
            continue;

        if (!bot->InBattleGround() && !AI_VALUE2(bool, "can free target", GuidPosition(unit).to_string())) //Do not grind mobs far away from master.
        {
            if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + "(敌对)被忽略(超出可攻击范围).");
            continue;
        }

        if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
            ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) +"(敌对)已选定.");
       
        return unit;
    }

    list<ObjectGuid> targets = *context->GetValue<list<ObjectGuid> >("possible targets");

    if (targets.empty())
        return NULL;

    float distance = 0;
    Unit* result = NULL;

    unordered_map<uint32, bool> needForQuestMap;

    for (list<ObjectGuid>::iterator tIter = targets.begin(); tIter != targets.end(); tIter++)
    {
        Unit* unit = ai->GetUnit(*tIter);
        if (!unit)
            continue;

        if (abs(bot->GetPositionZ() - unit->GetPositionZ()) > sPlayerbotAIConfig.spellDistance)
        {
            if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 被忽略(位置过高/过低).");
            continue;
        }

        if (!bot->InBattleGround() && !AI_VALUE2(bool, "can free target", GuidPosition(unit).to_string())) //Do not grind mobs far away from master.
        {
            if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 被忽略(超出可攻击范围).");
            continue;
        }

        if (!bot->InBattleGround() && master && ai->HasStrategy("follow", BotState::BOT_STATE_NON_COMBAT) && sServerFacade.GetDistance2d(master, unit) > sPlayerbotAIConfig.lootDistance)
        {
            if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 被忽略(距离主人过远).");
            continue;
        }

        if (!bot->InBattleGround() && (int)unit->GetLevel() - (int)bot->GetLevel() > 4 && !unit->GetObjectGuid().IsPlayer())
        {
            if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 被忽略(目标等级比机器人等级高 " + to_string((int)unit->GetLevel() - (int)bot->GetLevel()) + " 级).");
            continue;
        }

        Creature* creature = dynamic_cast<Creature*>(unit);
        if (creature && creature->GetCreatureInfo() && creature->GetCreatureInfo()->Rank > CREATURE_ELITE_NORMAL && !AI_VALUE(bool, "can fight elite"))
        {
            if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 被忽略(当前无法攻击精英怪).");
            continue;
        }

        if (!AttackersValue::IsValid(unit, bot, nullptr, false, false))
        {
            if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 被忽略(是宠物或者正在逃避/不可击杀).");
            continue;
        }

        if (!PossibleAttackTargetsValue::IsPossibleTarget(unit, bot, sPlayerbotAIConfig.sightDistance, false))
        {
            if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 被忽略(已被其他玩家攻击、被控制或超出射程范围).");
            continue;
        }

        if (creature && creature->IsCritter() && urand(0, 10))
        {
            if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 被忽略(忽略小动物).");
            continue;
        }

        float newdistance = sServerFacade.GetDistance2d(bot, unit);

        if (needForQuestMap.find(unit->GetEntry()) == needForQuestMap.end())
            needForQuestMap[unit->GetEntry()] = needForQuest(unit);

        if (!needForQuestMap[unit->GetEntry()])
        {
            if (urand(0, 100) < 99 && AI_VALUE(TravelTarget*, "travel target")->isWorking() && AI_VALUE(TravelTarget*, "travel target")->getDestination()->getName() != "GrindTravelDestination")
            {
                if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                    ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 被忽略(不是当前任务所需目标).");

                continue;
            }
            else if (creature && !MaNGOS::XP::Gain(bot, creature) && urand(0, 50))
            {
                if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                    if ((context->GetValue<TravelTarget*>("travel target")->Get()->isWorking() && context->GetValue<TravelTarget*>("travel target")->Get()->getDestination()->getName() != "GrindTravelDestination"))
                        ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 被忽略(无经验值并且不是当前任务所需目标).");

                continue;
            }
            else if (urand(0, 100) < 75)
            {
                if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                    if ((context->GetValue<TravelTarget*>("travel target")->Get()->isWorking() && context->GetValue<TravelTarget*>("travel target")->Get()->getDestination()->getName() != "GrindTravelDestination"))
                        ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 距离增加(不是当前任务所需目标).");

                newdistance += 20;
            }
        }

        if (!bot->InBattleGround() && GetTargetingPlayerCount(unit) > assistCount)
        {
            if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayer(GetMaster(), chat->formatWorldobject(unit) + " 距离增加(已有" + to_string(GetTargetingPlayerCount(unit)) + " 个机器人正在攻击).");

            newdistance =+ GetTargetingPlayerCount(unit) * 5;
        }

        if (group)
        {
            Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
            for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
            {
                Player* member = sObjectMgr.GetPlayer(itr->guid);
                if (!member || !sServerFacade.IsAlive(member))
                    continue;

                newdistance = sServerFacade.GetDistance2d(member, unit);
                if (!result || newdistance < distance)
                {
                    distance = newdistance;
                    result = unit;
                }
            }
        }
        else
        {
            if (!result || (newdistance < distance && urand(0, abs(distance - newdistance)) > sPlayerbotAIConfig.sightDistance * 0.1))
            {
                distance = newdistance;
                result = unit;
            }
        }
    }

    if (ai->HasStrategy("debug grind", BotState::BOT_STATE_NON_COMBAT))
    {
        if(result)
        {
            ai->TellPlayer(GetMaster(), chat->formatWorldobject(result) + " 已选定.");
        }
        else
        {
            ai->TellPlayer(GetMaster(), "未找到可攻击目标.");
        }
    }

    return result;
}

bool GrindTargetValue::needForQuest(Unit* target)
{
    bool justCheck = (bot->GetObjectGuid() == target->GetObjectGuid());

    QuestStatusMap& questMap = bot->getQuestStatusMap();
    for (auto& quest : questMap)
    {
        const Quest* questTemplate = sObjectMgr.GetQuestTemplate(quest.first);
        if (!questTemplate)
            continue;

        uint32 questId = questTemplate->GetQuestId();

        if (!questId)
            continue;

        QuestStatus status = bot->GetQuestStatus(questId);

        if ((status == QUEST_STATUS_COMPLETE && !bot->GetQuestRewardStatus(questId)))
        {
            if (!justCheck && !target->HasInvolvedQuest(questId))
                continue;

            return true;
        }
        else if (status == QUEST_STATUS_INCOMPLETE)
        {
            QuestStatusData* questStatus = sTravelMgr.getQuestStatus(bot, questId);

            if ((int32)questTemplate->GetQuestLevel() > (int32)bot->GetLevel()+5)
                continue;

            for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
            {
                int32 entry = questTemplate->ReqCreatureOrGOId[j];

                if (entry && entry > 0)
                {
                    int required = questTemplate->ReqCreatureOrGOCount[j];
                    int available = questStatus->m_creatureOrGOcount[j];

                    if (required && available < required && (target->GetEntry() == entry || justCheck))
                        return true;
                }

                if (justCheck)
                {
                    int32 itemId = questTemplate->ReqItemId[j];

                    if (itemId && itemId > 0)
                    {
                        int required = questTemplate->ReqItemCount[j];
                        int available = questStatus->m_itemcount[j];

                        if (required && available < required)
                            return true;
                    }
                }
            }

            if (!justCheck)
            {
                CreatureInfo const* data = sObjectMgr.GetCreatureTemplate(target->GetEntry());

                if (data)
                {
                    uint32 lootId = data->LootId;

                    if (lootId)
                    {
                        if (LootTemplates_Creature.HaveQuestLootForPlayer(lootId, bot))
                            return true;
                    }
                }
            }
        }

    }
    return false;
}

int GrindTargetValue::GetTargetingPlayerCount( Unit* unit )
{
    Group* group = bot->GetGroup();
    if (!group)
        return 0;

    int count = 0;
    Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
    for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
    {
        Player *member = sObjectMgr.GetPlayer(itr->guid);
        if( !member || !sServerFacade.IsAlive(member) || member == bot)
            continue;

        PlayerbotAI* ai = member->GetPlayerbotAI();
        if ((ai && *ai->GetAiObjectContext()->GetValue<Unit*>("current target") == unit) ||
            (!ai && member->GetSelectionGuid() == unit->GetObjectGuid()))
            count++;
    }

    return count;
}