#include "pch.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"
#include "tools/ransuu.hpp"
#include "fishing.hpp"

static const struct FishLoot { short id; const char* name; int weight; } fish_table[] = {
    {3568, "Sardine", 30},
    {3570, "Anchovy", 25},
    {3572, "Bass", 20},
    {3574, "Salmon", 15},
    {3576, "Tuna", 10},
    {3578, "Swordfish", 5},
    {3580, "Shark", 3},
    {3582, "Kraken", 1},
};

static short roll_fish(ransuu& r)
{
    int total = 0;
    for (const auto& f : fish_table) total += f.weight;
    int roll = r[{0, total - 1}];
    for (const auto& f : fish_table)
    {
        roll -= f.weight;
        if (roll < 0) return f.id;
    }
    return fish_table[0].id;
}

void fishing_return(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    if (hPipe["buttonClicked"] == "fish_reel" || hPipe["dialog_name"] == "fishing_wait")
    {
        ransuu r;

        /* 70% chance to catch something, 30% chance it got away */
        if (r[{0, 9}] < 7)
        {
            short fish_id = roll_fish(r);
            short count = (short)r[{1, 3}];
            modify_item_inventory(event, ::slot(fish_id, count));

            auto it = std::ranges::find(items, fish_id, &::item::id);
            std::string name = (it != items.end()) ? it->raw_name : "Fish";
            on::ConsoleMessage(event.peer, std::format("`2You caught {} x{}!``", name, count));

            /* small gem reward */
            short gems = (short)r[{1, 5}];
            pPeer->gems += gems;
            on::ConsoleMessage(event.peer, std::format("`2You also found {} gems!``", gems));
        }
        else
        {
            on::ConsoleMessage(event.peer, "`4The fish got away! Try again.``");
        }
    }
}
