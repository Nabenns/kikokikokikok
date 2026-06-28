#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

#include "trade.hpp"

/* ── Trade state ── */
struct Trade
{
    int p1_uid{}; // @note initiator user_id
    int p2_uid{}; // @note target user_id
    std::vector<::slot> p1_items{}; // @note items offered by p1
    std::vector<::slot> p2_items{}; // @note items offered by p2
    bool p1_confirm{};
    bool p2_confirm{};
};

std::vector<Trade> active_trades{};

/* ── helpers ── */

/* find an online peer by netID, returning the ENetPeer* and ::peer* */
static ::peer* find_peer_by_netid(short netid, ENetPeer** out_enet = nullptr)
{
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
    {
        if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
        {
            ::peer *pp = static_cast<::peer*>(p.data);
            if (pp->netid == netid)
            {
                if (out_enet) *out_enet = &p;
                return pp;
            }
        }
    }
    return nullptr;
}

static ENetPeer* find_enet_peer(::peer* pp)
{
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
        if (p.state == ENET_PEER_STATE_CONNECTED && p.data == pp)
            return &p;
    return nullptr;
}

/* find a trade by either participant's user_id */
static Trade* find_trade(int uid)
{
    for (auto &t : active_trades)
        if (t.p1_uid == uid || t.p2_uid == uid)
            return &t;
    return nullptr;
}

/* render the trade dialog for a given participant */
static void show_trade_dialog(ENetEvent& event, ::peer *pPeer, Trade *trade)
{
    bool is_p1 = (trade->p1_uid == pPeer->user_id);
    const auto &my_items  = is_p1 ? trade->p1_items : trade->p2_items;
    const auto &their_items = is_p1 ? trade->p2_items : trade->p1_items;
    bool my_confirm   = is_p1 ? trade->p1_confirm : trade->p2_confirm;
    bool their_confirm = is_p1 ? trade->p2_confirm : trade->p1_confirm;

    ::peer *other = nullptr;
    int other_uid = is_p1 ? trade->p2_uid : trade->p1_uid;
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
        if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
        {
            ::peer *pp = static_cast<::peer*>(p.data);
            if (pp->user_id == other_uid) { other = pp; break; }
        }

    auto item_name = [](short id) -> std::string {
        auto it = std::ranges::find(items, id, &::item::id);
        return (it != items.end()) ? it->raw_name : std::format("Item #{}", id);
    };

    std::string my_list, their_list;
    for (const ::slot &s : my_items)
        my_list += std::format("add_label_with_icon|small|`w{}x {}``|left|{}|\n", s.count, item_name(s.id), s.id);
    if (my_list.empty()) my_list = "add_textbox|`oNo items offered yet.``|left|\n";

    for (const ::slot &s : their_items)
        their_list += std::format("add_label_with_icon|small|`w{}x {}``|left|{}|\n", s.count, item_name(s.id), s.id);
    if (their_list.empty()) their_list = "add_textbox|`oNo items offered yet.``|left|\n";

    std::string confirm_btn = my_confirm
        ? "add_button|trade_confirm|`2Confirmed``|noflags|0|0|\n"
        : "add_button|trade_confirm|`oConfirm Trade``|noflags|0|0|\n";

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wTrade with {}``|left|5024|\n"
            "add_spacer|small|\n"
            "add_label|small|`$Your Offer:``|left|\n"
            "{}"
            "add_spacer|small|\n"
            "add_label|small|`$Their Offer:``|left|\n"
            "{}"
            "add_spacer|small|\n"
            "add_item_picker|trade_add|`wAdd an item to your offer``|Select an item to offer|{}\n"
            "add_spacer|small|\n"
            "add_textbox|`oTheir status: {}``|left|\n"
            "{}"
            "add_button|trade_cancel|`4Cancel Trade``|noflags|0|0|\n"
            "end_dialog|trade_active|Close|{}|\n"
            "add_quick_exit|\n",
            other ? std::format("`{}{}``", other->prefix, other->growid) : std::string("Unknown"),
            my_list,
            their_list,
            "",
            their_confirm ? "`2Confirmed``" : "`oNot confirmed``",
            confirm_btn,
            "Update"
        )
    });
}

/* ═══════ action::trade ═══════ */
void action::trade(ENetEvent& event, const std::string& header, const std::string_view target)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* ── Phase 2: trade already active, show the trade dialog ── */
    Trade *trade = find_trade(pPeer->user_id);
    if (trade)
    {
        show_trade_dialog(event, pPeer, trade);
        return;
    }

    /* ── Phase 1: no active trade — send a request to the target ── */
    if (target.empty())
    {
        /* parse netID from header (wrench menu embeds it) */
        std::vector<std::string> pipes = readch(header, '|');
        short netid = 0;
        for (std::size_t i = 0; i + 1 < pipes.size(); ++i)
            if (pipes[i] == "netID")
            { netid = (short)atoi(pipes[i + 1].c_str()); break; }
        if (netid == 0) return;

        ENetPeer *target_enet = nullptr;
        ::peer *target_peer = find_peer_by_netid(netid, &target_enet);
        if (!target_peer || !target_enet) 
        { on::ConsoleMessage(event.peer, "`4Player not found or offline."); return; }

        if (target_peer->user_id == pPeer->user_id)
        { on::ConsoleMessage(event.peer, "`4You can't trade with yourself."); return; }

        /* check if target already has a trade */
        if (find_trade(target_peer->user_id))
        { on::ConsoleMessage(event.peer, std::format("`4{} is already in a trade.", target_peer->growid)); return; }

        /* store the pending trade */
        active_trades.push_back(Trade{
            .p1_uid = pPeer->user_id,
            .p2_uid = target_peer->user_id
        });

        /* send request dialog to target */
        send_varlist(target_enet, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wTrade Request``|left|5024|\n"
                "add_spacer|small|\n"
                "add_textbox|`{}{}`` wants to trade with you. Accept?|left|\n"
                "add_spacer|small|\n"
                "add_button|trade_accept|`2Accept``|noflags|0|0|\n"
                "add_button|trade_decline|`4Decline``|noflags|0|0|\n"
                "end_dialog|trade_request|Close|Cancel|\n"
                "add_quick_exit|\n",
                pPeer->prefix, pPeer->growid
            )
        });

        on::ConsoleMessage(event.peer, std::format("`oSent trade request to `w{}``.", target_peer->growid));
    }
}
