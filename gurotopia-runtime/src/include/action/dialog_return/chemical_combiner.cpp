#include "pch.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"
#include "chemical_combiner.hpp"

static const struct ChemRecipe { short ing1; short ing2; short result; const char* name; } chem_recipes[] = {
    {914, 916, 922, "Orange"},
    {914, 920, 926, "Cyan"},
    {914, 924, 928, "Lime"},
    {916, 920, 930, "Purple"},
    {916, 924, 932, "Red-Yellow"},
    {920, 924, 934, "Green-Blue"},
    {918, 916, 936, "Pink"},
    {918, 920, 938, "Magenta"},
};

void chemical_combiner_return(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    short ing1 = (short)atoi(hPipe["chem_ingredient1"].c_str());
    short ing2 = (short)atoi(hPipe["chem_ingredient2"].c_str());

    if (ing1 == 0 || ing2 == 0)
    {
        on::ConsoleMessage(event.peer, "`4You need to select two chemicals!``");
        return;
    }

    for (const auto& r : chem_recipes)
    {
        if ((r.ing1 == ing1 && r.ing2 == ing2) || (r.ing1 == ing2 && r.ing2 == ing1))
        {
            /* verify peer has both chemicals */
            bool has1 = false, has2 = false;
            for (const ::slot &s : pPeer->slots)
            {
                if (s.id == ing1 && s.count >= 1) has1 = true;
                if (s.id == ing2 && s.count >= 1) has2 = true;
            }
            if (!has1 || !has2)
            {
                on::ConsoleMessage(event.peer, "`4You don't have the required chemicals!``");
                return;
            }

            if (ing1 == ing2)
                modify_item_inventory(event, ::slot(ing1, -2));
            else
            {
                modify_item_inventory(event, ::slot(ing1, -1));
                modify_item_inventory(event, ::slot(ing2, -1));
            }

            modify_item_inventory(event, ::slot(r.result, 1));
            auto it = std::ranges::find(items, r.result, &::item::id);
            on::ConsoleMessage(event.peer, std::format("`2You combined chemicals to make {}!``",
                (it != items.end()) ? it->raw_name : "a new chemical"));
            return;
        }
    }

    on::ConsoleMessage(event.peer, "`4That combination doesn't produce anything useful!``");
}
