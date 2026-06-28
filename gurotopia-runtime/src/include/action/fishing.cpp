#include "pch.hpp"
#include "fishing.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"
#include "tools/ransuu.hpp"

/* @note fishing mini-game — wrench on FISHING_BLOCK near water, wait 2-5 sec, catch random fish */
/* fish loot table: item_id, name, weight (higher = more common) */
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

void action::fishing(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    ::hPipe hPipe{ header };
    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    /* check for water nearby — at least one adjacent tile must have S_WATER */
    bool has_water = false;
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            if (dx == 0 && dy == 0) continue;
            int nx = tilex + dx, ny = tiley + dy;
            if (nx < 0 || nx >= 100 || ny < 0 || ny >= 60) continue;
            if (world->blocks[cord(nx, ny)].state[3] & S_WATER) { has_water = true; break; }
        }
        if (has_water) break;
    }

    if (!has_water)
    {
        on::ConsoleMessage(event.peer, "`4You need to be near water to fish!``");
        return;
    }

    /* start fishing — show a dialog that simulates waiting */
    ransuu r;
    int wait_sec = r[{2, 5}];

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wFishing``|left|5638|\\n"
            "add_spacer|small|\\n"
            "add_textbox|`oYou cast your line into the water...``|left|\\n"
            "add_spacer|small|\\n"
            "add_textbox|`oWait for a bite! Estimated time: `w{} seconds````|left|\\n"
            "add_spacer|small|\\n"
            "embed_data|tilex|{}\\n"
            "embed_data|tiley|{}\\n"
            "embed_data|wait_time|{}\\n"
            "add_button|fish_reel|`wReel In!``|noflags|0|0|\\n"
            "add_spacer|small|\\n"
            "add_quick_exit|\\n"
            "end_dialog|fishing_wait|Close|Reel In!|\\n",
            wait_sec, tilex, tiley, wait_sec
        )
    });

    on::ConsoleMessage(event.peer, std::format("`oYou cast your line. Wait for a bite!``"));
}
