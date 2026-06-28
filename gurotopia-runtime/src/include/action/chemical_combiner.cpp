#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "chemical_combiner.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"

/* @note chemical combine recipes: {chemical1, chemical2, result} */
/* chemicals: G=914, R=916, P=918, B=920, Y=924 */
static const struct ChemRecipe { short ing1; short ing2; short result; const char* name; } chem_recipes[] = {
    {914, 916, 922, "Orange"},     /* G + R -> Orange */
    {914, 920, 926, "Cyan"},       /* G + B -> Cyan */
    {914, 924, 928, "Lime"},       /* G + Y -> Lime */
    {916, 920, 930, "Purple"},     /* R + B -> Purple */
    {916, 924, 932, "Red-Yellow"}, /* R + Y -> Orange-Yellow */
    {920, 924, 934, "Green-Blue"}, /* B + Y -> Green-Blue */
    {918, 916, 936, "Pink"},       /* P + R -> Pink */
    {918, 920, 938, "Magenta"},    /* P + B -> Magenta */
};

void action::chemical_combiner(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    ::hPipe hPipe{ header };
    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    /* build recipe list */
    std::string recipe_text;
    for (const auto& r : chem_recipes)
    {
        auto i1 = std::ranges::find(items, r.ing1, &::item::id);
        auto i2 = std::ranges::find(items, r.ing2, &::item::id);
        auto res = std::ranges::find(items, r.result, &::item::id);
        recipe_text += std::format("add_textbox|`o{} + {} -> `2{}``|left|\\n",
            (i1 != items.end()) ? i1->raw_name : "?",
            (i2 != items.end()) ? i2->raw_name : "?",
            (res != items.end()) ? res->raw_name : "?");
    }

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wChemical Combiner``|left|4558|\\n"
            "add_spacer|small|\\n"
            "add_textbox|`oCombine chemicals to create new ones:``|left|\\n"
            "add_spacer|small|\\n"
            "{}"
            "add_spacer|small|\\n"
            "add_item_picker|chem_ingredient1|`wFirst Chemical``|Choose first chemical|\\n"
            "add_item_picker|chem_ingredient2|`wSecond Chemical``|Choose second chemical|\\n"
            "embed_data|tilex|{}\\n"
            "embed_data|tiley|{}\\n"
            "add_spacer|small|\\n"
            "add_quick_exit|\\n"
            "end_dialog|chemical_combiner|Close|Combine!|\\n",
            recipe_text, tilex, tiley
        )
    });
}
