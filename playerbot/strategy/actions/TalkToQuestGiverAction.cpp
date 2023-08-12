#include "Config/Config.h"
#include "botpch.h"
#include "../../playerbot.h"
#include "TalkToQuestGiverAction.h"
#include "../values/ItemUsageValue.h"

using namespace ai;

bool TalkToQuestGiverAction::ProcessQuest(Player* requester, Quest const* quest, WorldObject* questGiver)
{
    bool isCompleted = false;

    std::ostringstream out; out << "Quest ";

    QuestStatus status = bot->GetQuestStatus(quest->GetQuestId());

    if (sPlayerbotAIConfig.syncQuestForPlayer)
    {
        if (requester && (!requester->GetPlayerbotAI() || requester->GetPlayerbotAI()->IsRealPlayer()))
        {
            QuestStatus masterStatus = requester->GetQuestStatus(quest->GetQuestId());
            if (masterStatus == QUEST_STATUS_INCOMPLETE || masterStatus == QUEST_STATUS_FAILED)
                isCompleted |= CompleteQuest(requester, quest->GetQuestId());
        }
    }

    if (sPlayerbotAIConfig.syncQuestWithPlayer)
    {        
        if (requester && requester->GetQuestStatus(quest->GetQuestId()) == QUEST_STATUS_COMPLETE && (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_FAILED))
        {
            isCompleted |= CompleteQuest(bot, quest->GetQuestId());
            status = bot->GetQuestStatus(quest->GetQuestId());
        }
    }    

    switch (status)
    {
    case QUEST_STATUS_COMPLETE:
#ifdef MANGOS
    case QUEST_STATUS_FORCE_COMPLETE:
#endif
        isCompleted |= TurnInQuest(requester, quest, questGiver, out);
        break;
    case QUEST_STATUS_INCOMPLETE:
        out << "|cffff0000未完成|r";
        break;
    case QUEST_STATUS_AVAILABLE:
    case QUEST_STATUS_NONE:
        out << "|cff00ff00可接受|r";
        break;
    case QUEST_STATUS_FAILED:
        out << "|cffff0000失败|r";
        break;
    }

    out << ": " << chat->formatQuest(quest);
    ai->TellPlayer(requester, out, PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, false);

    return isCompleted;
}

bool TalkToQuestGiverAction::TurnInQuest(Player* requester, Quest const* quest, WorldObject* questGiver, ostringstream& out)
{
    uint32 questID = quest->GetQuestId();
        
    if (bot->GetQuestRewardStatus(questID))
        return false;
    
    if (sPlayerbotAIConfig.globalSoundEffects)
        bot->PlayDistanceSound(621);

    sPlayerbotAIConfig.logEvent(ai, "TalkToQuestGiverAction", quest->GetTitle(), to_string(quest->GetQuestId()));

    if (quest->GetRewChoiceItemsCount() == 0)
        RewardNoItem(quest, questGiver, out);
    else if (quest->GetRewChoiceItemsCount() == 1)
        RewardSingleItem(quest, questGiver, out);
    else {
        RewardMultipleItem(requester, quest, questGiver, out);
    }

    return true;
}

void TalkToQuestGiverAction::RewardNoItem(Quest const* quest, WorldObject* questGiver, ostringstream& out) 
{
    if (bot->CanRewardQuest(quest, false))
    {
        bot->RewardQuest(quest, 0, questGiver, false);
        out << "已完成.";
    }
    else
    {
        out << "|cffff0000无法提交任务|r";
    }
}

void TalkToQuestGiverAction::RewardSingleItem(Quest const* quest, WorldObject* questGiver, ostringstream& out) 
{
    int index = 0;
    ItemPrototype const *item = sObjectMgr.GetItemPrototype(quest->RewChoiceItemId[index]);
    if (bot->CanRewardQuest(quest, index, false))
    {
        bot->RewardQuest(quest, index, questGiver, true);

        out << "奖励物品 " << chat->formatItem(item);
    }
    else
    {
        out << "|cffff0000无法提交任务:|r, 奖励物品: " << chat->formatItem(item);
    }
}

ItemIds TalkToQuestGiverAction::BestRewards(Quest const* quest)
{
    ItemIds returnIds;
    ItemUsage bestUsage = ItemUsage::ITEM_USAGE_NONE;
    if (quest->GetRewChoiceItemsCount() == 0)
        return returnIds;
    else if (quest->GetRewChoiceItemsCount() == 1)    
        return { 0 };
    else
    {
        for (uint8 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
        {
            ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", quest->RewChoiceItemId[i]);
            if (usage == ItemUsage::ITEM_USAGE_EQUIP)
                bestUsage = ItemUsage::ITEM_USAGE_EQUIP;
            else if (usage == ItemUsage::ITEM_USAGE_BAD_EQUIP && bestUsage != ItemUsage::ITEM_USAGE_EQUIP)
                bestUsage = usage;
            else if (usage != ItemUsage::ITEM_USAGE_NONE && bestUsage == ItemUsage::ITEM_USAGE_NONE)
                bestUsage = usage;
        }
        for (uint8 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
        {
            ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", quest->RewChoiceItemId[i]);
            if (usage == bestUsage)
                returnIds.insert(i);
        }
        return returnIds;
    }
}

void TalkToQuestGiverAction::RewardMultipleItem(Player* requester, Quest const* quest, WorldObject* questGiver, ostringstream& out)
{
    set<uint32> bestIds;

    ostringstream outid;
    if (!ai->IsAlt() || sPlayerbotAIConfig.autoPickReward == "yes")
    {
        //Pick the first item of the best rewards.
        bestIds = BestRewards(quest);
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(quest->RewChoiceItemId[*bestIds.begin()]);
        if(proto)
            out << "奖励物品 " << chat->formatItem(proto);
        bot->RewardQuest(quest, *bestIds.begin(), questGiver, true);
    }
    else if (sPlayerbotAIConfig.autoPickReward == "no")
    {   //Old functionality, list rewards.
        AskToSelectReward(requester, quest, out, false);       
    }
    else 
    {   //Try to pick the usable item. If multiple list usable rewards.
        bestIds = BestRewards(quest);
        if (bestIds.size() > 0)
        {
            AskToSelectReward(requester, quest, out, true);
        }
        else
        {
            //Pick the first item
            ItemPrototype const* proto = sObjectMgr.GetItemPrototype(quest->RewChoiceItemId[*bestIds.begin()]);
            if (proto)
                out << "奖励物品 " << chat->formatItem(proto);
            bot->RewardQuest(quest, *bestIds.begin(), questGiver, true);
        }
    }
}

void TalkToQuestGiverAction::AskToSelectReward(Player* requester, Quest const* quest, ostringstream& out, bool forEquip)
{
    ostringstream msg;
    msg << "请选择奖励: ";
    for (uint8 i=0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        ItemPrototype const* item = sObjectMgr.GetItemPrototype(quest->RewChoiceItemId[i]);
        ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", quest->RewChoiceItemId[i]);

        if (!forEquip || BestRewards(quest).count(i) > 0)
        {
            msg << chat->formatItem(item);
        }
    }
    ai->TellPlayer(requester, msg, PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, false);

    out << "奖励待领取.";
}
