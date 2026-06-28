#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "geiger_charger.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"
#include "tools/ransuu.hpp"

/* @note geiger charger — generates random radioactive items after charging time */
/* radioactive item loot table */
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

void action::geiger_charger(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    ::hPipe hPipe{ header };
    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    ::block &block = world->blocks[cord(tilex, tiley)];

    /* check if charged (cooldown check using block.tick) */
    using namespace std::chrono;
    using namespace std::literals::chrono_literals;
    int charge_time = 3600; /* 1 hour cooldown */
    bool is_ready = (steady_clock::now() - block.tick) / 1s >= charge_time;

    if (!is_ready)
    {
        int remaining = charge_time - (int)((steady_clock::now() - block.tick) / 1s);
        int mins = remaining / 60;
        int secs = remaining % 60;

        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wGeiger Charger``|left|5478|\\n"
                "add_spacer|small|\\n"
                "add_textbox|`oThe charger is still recharging.``|left|\\n"
                "add_spacer|small|\\n"
                "add_textbox|`oTime remaining: `w{}m {}s````|left|\\n"
                "add_spacer|small|\\n"
                "add_quick_exit|\\n"
                "end_dialog|geiger_charging|Close||\\n",
                mins, secs
            )
        });
        return;
    }

    /* ready to charge — show the generate dialog */
    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wGeiger Charger``|left|5478|\\n"
            "add_spacer|small|\\n"
            "add_textbox|`oThe charger is ready! Generate a random radioactive item.``|left|\\n"
            "add_spacer|small|\\n"
            "add_textbox|`oPossible items:``|left|\\n"
            "add_textbox|`5Radioactive Shard, Glowing Ore, Uranium, Plutonium``|left|\\n"
            "add_textbox|`5Radium, Polonium, Cosmic Ore, Void Crystal, Antimatter``|left|\\n"
            "add_spacer|small|\\n"
            "embed_data|tilex|{}\\n"
            "embed_data|tiley|{}\\n"
            "add_button|geiger_generate|`wGenerate!``|noflags|0|0|\\n"
            "add_spacer|small|\\n"
            "add_quick_exit|\\n"
            "end_dialog|geiger_ready|Close||\\n",
            tilex, tiley
        )
    });
}
