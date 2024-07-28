
#include "playerbot/playerbot.h"
#include "HireAction.h"

using namespace ai;

bool HireAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    if (!requester)
        return false;

    if (!sRandomPlayerbotMgr.IsRandomBot(bot))
        return false;

    uint32 account = sObjectMgr.GetPlayerAccountIdByGUID(requester->GetObjectGuid());
    auto results = CharacterDatabase.PQuery("SELECT count(*) FROM characters where account = '%u'", account);

    uint32 charCount = 10;
    if (results)
    {
        Field* fields = results->Fetch();
        charCount = fields[0].GetUInt32();
    }

    if (charCount >= 10)
    {
        ai->TellPlayer(requester, "你已经达到最大角色数量限制");
        return false;
    }

    if ((int)bot->GetLevel() > (int)requester->GetLevel())
    {
        ai->TellPlayer(requester, "你不能雇佣比你级别高的角色");
        return false;
    }

    uint32 discount = sRandomPlayerbotMgr.GetTradeDiscount(bot, requester);
    uint32 m = 1 + (bot->GetLevel() / 10);
    uint32 moneyReq = m * 5000 * bot->GetLevel();
    if ((int)discount < (int)moneyReq)
    {
        std::ostringstream out;
        out << "你不能雇佣我,我基本上都不认识你,确保你至少拥有 " << chat->formatMoney(moneyReq) << " 作为交易费用";
        ai->TellPlayer(requester, out.str());
        return false;
    }

    ai->TellPlayer(requester, "我会在你下次重新登录时加入你的队伍");

    bot->SetMoney(moneyReq);
    sRandomPlayerbotMgr.Remove(bot);
    CharacterDatabase.PExecute("update characters set account = '%u' where guid = '%u'",
            account, bot->GetGUIDLow());

    return true;
}
