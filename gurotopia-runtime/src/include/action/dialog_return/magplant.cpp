#include "pch.hpp"
#include <map>
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"
#include "magplant.hpp"

static std::map<std::string, std::vector<::slot>> mp_storage;
static std::map<std::string, short> mp_stored_id;
static std::map<std::string, int> mp_stored_count;

static std::string mp_key(const std::string& wname, short tx, short ty)
{
    return std::format("{}_{}_{}", wname, tx, ty);
}

void magplant_return(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::string key = mp_key(world->name, tilex, tiley);

    /* withdraw all */
    if (hPipe["buttonClicked"] == "mp_withdraw_all")
    {
        short stored_id = mp_stored_id.count(key) ? mp_stored_id[key] : 0;
        int stored_count = mp_stored_count.count(key) ? mp_stored_count[key] : 0;
        if (stored_id == 0 || stored_count == 0)
        {
            on::ConsoleMessage(event.peer, "`4The MAGPLANT is empty!``");
            return;
        }

        while (stored_count > 0)
        {
            short batch = (stored_count > 200) ? 200 : (short)stored_count;
            modify_item_inventory(event, ::slot(stored_id, batch));
            stored_count -= batch;
        }
        mp_stored_count[key] = 0;
        mp_stored_id[key] = 0;
        on::ConsoleMessage(event.peer, "`2Withdrew all items from the MAGPLANT.``");
        return;
    }

    /* deposit */
    if (!hPipe["mp_deposit_item"].empty())
    {
        short item_id = (short)atoi(hPipe["mp_deposit_item"].c_str());
        short count = (short)atoi(hPipe["mp_deposit_count"].c_str());
        if (count <= 0) count = 1;

        /* verify peer has the item */
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

        /* MAGPLANT can only store one type of item */
        short current_id = mp_stored_id.count(key) ? mp_stored_id[key] : 0;
        if (current_id != 0 && current_id != item_id)
        {
            on::ConsoleMessage(event.peer, "`4This MAGPLANT already has a different item stored!``");
            return;
        }

        modify_item_inventory(event, ::slot(item_id, -count));
        mp_stored_id[key] = item_id;
        mp_stored_count[key] += count;

        auto it = std::ranges::find(items, item_id, &::item::id);
        on::ConsoleMessage(event.peer, std::format("`2Deposited {} x{} into the MAGPLANT.``",
            (it != items.end()) ? it->raw_name : "Item", count));

        /* if auto-farm enabled, it would auto-plant/break nearby — just notify for now */
        if (atoi(hPipe["mp_autofarm"].c_str()))
        {
            on::ConsoleMessage(event.peer, "`2Auto-Farm enabled for this MAGPLANT.``");
        }
    }
}
