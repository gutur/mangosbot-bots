
#include "playerbot/playerbot.h"
#include "SkipSpellsListAction.h"
#include "playerbot/strategy/values/SkipSpellsListValue.h"
#include "LootAction.h"
#include "playerbot/ServerFacade.h"

using namespace ai;

bool SkipSpellsListAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    std::string cmd = event.getParam();
    std::set<uint32>& skipSpells = AI_VALUE(std::set<uint32>&, "skip spells list");

    if (cmd == "reset")
    {
        skipSpells.clear();
        ai->TellPlayer(requester, "已清空忽略的法术列表");
        return true;
    }
    else if (cmd.empty() || cmd == "?")
    {   
        if (skipSpells.empty())
        {
            ai->TellPlayer(requester, "忽略的法术列表为空");
        }
        else
        {
            bool first = true;
            std::ostringstream out;
            out << "忽略的法术列表: ";
            for (std::set<uint32>::iterator i = skipSpells.begin(); i != skipSpells.end(); i++)
            {
                const SpellEntry* spellEntry = sServerFacade.LookupSpellInfo(*i);
                if (!spellEntry)
                {
                    continue;
                }

                if (first) first = false; else out << ", ";
                out << chat->formatSpell(spellEntry);
            }

            ai->TellPlayer(requester, out);
        }

        return true;
    }
    else
    {
        std::vector<std::string> spells = ParseSpells(cmd);
        if (!spells.empty())
        {
            for (std::string& spell : spells)
            {
                const bool remove = spell.substr(0, 1) == "-";
                if (remove)
                {
                    // Remove the -
                    spell = spell.substr(1);
                }

                uint32 spellId = chat->parseSpell(spell);
                if (!spellId)
                {
                    spellId = AI_VALUE2(uint32, "spell id", spell);
                }

                if (!spellId)
                {
                    ai->TellError(requester, "未知法术 " + spell);
                    continue;
                }

                const SpellEntry* spellEntry = sServerFacade.LookupSpellInfo(spellId);
                if (!spellEntry)
                {
                    ai->TellError(requester, "未知法术 " + spell);
                    continue;
                }

                if (remove)
                {
                    std::set<uint32>::iterator j = skipSpells.find(spellId);
                    if (j != skipSpells.end())
                    {
                        skipSpells.erase(j);
                        std::ostringstream out;
                        out << chat->formatSpell(spellEntry) << " 已从忽略的法术中移除";
                        ai->TellPlayer(requester, out);
                    }
                }
                else
                {
                    std::set<uint32>::iterator j = skipSpells.find(spellId);
                    if (j == skipSpells.end())
                    {
                        skipSpells.insert(spellId);
                        std::ostringstream out;
                        out << chat->formatSpell(spellEntry) << " 已添加至忽略的法术列表";
                        ai->TellPlayer(requester, out);
                    }
                }
            }

            return true;
        }
        else
        {
            ai->TellPlayer(requester, "请指定一个或多个要忽略的法术");
        }
    }

    return false;
}

std::vector<std::string> SkipSpellsListAction::ParseSpells(const std::string& text)
{
    std::vector<std::string> spells;

    size_t pos = 0;
    while (pos != std::string::npos) 
    {
        size_t nextPos = text.find(',', pos);
        std::string token = text.substr(pos, nextPos - pos);
        spells.push_back(token);

        if (nextPos != std::string::npos) 
        {
            pos = nextPos + 1;
        }
        else 
        {
            break;
        }
    }

    return spells;
}
