#include "pch.hpp"
#include <map>
#include "tools/create_dialog.hpp"
#include "storage_box.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"

/* @note simple in-memory storage per box position keyed by world name + pos */
struct sb_entry { short id; short count; };
static std::map<std::string, std::vector<sb_entry>> sb_storage;

static std::string sb_key(const std::string& wname, short tx, short ty)
{
    return std::format("{}_{}_{}", wname, tx, ty);
}

void storage_box_return(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::string key = sb_key(world->name, tilex, tiley);

    /* withdraw view */
    if (hPipe["buttonClicked"] == "sb_withdraw")
    {
        std::string items_list;
        auto &vec = sb_storage[key];
        if (vec.empty())
        {
            items_list = "add_textbox|`oThe storage box is empty.``|left|\\n";
        }
        else
        {
            for (std::size_t i = 0; i < vec.size(); ++i)
            {
                auto item = std::ranges::find(items, vec[i].id, &::item::id);
                std::string name = (item != items.end()) ? item->raw_name : "Unknown";
                items_list += std::format("add_button|sb_take_{}|`w{} x{}``|noflags|0|0|\\n", i, name, vec[i].count);
            }
        }
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wStorage Box - Withdraw``|left|4554|\\n"
                "add_spacer|small|\\n"
                "{}"
                "add_spacer|small|\\n"
                "embed_data|tilex|{}\\n"
                "embed_data|tiley|{}\\n"
                "add_quick_exit|\\n"
                "end_dialog|storage_box|Close||\\n",
                items_list, tilex, tiley
            )
        });
        return;
    }

    /* take item by index */
    std::string btn = hPipe["buttonClicked"];
    if (btn.starts_with("sb_take_"))
    {
        int idx = atoi(btn.c_str() + 8);
        auto &vec = sb_storage[key];
        if (idx >= 0 && (std::size_t)idx < vec.size())
        {
            modify_item_inventory(event, ::slot(vec[idx].id, vec[idx].count));
            on::ConsoleMessage(event.peer, std::format("`2Withdrew {} x{} from Storage Box.``",
                std::ranges::find(items, vec[idx].id, &::item::id)->raw_name, vec[idx].count));
            vec.erase(vec.begin() + idx);
        }
        return;
    }

    /* deposit */
    if (!hPipe["sb_deposit_item"].empty())
    {
        short item_id = (short)atoi(hPipe["sb_deposit_item"].c_str());
        short count = (short)atoi(hPipe["sb_deposit_count"].c_str());
        if (count <= 0) count = 1;

        /* verify peer has the item */
        bool has = false;
        for (const ::slot &s : pPeer->slots)
        {
            if (s.id == item_id && s.count >= count) { has = true; break; }
        }
        if (!has)
        {
            on::ConsoleMessage(event.peer, "`4You don't have enough of that item!``");
            return;
        }

        modify_item_inventory(event, ::slot(item_id, -count));
        auto &vec = sb_storage[key];
        bool merged = false;
        for (auto &e : vec)
        {
            if (e.id == item_id) { e.count += count; merged = true; break; }
        }
        if (!merged) vec.push_back({item_id, count});

        auto it = std::ranges::find(items, item_id, &::item::id);
        on::ConsoleMessage(event.peer, std::format("`2Stored {} x{} in the Storage Box.``",
            (it != items.end()) ? it->raw_name : "Item", count));
    }
}
