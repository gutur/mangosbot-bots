#include "botpch.h"
#include "../../playerbot.h"
#include "ChangeChatAction.h"

using namespace ai;

bool ChangeChatAction::Execute(Event& event)
{
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    string text = event.getParam();
    ChatMsg parsed = chat->parseChat(text);
    if (parsed == CHAT_MSG_SYSTEM)
    {
        ostringstream out; out << "当前聊天为 " << chat->formatChat(*context->GetValue<ChatMsg>("chat"));
        ai->TellPlayer(requester, out);
    }
    else
    {
        context->GetValue<ChatMsg>("chat")->Set(parsed);
        ostringstream out; out << "聊天设置为 " << chat->formatChat(parsed);
        ai->TellPlayer(requester, out);
    }
    
    return true;
}
