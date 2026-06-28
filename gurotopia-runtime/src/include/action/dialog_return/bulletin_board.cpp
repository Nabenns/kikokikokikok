#include "pch.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "proton/Variant.hpp"
#include "bulletin_board.hpp"

/* @note bulletin board dialog return — stores message in block.label like sign_edit */
void bulletin_edit(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    ::block &block = world->blocks[cord(tilex, tiley)];
    block.label = hPipe["bulletin_text"];

    send_tile_update(event, {
        .id = block.fg,
        .punch = { tilex, tiley }
    }, block, *world);

    on::ConsoleMessage(event.peer, "`2Bulletin board updated!``");
}
