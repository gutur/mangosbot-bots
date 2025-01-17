#pragma once
#include "GenericActions.h"

namespace ai
{
    class PassLeadershipToMasterAction : public ChatCommandAction
    {
    public:
        PassLeadershipToMasterAction(PlayerbotAI* ai, string name = "队长", string message = "把队长传给你了!") : ChatCommandAction(ai, name), message(message) {}

        virtual bool Execute(Event& event) override
        {
            Player* master = GetMaster();
            if (master && master != bot && bot->GetGroup() && bot->GetGroup()->IsMember(master->GetObjectGuid()))
            {
                WorldPacket p(SMSG_GROUP_SET_LEADER, 8);
                p << master->GetObjectGuid();
                bot->GetSession()->HandleGroupSetLeaderOpcode(p);
                
                if (!message.empty())
                    ai->TellPlayerNoFacing(GetMaster(), message);

                if (sRandomPlayerbotMgr.IsRandomBot(bot))
                {
                    ai->ResetStrategies();
                    ai->Reset();
                }
                
                return true;
            }

            return false;
        }

        virtual bool isUseful()
        {
            return ai->IsAlt() && bot->GetGroup() && bot->GetGroup()->IsLeader(bot->GetObjectGuid());
        }

        bool isUsefulWhenStunned() override { return true; }

    protected:
        string message;
    };

    class GiveLeaderAction : public PassLeadershipToMasterAction 
    {
    public:
        GiveLeaderAction(PlayerbotAI* ai, string message = "Lead the way!") : PassLeadershipToMasterAction(ai, "give leader", message) {}

        virtual bool isUseful()
        {
            return ai->HasRealPlayerMaster() && bot->GetGroup() && bot->GetGroup()->IsLeader(bot->GetObjectGuid());
        }
    };    
}
