#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "storage_box.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"

void action::storage_box(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    /* header format: "tilex|X|tiley|Y" */
    ::hPipe hPipe{ header };
    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    ::block &block = world->blocks[cord(tilex, tiley)];

    /* show storage dialog — deposit/withdraw via item picker */
    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wStorage Box``|left|4554|\\n"
            "add_spacer|small|\\n"
            "add_textbox|`oStore your items safely in this box.``|left|\\n"
            "add_spacer|small|\\n"
            "embed_data|tilex|{}\\n"
            "embed_data|tiley|{}\\n"
            "add_item_picker|sb_deposit_item|`wDeposit Item``|Choose an item to store in the box!|\\n"
            "add_text_input|sb_deposit_count|Amount|1|5|\\n"
            "add_spacer|small|\\n"
            "add_button|sb_withdraw|`wWithdraw Items``|noflags|0|0|\\n"
            "add_spacer|small|\\n"
            "add_quick_exit|\\n"
            "end_dialog|storage_box|Close|Deposit|\\n",
            tilex, tiley
        )
    });
}
