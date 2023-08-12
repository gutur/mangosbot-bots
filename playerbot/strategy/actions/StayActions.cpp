#include "botpch.h"
#include "../../playerbot.h"
#include "StayActions.h"

#include "../../ServerFacade.h"
#include "../values/LastMovementValue.h"

using namespace ai;

bool StayActionBase::Stay()
{
    AI_VALUE(LastMovement&, "last movement").Set(NULL);

    //if (!urand(0, 10)) ai->PlaySound(TEXTEMOTE_YAWN);

    MotionMaster &mm = *bot->GetMotionMaster();
#ifdef CMANGOS
	if (mm.GetCurrentMovementGeneratorType() == TAXI_MOTION_TYPE || bot->IsTaxiFlying())
#endif
#ifdef MANGOS
	if (mm.GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE || bot->IsFlying())
#endif
	{
		if (verbose) ai->TellError("我不能留在原地,我正在飞行中!");
		return false;
	} 

    uint32 sitDelay = sPlayerbotAIConfig.sitDelay / 1000;
    time_t stayTime = AI_VALUE(time_t, "stay time");
    time_t now = time(0);
    if (!stayTime)
    {
        stayTime = now - urand(0, sitDelay / 2);
        context->GetValue<time_t>("stay time")->Set(stayTime);
    }

    if (!sServerFacade.isMoving(bot))
        return false;

    ai->StopMoving();
	bot->clearUnitState(UNIT_STAT_CHASE);
	bot->clearUnitState(UNIT_STAT_FOLLOW);

    return true;
}

bool StayAction::Execute(Event& event)
{
    return Stay();
}

bool StayAction::isUseful()
{
    return !AI_VALUE2(bool, "moving", "self target");
}

bool SitAction::Execute(Event& event)
{
    if (sServerFacade.isMoving(bot))
        return false;

    bot->SetStandState(UNIT_STAND_STATE_SIT);
    return true;
}

bool SitAction::isUseful()
{
    return !AI_VALUE2(bool, "moving", "self target");
}
