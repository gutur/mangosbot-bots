#include "botpch.h"
#include "../../playerbot.h"
#include "FocusTargetAction.h"

using namespace ai;

std::string LowercaseString(const std::string& string)
{
    std::string result = string;
    if (!string.empty())
    {
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
    }

    return result;
}

std::string LowercaseString(const char* string)
{
    std::string str = string;
    return LowercaseString(str);
}

bool FocusHealSetTargetAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    if (ai->IsHeal(bot) || ai->HasStrategy("offheal", BotState::BOT_STATE_COMBAT))
    {
        const std::string param = LowercaseString(event.getParam());
        if (!param.empty())
        {
            bool removeTargets = false;
            std::vector<std::string> targetNames;

            // Multiple focus heal targets
            if (param.find(',') != std::string::npos)
            {
                std::string targetName;
                std::stringstream ss(param);
                
                while (std::getline(ss, targetName, ','))
                {
                    targetNames.push_back(targetName);
                }
            }
            else
            {
                removeTargets = param == "none" || param == "unset";
                if (!removeTargets)
                {
                    targetNames.push_back(param);
                }
            }

            std::list<ObjectGuid> focusHealTargets;
            if (removeTargets)
            {
                ai->TellPlayerNoFacing(requester, "移除焦点治疗目标");
            }
            else
            {
                // Look for the targets in the group
                const Group* group = bot->GetGroup();
                if (group)
                {
                    Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
                    for (const std::string& targetName : targetNames)
                    {
                        ObjectGuid targetGuid;
                        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
                        {
                            Player* member = sObjectMgr.GetPlayer(itr->guid);
                            if (member)
                            {
                                const std::string memberName = LowercaseString(member->GetName());
                                if (memberName == targetName)
                                {
                                    targetGuid = itr->guid;
                                    break;
                                }
                            }
                        }

                        if (!targetGuid.IsEmpty())
                        {
                            focusHealTargets.push_back(targetGuid);
                            std::stringstream message; message << "添加 " << targetName << " 焦点治疗目标";
                            ai->TellPlayerNoFacing(requester, message.str());
                        }
                        else
                        {
                            std::stringstream message; message << "我跟他不在一个队里: " << targetName;
                            ai->TellPlayerNoFacing(requester, message.str());
                        }
                    }
                }
                else
                {
                    ai->TellPlayerNoFacing(requester, "I'm not in a group");
                }
            }

            SET_AI_VALUE(std::list<ObjectGuid>, "focus heal target", focusHealTargets);

            if (focusHealTargets.empty())
            {
                // Remove the focus heal target strategy if not set
                if (ai->HasStrategy("focus heal target", BotState::BOT_STATE_COMBAT))
                {
                    ai->ChangeStrategy("-focus heal target", BotState::BOT_STATE_COMBAT);
                }

                return removeTargets;
            }
            else
            {
                // Set the focus heal target strategy if not set
                if (!ai->HasStrategy("focus heal target", BotState::BOT_STATE_COMBAT))
                {
                    ai->ChangeStrategy("+focus heal target", BotState::BOT_STATE_COMBAT);
                }

                return true;
            }
        }
        else
        {
            ai->TellPlayerNoFacing(requester, "请提供一个或者更多玩家名称");
        }
    }
    else
    {
        ai->TellPlayerNoFacing(requester, "我不是治疗师或辅助治疗师(请将我的战略设为治疗或辅助治疗)");
    }

    return false;   
}