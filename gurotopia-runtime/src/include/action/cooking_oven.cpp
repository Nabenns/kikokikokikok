#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "cooking_oven.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"

/* @note cooking recipes: {ingredient1, ingredient2, result, result_count} */
static const struct Recipe { short ing1; short ing2; short result; short count; const char* name; } recipes[] = {
    {3458, 3458, 3460, 1, "Cooked Fish"},   /* 2x Raw Fish -> Cooked Fish */
    {3462, 3462, 3464, 1, "Cooked Meat"},   /* 2x Raw Meat -> Cooked Meat */
    {3458, 3462, 3466, 1, "Fish Stew"},     /* Fish + Meat -> Fish Stew */
    {2766, 3462, 3468, 1, "Pizza"},         /* Wheat + Meat -> Pizza */
    {2766, 3458, 3470, 1, "Fish Pie"},      /* Wheat + Fish -> Fish Pie */
    {2766, 2766, 3472, 1, "Bread"},         /* 2x Wheat -> Bread */
    {3472, 3462, 3474, 1, "Meat Pie"},      /* Bread + Meat -> Meat Pie */
    {3472, 3458, 3476, 1, "Sandwich"},      /* Bread + Fish -> Sandwich */
};

void action::cooking_oven(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    ::hPipe hPipe{ header };
    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    /* build recipe list text */
    std::string recipe_text;
    for (const auto& r : recipes)
    {
        auto i1 = std::ranges::find(items, r.ing1, &::item::id);
        auto i2 = std::ranges::find(items, r.ing2, &::item::id);
        auto res = std::ranges::find(items, r.result, &::item::id);
        recipe_text += std::format("add_textbox|`o{} + {} -> `2{} x{}``|left|\\n",
            (i1 != items.end()) ? i1->raw_name : "?",
            (i2 != items.end()) ? i2->raw_name : "?",
            (res != items.end()) ? res->raw_name : "?",
            r.count);
    }

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wCooking Oven``|left|4556|\\n"
            "add_spacer|small|\\n"
            "add_textbox|`oAvailable Recipes:``|left|\\n"
            "add_spacer|small|\\n"
            "{}"
            "add_spacer|small|\\n"
            "add_textbox|`oSelect two ingredients to cook:``|left|\\n"
            "add_item_picker|cook_ingredient1|`wFirst Ingredient``|Choose first ingredient|\\n"
            "add_item_picker|cook_ingredient2|`wSecond Ingredient``|Choose second ingredient|\\n"
            "embed_data|tilex|{}\\n"
            "embed_data|tiley|{}\\n"
            "add_spacer|small|\\n"
            "add_quick_exit|\\n"
            "end_dialog|cooking_oven|Close|Cook!|\\n",
            recipe_text, tilex, tiley
        )
    });
}
