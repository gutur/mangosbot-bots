#include "botpch.h"
#include "../../playerbot.h"
#include "RangedCombatStrategy.h"

using namespace ai;

void RangedCombatStrategy::InitCombatTriggers(list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "enemy too close for spell",
        NextAction::array(0, new NextAction("flee", ACTION_MOVE), NULL)));
}
