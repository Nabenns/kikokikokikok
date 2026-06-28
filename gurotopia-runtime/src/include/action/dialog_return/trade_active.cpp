#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

#include "trade_active.hpp"

/* shared trade state — must match the one in trade.cpp */
struct Trade
{
    int p1_uid{};
    int p2_uid{};
    std::vector<::slot> p1_items{};
    std::vector<::slot> p2_items{};
    bool p1_confirm{};
    bool p2_confirm{};
};

extern std::vector<Trade> active_trades;

/* ── helpers (duplicated from trade.cpp for the TU) ── */

static ::peer* find_peer_by_uid(int uid, ENetPeer** out_enet = nullptr)
{
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
    {
        if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
        {
            ::peer *pp = static_cast<::peer*>(p.data);
            if (pp->user_id == uid)
            {
                if (out_enet) *out_enet = &p;
                return pp;
            }
        }
    }
    return nullptr;
}

static Trade* find_trade(int uid)
{
    for (auto &t : active_trades)
        if (t.p1_uid == uid || t.p2_uid == uid)
            return &t;
    return nullptr;
}

/* ═══════ trade_active dialog return ═══════ */
void trade_active(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    std::string btn = hPipe["buttonClicked"];

    Trade *trade = find_trade(pPeer->user_id);
    if (!trade)
    { on::ConsoleMessage(event.peer, "`4Trade no longer active."); return; }

    bool is_p1 = (trade->p1_uid == pPeer->user_id);

    /* ── Add item to offer ── */
    if (btn.starts_with("trade_add"))
    {
        /* parse item from the item picker response: searchableItemListButton_<id> */
        std::string picker_val = hPipe["trade_add"];
        if (picker_val.empty()) picker_val = btn;

        std::vector<std::string> parts = readch(picker_val, '_');
        short item_id = 0;
        for (const auto &part : parts)
            if (number(part)) { item_id = (short)atoi(part.c_str()); break; }
        if (item_id == 0)
        {
            /* try extracting from buttonClicked format */
            std::string id_str = readch(btn, '_').back();
            if (number(id_str)) item_id = (short)atoi(id_str.c_str());
        }
        if (item_id == 0) return;

        /* verify the player has the item */
        auto it = std::ranges::find(pPeer->slots, item_id, &::slot::id);
        if (it == pPeer->slots.end())
        { on::ConsoleMessage(event.peer, "`4You don't have that item."); return; }

        short count = it->count;
        if (count < 1) count = 1;

        /* add to offer (merge if same item already offered) */
        auto &my_items = is_p1 ? trade->p1_items : trade->p2_items;
        auto offer_it = std::ranges::find(my_items, item_id, &::slot::id);
        if (offer_it != my_items.end())
            offer_it->count += count;
        else
            my_items.emplace_back(item_id, count);

        /* reset both confirmations since offer changed */
        trade->p1_confirm = false;
        trade->p2_confirm = false;

        on::ConsoleMessage(event.peer, std::format("`oAdded `w{}x Item #{} ``to your offer.", count, item_id));
        return;
    }

    /* ── Confirm trade ── */
    if (btn == "trade_confirm")
    {
        if (is_p1) trade->p1_confirm = true;
        else trade->p2_confirm = true;

        on::ConsoleMessage(event.peer, "`oYou confirmed the trade.");

        /* notify the other party */
        int other_uid = is_p1 ? trade->p2_uid : trade->p1_uid;
        ENetPeer *other_enet = nullptr;
        ::peer *other = find_peer_by_uid(other_uid, &other_enet);
        if (other && other_enet)
            on::ConsoleMessage(other_enet, std::format("`o`w{}`` confirmed the trade. Update to see!", pPeer->growid));

        /* both confirmed — execute the trade */
        if (trade->p1_confirm && trade->p2_confirm)
        {
            ENetPeer *p1_enet = nullptr, *p2_enet = nullptr;
            ::peer *p1 = find_peer_by_uid(trade->p1_uid, &p1_enet);
            ::peer *p2 = find_peer_by_uid(trade->p2_uid, &p2_enet);

            if (!p1 || !p2 || !p1_enet || !p2_enet)
            {
                on::ConsoleMessage(event.peer, "`4Trade partner is offline. Trade cancelled.");
                active_trades.erase(std::ranges::find_if(active_trades, [trade](const Trade &t) {
                    return t.p1_uid == trade->p1_uid && t.p2_uid == trade->p2_uid;
                }));
                return;
            }

            /* remove offered items from each player, give to the other */
            for (const ::slot &s : trade->p1_items)
            {
                ENetEvent fake1{ .peer = p1_enet };
                modify_item_inventory(fake1, ::slot(s.id, -s.count));
            }
            for (const ::slot &s : trade->p2_items)
            {
                ENetEvent fake2{ .peer = p2_enet };
                modify_item_inventory(fake2, ::slot(s.id, -s.count));
            }
            for (const ::slot &s : trade->p1_items)
            {
                ENetEvent fake2{ .peer = p2_enet };
                modify_item_inventory(fake2, ::slot(s.id, s.count));
            }
            for (const ::slot &s : trade->p2_items)
            {
                ENetEvent fake1{ .peer = p1_enet };
                modify_item_inventory(fake1, ::slot(s.id, s.count));
            }

            on::ConsoleMessage(p1_enet, "`2Trade complete!");
            on::ConsoleMessage(p2_enet, "`2Trade complete!");

            active_trades.erase(std::ranges::find_if(active_trades, [trade](const Trade &t) {
                return t.p1_uid == trade->p1_uid && t.p2_uid == trade->p2_uid;
            }));
        }
        return;
    }

    /* ── Cancel trade ── */
    if (btn == "trade_cancel")
    {
        int other_uid = is_p1 ? trade->p2_uid : trade->p1_uid;
        ENetPeer *other_enet = nullptr;
        ::peer *other = find_peer_by_uid(other_uid, &other_enet);
        if (other && other_enet)
            on::ConsoleMessage(other_enet, std::format("`4`w{}`` cancelled the trade.", pPeer->growid));

        on::ConsoleMessage(event.peer, "`4Trade cancelled.");

        active_trades.erase(std::ranges::find_if(active_trades, [trade](const Trade &t) {
            return t.p1_uid == trade->p1_uid && t.p2_uid == trade->p2_uid;
        }));
        return;
    }
}
