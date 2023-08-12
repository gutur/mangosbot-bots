#include "botpch.h"
#include "../../playerbot.h"
#include "RtscAction.h"


using namespace ai;

bool RTSCAction::Execute(Event& event)
{
	string command = event.getParam();

	Player* master = ai->GetMaster();

	if (!master)
		return false;

	if (command != "reset" && !master->HasSpell(RTSC_MOVE_SPELL))
	{
		master->learnSpell(RTSC_MOVE_SPELL, false);
		ai->TellPlayerNoFacing(GetMaster(), "RTS控制已启用.");
		ai->TellPlayerNoFacing(GetMaster(), "已学会Aedm(超赞充满活力的移动)法术.");
	}
	else if (command == "reset")
	{
		if (master->HasSpell(RTSC_MOVE_SPELL))
		{
			master->removeSpell(RTSC_MOVE_SPELL);
			ai->TellPlayerNoFacing(GetMaster(), "已移除RTS控制法术.");
		}

		RESET_AI_VALUE(bool, "RTSC selected");
		RESET_AI_VALUE(string, "RTSC next spell action");

		for (auto value : ai->GetAiObjectContext()->GetValues())
			if (value.find("RTSC saved location::") != std::string::npos)
				RESET_AI_VALUE(WorldPosition, value.c_str());

		return true;
	}

	bool selected = AI_VALUE(bool, "RTSC selected");

	if (command == "select" && !selected)
	{
		SET_AI_VALUE(bool, "RTSC selected", true);
		master->GetSession()->SendPlaySpellVisual(bot->GetObjectGuid(), 5036);
		return true;
	}
	else if (command == "cancel")
	{
		RESET_AI_VALUE(bool, "RTSC selected");
		RESET_AI_VALUE(string, "RTSC next spell action");
		if(selected)
			master->GetSession()->SendPlaySpellVisual(bot->GetObjectGuid(), 6372);
		return true;
	}
	else if (command == "toggle")
	{
		if (!selected)
		{
			SET_AI_VALUE(bool, "RTSC selected", true);
			master->GetSession()->SendPlaySpellVisual(bot->GetObjectGuid(), 5036);
		}
		else
		{
			SET_AI_VALUE(bool, "RTSC selected", false);
			master->GetSession()->SendPlaySpellVisual(bot->GetObjectGuid(), 6372);
		}

		return true;
	}
	else if (command.find("save here ") != std::string::npos)
	{
		string locationName = command.substr(10);

		WorldPosition spellPosition(bot);
		SET_AI_VALUE2(WorldPosition, "RTSC saved location", locationName, spellPosition);

		Creature* wpCreature = bot->SummonCreature(15631, spellPosition.getX(), spellPosition.getY(), spellPosition.getZ(), spellPosition.getO(), TEMPSPAWN_TIMED_DESPAWN, 2000.0f);
		wpCreature->SetObjectScale(0.5f);

		return true;
	}
	else if (command.find("unsave ") != std::string::npos)
	{
		string locationName = command.substr(7);

		RESET_AI_VALUE2(WorldPosition, "RTSC saved location", locationName);

		return true;
	}
	if (command.find("save ") != std::string::npos || command == "move")
	{	
		SET_AI_VALUE(string, "RTSC next spell action", command);			

		return true;
	}
	if (command.find("show ") != std::string::npos)
	{
		string locationName = command.substr(5);
		WorldPosition spellPosition = AI_VALUE2(WorldPosition, "RTSC saved location", locationName);

		if (spellPosition)
		{
			Creature* wpCreature = bot->SummonCreature(15631, spellPosition.getX(), spellPosition.getY(), spellPosition.getZ(), spellPosition.getO(), TEMPSPAWN_TIMED_DESPAWN, 2000.0f);
			wpCreature->SetObjectScale(0.5f);
		}

		return true;
	}
	if (command.find("show") != std::string::npos)
	{
		ostringstream out; out << "已保存的位置: ";

		for (auto value : ai->GetAiObjectContext()->GetValues())
			if (value.find("RTSC saved location::") != std::string::npos)
				if (AI_VALUE2(WorldPosition, "RTSC saved location", value.substr(21).c_str()))
					out << value.substr(21).c_str() << ",";

		out.seekp(-1, out.cur);
		out << ".";

		ai->TellPlayerNoFacing(GetMaster(), out);
	}
	if (command.find("go ") != std::string::npos)
	{
		string locationName = command.substr(3);
		WorldPosition spellPosition = AI_VALUE2(WorldPosition, "RTSC saved location", locationName);

		if(spellPosition)
			return MoveToSpell(spellPosition, false);

		return true;
	}
	else if (command == "last")
	{
		WorldPosition spellPosition = AI_VALUE(WorldPosition, "see spell location");
		if (spellPosition)
			return MoveToSpell(spellPosition);
	}	

	return false;
}