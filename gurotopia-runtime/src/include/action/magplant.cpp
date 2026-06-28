#include "pch.hpp"
#include <map>
#include "tools/create_dialog.hpp"
#include "magplant.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"

/* @note MAGPLANT — stores items for auto-farming, shows stored items and allows deposit */
static std::map<std::string, std::vector<::slot>> mp_storage; /* key -> stored items */
static std::map<std::string, short> mp_stored_id; /* key -> main stored item id */
static std::map<std::string, int> mp_stored_count; /* key -> total stored count */

static std::string mp_key(const std::string& wname, short tx, short ty)
{
    return std::format("{}_{}_{}", wname, tx, ty);
}

void action::magplant(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    ::hPipe hPipe{ header };
    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::string key = mp_key(world->name, tilex, tiley);
    short stored_id = mp_stored_id.count(key) ? mp_stored_id[key] : 0;
    int stored_count = mp_stored_count.count(key) ? mp_stored_count[key] : 0;

    std::string stored_text;
    if (stored_id == 0)
    {
        stored_text = "add_textbox|`oNo item stored in this MAGPLANT.``|left|\\n";
    }
    else
    {
        auto it = std::ranges::find(items, stored_id, &::item::id);
        std::string name = (it != items.end()) ? it->raw_name : "Unknown";
        stored_text = std::format("add_textbox|`wStored Item:`` `o{}``|left|\\n", name);
        stored_text += std::format("add_textbox|`wAmount:`` `o{}``|left|\\n", stored_count);
    }

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wMAGPLANT``|left|5640|\\n"
            "add_spacer|small|\\n"
            "{}"
            "add_spacer|small|\\n"
            "add_textbox|`oDeposit seeds or blocks for auto-farming.``|left|\\n"
            "add_spacer|small|\\n"
            "add_item_picker|mp_deposit_item|`wDeposit Item``|Choose an item to store in the MAGPLANT|\\n"
            "add_text_input|mp_deposit_count|Amount|1|5|\\n"
            "embed_data|tilex|{}\\n"
            "embed_data|tiley|{}\\n"
            "add_spacer|small|\\n"
            "add_button|mp_withdraw_all|`wWithdraw All``|noflags|0|0|\\n"
            "add_spacer|small|\\n"
            "add_checkbox|mp_autofarm|`2Enable Auto-Farm``|0|\\n"
            "add_spacer|small|\\n"
            "add_quick_exit|\\n"
            "end_dialog|magplant|Close|Deposit|\\n",
            stored_text, tilex, tiley
        )
    });
}
