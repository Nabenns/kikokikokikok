#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

#include "trade_request.hpp"

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

/* ═══════ trade_request dialog return ═══════ */
void trade_request(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    std::string btn = hPipe["buttonClicked"];

    /* find trade where this peer is the target (p2) */
    auto it = std::ranges::find_if(active_trades, [pPeer](const Trade &t) {
        return t.p2_uid == pPeer->user_id;
    });
    if (it == active_trades.end())
    { on::ConsoleMessage(event.peer, "`4No pending trade request."); return; }

    if (btn == "trade_accept")
    {
        /* show the trade dialog to the accepter */
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wTrade Started``|left|5024|\n"
                "add_spacer|small|\n"
                "add_textbox|`oTrade has started! Add items and confirm when ready.|left|\n"
                "add_spacer|small|\n"
                "add_item_picker|trade_add|`wAdd an item to your offer``|Select an item to offer|\n"
                "add_spacer|small|\n"
                "add_button|trade_confirm|`oConfirm Trade``|noflags|0|0|\n"
                "add_button|trade_cancel|`4Cancel Trade``|noflags|0|0|\n"
                "end_dialog|trade_active|Close|Update|\n"
                "add_quick_exit|\n"
            )
        });

        /* notify the initiator */
        for (ENetPeer &p : std::span(host->peers, host->peerCount))
            if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
            {
                ::peer *pp = static_cast<::peer*>(p.data);
                if (pp->user_id == it->p1_uid)
                {
                    on::ConsoleMessage(&p, std::format("`o`w{}`` accepted your trade request!", pPeer->growid));
                    break;
                }
            }
    }
    else /* trade_decline or close */
    {
        /* notify initiator */
        for (ENetPeer &p : std::span(host->peers, host->peerCount))
            if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
            {
                ::peer *pp = static_cast<::peer*>(p.data);
                if (pp->user_id == it->p1_uid)
                {
                    on::ConsoleMessage(&p, std::format("`4`w{}`` declined your trade request.", pPeer->growid));
                    break;
                }
            }
        active_trades.erase(it);
    }
}
