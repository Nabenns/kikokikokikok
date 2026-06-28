#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

#include "wl_storage.hpp"

/* ── World Lock Storage state ──
 * In-memory cache of stored WLs per user_id.
 * Persisted to the peer's stats CSV as "wl_stored:value;".
 */
static std::unordered_map<int, int> wl_stored{};

/* load stored WL count from stats CSV */
static int load_wl_stored(::peer *pPeer)
{
    std::string stats_csv = pPeer->mysql_select<std::string>("stats");
    if (auto pos = stats_csv.find('\0'); pos != std::string::npos) stats_csv.resize(pos);

    std::vector<std::string> entries = readch(stats_csv, ';');
    for (const auto &entry : entries)
    {
        std::vector<std::string> kv = readch(entry, ':');
        if (kv.size() >= 2 && kv[0] == "wl_stored")
            return atoi(kv[1].c_str());
    }
    return 0;
}

/* save stored WL count to stats CSV (merges with existing stats) */
static void save_wl_stored(::peer *pPeer, int count)
{
    /* load current stats */
    std::string stats_csv = pPeer->mysql_select<std::string>("stats");
    if (auto pos = stats_csv.find('\0'); pos != std::string::npos) stats_csv.resize(pos);

    /* parse, update, re-serialize */
    std::vector<std::string> entries = readch(stats_csv, ';');
    bool found = false;
    std::string result;
    for (auto &entry : entries)
    {
        if (entry.empty()) continue;
        std::vector<std::string> kv = readch(entry, ':');
        if (kv.size() >= 2 && kv[0] == "wl_stored")
        {
            entry = std::format("wl_stored:{}", count);
            found = true;
        }
        if (!result.empty()) result += ";";
        result += entry;
    }
    if (!found)
    {
        if (!result.empty()) result += ";";
        result += std::format("wl_stored:{}", count);
    }

    pPeer->mysql_update<std::string>("stats", result);

    /* update in-memory cache */
    wl_stored[pPeer->user_id] = count;
}

/* count item 242 (World Lock) in inventory */
static int count_wl_in_inventory(::peer *pPeer)
{
    auto it = std::ranges::find(pPeer->slots, 242, &::slot::id);
    return (it != pPeer->slots.end()) ? it->count : 0;
}

/* ═══════ action::wl_storage ═══════ */
void action::wl_storage(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* parse buttonClicked from header if this is a dialog return */
    ::hPipe hPipe{ header };
    std::string btn = hPipe["buttonClicked"];

    /* load stored WL count */
    int stored = 0;
    if (wl_stored.contains(pPeer->user_id))
        stored = wl_stored[pPeer->user_id];
    else
    {
        stored = load_wl_stored(pPeer);
        wl_stored[pPeer->user_id] = stored;
    }

    int in_inventory = count_wl_in_inventory(pPeer);

    /* ── Deposit: select WLs from inventory ── */
    if (btn == "wl_deposit")
    {
        if (in_inventory <= 0)
        { on::ConsoleMessage(event.peer, "`4You don't have any World Locks to deposit."); return; }

        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wDeposit World Locks``|left|242|\n"
                "add_spacer|small|\n"
                "add_textbox|`oYou have `w{}`` World Locks in your inventory.|left|\n"
                "add_textbox|`oCurrently stored: `w{}``|left|\n"
                "add_spacer|small|\n"
                "add_text_input|wl_deposit_amount|Amount to deposit:|{}|5|\n"
                "add_spacer|small|\n"
                "end_dialog|wl_deposit_confirm|Close|Deposit|\n"
                "add_quick_exit|\n",
                in_inventory, stored, in_inventory
            )
        });
        return;
    }

    /* ── Deposit confirm ── */
    if (btn == "wl_deposit_confirm" || !hPipe["wl_deposit_amount"].empty())
    {
        int amount = atoi(hPipe["wl_deposit_amount"].c_str());
        if (amount <= 0)
        { on::ConsoleMessage(event.peer, "`4Invalid amount."); return; }
        if (amount > in_inventory)
        { on::ConsoleMessage(event.peer, "`4You don't have that many World Locks."); return; }

        /* remove from inventory */
        modify_item_inventory(event, ::slot(242, -amount));

        /* add to storage */
        stored += amount;
        save_wl_stored(pPeer, stored);

        on::ConsoleMessage(event.peer, std::format("`2Deposited `w{}`` World Lock(s). Total stored: `w{}``", amount, stored));
        return;
    }

    /* ── Withdraw ── */
    if (btn == "wl_withdraw")
    {
        if (stored <= 0)
        { on::ConsoleMessage(event.peer, "`4You have no World Locks stored."); return; }

        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wWithdraw World Locks``|left|242|\n"
                "add_spacer|small|\n"
                "add_textbox|`oCurrently stored: `w{}`` World Locks|left|\n"
                "add_textbox|`oIn inventory: `w{}``|left|\n"
                "add_spacer|small|\n"
                "add_text_input|wl_withdraw_amount|Amount to withdraw:|{}|5|\n"
                "add_spacer|small|\n"
                "end_dialog|wl_withdraw_confirm|Close|Withdraw|\n"
                "add_quick_exit|\n",
                stored, in_inventory, stored
            )
        });
        return;
    }

    /* ── Withdraw confirm ── */
    if (btn == "wl_withdraw_confirm" || !hPipe["wl_withdraw_amount"].empty())
    {
        int amount = atoi(hPipe["wl_withdraw_amount"].c_str());
        if (amount <= 0)
        { on::ConsoleMessage(event.peer, "`4Invalid amount."); return; }
        if (amount > stored)
        { on::ConsoleMessage(event.peer, "`4You don't have that many World Locks stored."); return; }

        /* remove from storage */
        stored -= amount;
        save_wl_stored(pPeer, stored);

        /* add to inventory */
        modify_item_inventory(event, ::slot(242, amount));

        on::ConsoleMessage(event.peer, std::format("`2Withdrew `w{}`` World Lock(s). Remaining stored: `w{}``", amount, stored));
        return;
    }

    /* ── Default: show the main storage dialog ── */
    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wWorld Lock Storage``|left|242|\n"
            "add_spacer|small|\n"
            "add_textbox|`oWorld Locks in inventory: `w{}``|left|\n"
            "add_textbox|`oWorld Locks stored: `w{}``|left|\n"
            "add_spacer|small|\n"
            "add_button|wl_deposit|`2Deposit World Locks``|noflags|0|0|\n"
            "add_button|wl_withdraw|`2Withdraw World Locks``|noflags|0|0|\n"
            "add_spacer|small|\n"
            "end_dialog|wl_storage|Close|Cancel|\n"
            "add_quick_exit|\n",
            in_inventory, stored
        )
    });
}
