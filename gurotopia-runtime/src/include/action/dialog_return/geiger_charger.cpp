#include "pch.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"
#include "tools/ransuu.hpp"
#include "geiger_charger.hpp"

static const struct GeigerLoot { short id; const char* name; int weight; } geiger_table[] = {
    {3114, "Radioactive Shard", 25},
    {3116, "Glowing Ore", 20},
    {3118, "Uranium", 15},
    {3120, "Plutonium", 10},
    {3122, "Radium", 8},
    {3124, "Polonium", 5},
    {3126, "Cosmic Ore", 3},
    {3128, "Void Crystal", 2},
    {3130, "Antimatter", 1},
};

void geiger_charger_return(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    if (hPipe["buttonClicked"] == "geiger_generate")
    {
        const short tilex = (short)atoi(hPipe["tilex"].c_str());
        const short tiley = (short)atoi(hPipe["tiley"].c_str());

        auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
        if (world == worlds.end()) return;

        ::block &block = world->blocks[cord(tilex, tiley)];

        /* set cooldown */
        block.tick = std::chrono::steady_clock::now();

        ransuu r;
        int total = 0;
        for (const auto& g : geiger_table) total += g.weight;
        int roll = r[{0, total - 1}];
        short result_id = geiger_table[0].id;
        for (const auto& g : geiger_table)
        {
            roll -= g.weight;
            if (roll < 0) { result_id = g.id; break; }
        }

        short count = (short)r[{1, 3}];
        modify_item_inventory(event, ::slot(result_id, count));

        auto it = std::ranges::find(items, result_id, &::item::id);
        on::ConsoleMessage(event.peer, std::format("`5The Geiger Charger generated {} x{}!``",
            (it != items.end()) ? it->raw_name : "a radioactive item", count));

        /* send tile update to show charging visual */
        send_tile_update(event, {
            .id = block.fg,
            .punch = { tilex, tiley }
        }, block, *world);
    }
}
