
#include "playerbot/playerbot.h"
#include "OutfitAction.h"
#include "playerbot/strategy/values/OutfitListValue.h"
#include "playerbot/strategy/ItemVisitors.h"

using namespace ai;

bool OutfitAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    std::string param = event.getParam();

    if (param == "?")
    {
        List(requester);
        ai->TellPlayer(requester, "使用方法:outfit <名字> +[物品] 添加物品");
        ai->TellPlayer(requester, "使用方法:outfit <名字> -[物品] 移除物品");
        ai->TellPlayer(requester, "使用方法:outfit <名字> 装备/替换 装备物品");
    }
    else
    {
        std::string name = ai->InventoryParseOutfitName(param);
        ItemIds items = ai->InventoryParseOutfitItems(param);
        if (!name.empty())
        {
            Save(name, items);
            std::ostringstream out;
            out << "已将装备方案 " << name << " 设置为 " << param;
            ai->TellPlayer(requester, out);
            return true;
        }

        items = chat->parseItems(param);

        int space = param.find(" ");
        if (space == -1)
            return false;

        name = param.substr(0, space);
        ItemIds outfit = ai->InventoryFindOutfitItems(name);
        std::string command = param.substr(space + 1);
        if (command == "equip")
        {
            std::ostringstream out;
            out << "正在套用装备方案 " << name;
            ai->TellPlayer(requester, out);
            EquipItems(requester, outfit);
            return true;
        }
        else if (command == "replace")
        {
            std::ostringstream out;
            out << "替换当前方案,使用装备方案 " << name;
            ai->TellPlayer(requester, out);
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
            {
                Item* const pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                if (!pItem)
                    continue;

                uint8 bagIndex = pItem->GetBagSlot();
                uint8 dstBag = NULL_BAG;

                WorldPacket packet(CMSG_AUTOSTORE_BAG_ITEM, 3);
                packet << bagIndex << slot << dstBag;
                bot->GetSession()->HandleAutoStoreBagItemOpcode(packet);
            }
            EquipItems(requester, outfit);
            return true;
        }
        else if (command == "reset")
        {
            std::ostringstream out;
            out << "重置装备方案 " << name;
            ai->TellPlayer(requester, out);
            Save(name, ItemIds());
            return true;
        }
        else if (command == "update")
        {
            std::ostringstream out;
            out << "正在使用当前装备更新装备方案 " << name;
            ai->TellPlayer(requester, out);
            Update(name);
            return true;
        }

        bool remove = param.size() > 1 && param.substr(space + 1, 1) == "-";
        for (ItemIds::iterator i = items.begin(); i != items.end(); i++)
        {
            uint32 itemid = *i;
            ItemPrototype const *proto = sItemStorage.LookupEntry<ItemPrototype>(*i);
            std::ostringstream out;
            out << chat->formatItem(proto);
            if (remove)
            {
                std::set<uint32>::iterator j = outfit.find(itemid);
                if (j != outfit.end())
                    outfit.erase(j);

                out << " 已移除 ";
            }
            else
            {
                outfit.insert(itemid);
                out << " 已添加至 ";
            }
            out << name;
            ai->TellPlayer(requester, out.str());
        }
        Save(name, outfit);
    }

    return true;
}

void OutfitAction::Save(std::string name, ItemIds items)
{
    std::list<std::string>& outfits = AI_VALUE(std::list<std::string>&, "outfit list");
    for (std::list<std::string>::iterator i = outfits.begin(); i != outfits.end(); ++i)
    {
        std::string outfit = *i;
        if (name == ai->InventoryParseOutfitName(outfit))
        {
            outfits.erase(i);
            break;
        }
    }

    if (items.empty()) return;

    std::ostringstream out;
    out << name << "=";
    bool first = true;
    for (ItemIds::iterator i = items.begin(); i != items.end(); i++)
    {
        if (first) first = false; else out << ",";
        out << *i;
    }
    outfits.push_back(out.str());
}

void OutfitAction::List(Player* requester)
{
    std::list<std::string>& outfits = AI_VALUE(std::list<std::string>&, "outfit list");
    for (std::list<std::string>::iterator i = outfits.begin(); i != outfits.end(); ++i)
    {
        std::string outfit = *i;
        std::string name = ai->InventoryParseOutfitName(outfit);
        ItemIds items = ai->InventoryParseOutfitItems(outfit);

        std::ostringstream out;
        out << name << ": ";
        for (ItemIds::iterator j = items.begin(); j != items.end(); ++j)
        {
            ItemPrototype const *proto = sItemStorage.LookupEntry<ItemPrototype>(*j);
            if (proto)
            {
                out << chat->formatItem(proto) << " ";
            }
        }
        ai->TellPlayer(requester, out);
    }
}

void OutfitAction::Update(std::string name)
{
    ListItemsVisitor visitor;
    ai->InventoryIterateItems(&visitor, IterateItemsMask::ITERATE_ITEMS_IN_EQUIP);

    ItemIds items;
    for (std::map<uint32, int>::iterator i = visitor.items.begin(); i != visitor.items.end(); ++i)
        items.insert(i->first);

    Save(name, items);
}
