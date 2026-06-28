#include "pch.hpp"
#include <map>
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"
#include "safe_vault.hpp"

static std::map<std::string, std::string> sv_passwords;
static std::map<std::string, std::vector<::slot>> sv_storage;

static std::string sv_key(const std::string& wname, short tx, short ty)
{
    return std::format("{}_{}_{}", wname, tx, ty);
}

void safe_vault_return(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::string key = sv_key(world->name, tilex, tiley);

    /* setup new password */
    if (hPipe["dialog_name"] == "safe_vault_setup")
    {
        std::string pass = hPipe["sv_password"];
        if (pass.empty())
        {
            on::ConsoleMessage(event.peer, "`4Password cannot be empty!``");
            return;
        }
        sv_passwords[key] = pass;
        on::ConsoleMessage(event.peer, "`2Safe Vault password set!``");
        return;
    }

    /* unlock attempt */
    if (hPipe["dialog_name"] == "safe_vault_unlock")
    {
        std::string pass = hPipe["sv_password"];
        if (sv_passwords[key] != pass)
        {
            on::ConsoleMessage(event.peer, "`4Wrong password!``");
            return;
        }

        /* show storage contents */
        std::string items_list;
        auto &vec = sv_storage[key];
        if (vec.empty())
            items_list = "add_textbox|`oThe safe is empty.``|left|\\n";
        else
        {
            for (std::size_t i = 0; i < vec.size(); ++i)
            {
                auto item = std::ranges::find(items, vec[i].id, &::item::id);
                std::string name = (item != items.end()) ? item->raw_name : "Unknown";
                items_list += std::format("add_button|sv_take_{}|`w{} x{}``|noflags|0|0|\\n", i, name, vec[i].count);
            }
        }

        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wSafe Vault``|left|5476|\\n"
                "add_spacer|small|\\n"
                "{}"
                "add_spacer|small|\\n"
                "add_item_picker|sv_deposit_item|`wDeposit Item``|Choose an item to store|\\n"
                "add_text_input|sv_deposit_count|Amount|1|5|\\n"
                "embed_data|tilex|{}\\n"
                "embed_data|tiley|{}\\n"
                "add_spacer|small|\\n"
                "add_quick_exit|\\n"
                "end_dialog|safe_vault_deposit|Close|Deposit|\\n",
                items_list, tilex, tiley
            )
        });
        return;
    }

    /* deposit */
    if (hPipe["dialog_name"] == "safe_vault_deposit" && !hPipe["sv_deposit_item"].empty())
    {
        short item_id = (short)atoi(hPipe["sv_deposit_item"].c_str());
        short count = (short)atoi(hPipe["sv_deposit_count"].c_str());
        if (count <= 0) count = 1;

        bool has = false;
        for (const ::slot &s : pPeer->slots)
        {
            if (s.id == item_id && s.count >= count) { has = true; break; }
        }
        if (!has)
        {
            on::ConsoleMessage(event.peer, "`4You don't have enough of that item!``");
            return;
        }

        modify_item_inventory(event, ::slot(item_id, -count));
        auto &vec = sv_storage[key];
        bool merged = false;
        for (auto &e : vec)
        {
            if (e.id == item_id) { e.count += count; merged = true; break; }
        }
        if (!merged) vec.push_back(::slot(item_id, count));

        auto it = std::ranges::find(items, item_id, &::item::id);
        on::ConsoleMessage(event.peer, std::format("`2Stored {} x{} in the Safe Vault.``",
            (it != items.end()) ? it->raw_name : "Item", count));
        return;
    }

    /* take item */
    std::string btn = hPipe["buttonClicked"];
    if (btn.starts_with("sv_take_"))
    {
        int idx = atoi(btn.c_str() + 8);
        auto &vec = sv_storage[key];
        if (idx >= 0 && (std::size_t)idx < vec.size())
        {
            modify_item_inventory(event, ::slot(vec[idx].id, vec[idx].count));
            auto it = std::ranges::find(items, vec[idx].id, &::item::id);
            on::ConsoleMessage(event.peer, std::format("`2Withdrew {} x{} from Safe Vault.``",
                (it != items.end()) ? it->raw_name : "Item", vec[idx].count));
            vec.erase(vec.begin() + idx);
        }
    }
}
