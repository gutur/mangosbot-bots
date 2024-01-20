#pragma once
#include "../Value.h"
#include "../values/PartyMemberValue.h"

namespace ai
{
    class PartyTankWithoutLifebloomValue : public PartyMemberValue, public Qualified
    {
    public:
        PartyTankWithoutLifebloomValue(PlayerbotAI* ai, string name = "party tank without lifebloom", float range = 40.0f) :
            PartyMemberValue(ai, name), Qualified() {}

    protected:
        Unit* Calculate() override;
    };
}