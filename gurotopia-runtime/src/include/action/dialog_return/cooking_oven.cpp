#include "pch.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"
#include "cooking_oven.hpp"

static const struct Recipe { short ing1; short ing2; short result; short count; const char* name; } recipes[] = {
    {3458, 3458, 3460, 1, "Cooked Fish"},
    {3462, 3462, 3464, 1, "Cooked Meat"},
    {3458, 3462, 3466, 1, "Fish Stew"},
    {2766, 3462, 3468, 1, "Pizza"},
    {2766, 3458, 3470, 1, "Fish Pie"},
    {2766, 2766, 3472, 1, "Bread"},
    {3472, 3462, 3474, 1, "Meat Pie"},
    {3472, 3458, 3476, 1, "Sandwich"},
};

void cooking_oven_return(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    short ing1 = (short)atoi(hPipe["cook_ingredient1"].c_str());
    short ing2 = (short)atoi(hPipe["cook_ingredient2"].c_str());

    if (ing1 == 0 || ing2 == 0)
    {
        on::ConsoleMessage(event.peer, "`4You need to select two ingredients!``");
        return;
    }

    /* check recipe match (either order) */
    for (const auto& r : recipes)
    {
        if ((r.ing1 == ing1 && r.ing2 == ing2) || (r.ing1 == ing2 && r.ing2 == ing1))
        {
            /* verify peer has both ingredients */
            bool has1 = false, has2 = false;
            for (const ::slot &s : pPeer->slots)
            {
                if (s.id == ing1 && s.count >= 1) has1 = true;
                if (s.id == ing2 && s.count >= 1) has2 = true;
            }
            if (!has1 || !has2)
            {
                on::ConsoleMessage(event.peer, "`4You don't have the required ingredients!``");
                return;
            }

            /* consume ingredients (if same item, remove 2) */
            if (ing1 == ing2)
            {
                modify_item_inventory(event, ::slot(ing1, -2));
            }
            else
            {
                modify_item_inventory(event, ::slot(ing1, -1));
                modify_item_inventory(event, ::slot(ing2, -1));
            }

            /* give result */
            modify_item_inventory(event, ::slot(r.result, r.count));
            auto it = std::ranges::find(items, r.result, &::item::id);
            on::ConsoleMessage(event.peer, std::format("`2You cooked {} x{}!``",
                (it != items.end()) ? it->raw_name : "Item", r.count));
            return;
        }
    }

    on::ConsoleMessage(event.peer, "`4That combination doesn't make anything edible!``");
}
