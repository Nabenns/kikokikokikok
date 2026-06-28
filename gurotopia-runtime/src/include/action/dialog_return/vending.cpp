#include "pch.hpp"
#include "vending.hpp"
#include "database/database.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/SetBux.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"

/* In-memory vending machine state: pos -> {item_id, price, count} */
struct vending_state {
    ::pos pos;
    short item_id{};
    int price{};
    short count{};
};
static std::vector<vending_state> vending_machines;

void vending_return(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);
    std::string btn = hPipe["buttonClicked"];
    short tx = (short)atoi(hPipe["tilex"].c_str());
    short ty = (short)atoi(hPipe["tiley"].c_str());
    ::pos p{tx, ty};

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    auto it = std::ranges::find_if(vending_machines, [&p](const vending_state& v){ return v.pos == p; });
    if (it == vending_machines.end())
    {
        vending_machines.push_back({p, 0, 1, 0});
        it = vending_machines.end() - 1;
    }

    if (btn == "stockitem" || btn == "stockitem_selected")
    {
        /* item picker response — player selected an item to stock */
        std::string itemID = hPipe["itemID"];
        if (!itemID.empty())
        {
            short id = (short)atoi(itemID.c_str());
            /* remove from inventory */
            if (pPeer->emplace(::slot(id, -1)))
            {
                it->item_id = id;
                it->count = 1;
                on::ConsoleMessage(event.peer, "`oItem stocked in vending machine.");
            }
            else on::ConsoleMessage(event.peer, "`oYou don't have that item.");
        }
        return;
    }

    if (btn == "setprice")
    {
        int price = atoi(hPipe["price"].c_str());
        if (price > 0) it->price = price;
        on::ConsoleMessage(event.peer, std::format("`oPrice set to {} gems.", it->price));
        return;
    }

    if (btn == "buy")
    {
        if (it->item_id == 0 || it->count <= 0)
        {
            on::ConsoleMessage(event.peer, "`oMachine is empty.");
            return;
        }
        if (pPeer->gems < it->price)
        {
            on::ConsoleMessage(event.peer, "`oYou don't have enough gems.");
            return;
        }
        pPeer->gems -= it->price;
        on::SetBux(event);
        pPeer->mysql_update<signed>("gems", pPeer->gems);
        modify_item_inventory(event, ::slot(it->item_id, 1));
        it->count--;
        if (it->count <= 0) it->item_id = 0;
        auto item = std::ranges::find(items, it->item_id, &::item::id);
        on::ConsoleMessage(event.peer, std::format("`2You bought 1x {} for {} gems!", item->raw_name, it->price));
        return;
    }

    /* default: show vending dialog */
    auto item = (it->item_id > 0) ? std::ranges::find(items, it->item_id, &::item::id) : items.end();
    std::string item_info = (it->item_id > 0 && item != items.end()) ?
        std::format("add_label_with_icon|small|`w{}`` x{}|left|{}|\n", item->raw_name, it->count, it->item_id) :
        "add_textbox|This machine is empty.|left|\n";

    send_varlist(event.peer, {"OnDialogRequest",
        std::format("set_default_color|`o\n"
            "add_label_with_icon|big|`wVending Machine``|left|1276|\n"
            "add_spacer|small|\n"
            "{}"
            "add_textbox|`oPrice: `w{} gems``|left|\n"
            "add_spacer|small|\n"
            "add_item_picker|stockitem|`wStock Item``|Choose an item to stock|\n"
            "add_text_input|price|Price|{}|5|\n"
            "add_button|setprice|Set Price|noflags|0|0|\n"
            "add_spacer|small|\n"
            "add_button|buy|`2Buy ({} gems)``|noflags|0|0|\n"
            "add_spacer|small|\n"
            "embed_data|tilex|{}\n"
            "embed_data|tiley|{}\n"
            "add_quick_exit|\n"
            "end_dialog|popup||Close|\n",
            item_info, it->price, it->price, it->price, tx, ty)
    });
}
