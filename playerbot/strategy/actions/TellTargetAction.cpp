#include "botpch.h"
#include "../../playerbot.h"
#include "TellTargetAction.h"

#include "../../ServerFacade.h"
#include "ThreatManager.h"

using namespace ai;

bool TellTargetAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    Unit* target = context->GetValue<Unit*>("current target")->Get();
    if (target)
    {
        ostringstream out;
		out << "正在攻击 " << target->GetName();
        ai->TellPlayer(requester, out);

        context->GetValue<Unit*>("old target")->Set(target);
    }
    return true;
}

bool TellAttackersAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();

    ai->TellPlayer(requester, "--- 攻击者列表 ---");

    list<ObjectGuid> attackers = context->GetValue<list<ObjectGuid>>("attackers")->Get();
    for (list<ObjectGuid>::iterator i = attackers.begin(); i != attackers.end(); i++)
    {
        Unit* unit = ai->GetUnit(*i);
        if (!unit || !sServerFacade.IsAlive(unit))
            continue;

        ai->TellPlayer(requester, unit->GetName());
    }

    ai->TellPlayer(requester, "--- 威胁值 ---");
    HostileReference* ref = sServerFacade.GetHostileRefManager(bot).getFirst();
    if (!ref)
        return true;

    while (ref)
    {
        ThreatManager* threatManager = ref->getSource();
        Unit* unit = threatManager->getOwner();
        float threat = ref->getThreat();

        ostringstream out; out << unit->GetName() << " (" << threat << ")";
        ai->TellPlayer(requester, out);

        ref = ref->next();
    }
    return true;
}

bool TellPossibleAttackTargetsAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    ai->TellPlayer(requester, "--- 攻击目标列表 ---");

    list<ObjectGuid> attackers = context->GetValue<list<ObjectGuid>>("possible attack targets")->Get();
    for (list<ObjectGuid>::iterator i = attackers.begin(); i != attackers.end(); i++)
    {
        Unit* unit = ai->GetUnit(*i);
        if (!unit || !sServerFacade.IsAlive(unit))
            continue;

        ai->TellPlayer(requester, unit->GetName());
    }

    ai->TellPlayer(requester, "--- 威胁值 ---");
    HostileReference *ref = sServerFacade.GetHostileRefManager(bot).getFirst();
    if (!ref)
        return true;

    while( ref )
    {
        ThreatManager *threatManager = ref->getSource();
        Unit *unit = threatManager->getOwner();
        float threat = ref->getThreat();

        ostringstream out; out << unit->GetName() << " (" << threat << ")";
        ai->TellPlayer(requester, out);

        ref = ref->next();
    }
    return true;
}
