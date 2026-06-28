#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "bulletin_board.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"

void action::bulletin_board(ENetEvent& event, const std::string& header)
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

    /* reuse the sign edit pattern — bulletin boards store a public message in block.label */
    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_popup_name|BulletinEdit|\\n"
            "add_label_with_icon|big|`wEdit Bulletin Board``|left|{}|\\n"
            "add_textbox|`oWrite a public message for everyone to see on this bulletin board.``|left|\\n"
            "add_text_input|bulletin_text||{}|256|\\n"
            "embed_data|tilex|{}\\n"
            "embed_data|tiley|{}\\n"
            "end_dialog|bulletin_edit|Cancel|Post|\\n",
            block.fg, block.label, tilex, tiley
        )
    });
}
