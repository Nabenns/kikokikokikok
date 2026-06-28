#include "pch.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

#include "wl_storage.hpp"

/* shared in-memory cache — matches the one in wl_storage.cpp */
static std::unordered_map<int, int> wl_stored{};

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

static void save_wl_stored(::peer *pPeer, int count)
{
    std::string stats_csv = pPeer->mysql_select<std::string>("stats");
    if (auto pos = stats_csv.find('\0'); pos != std::string::npos) stats_csv.resize(pos);

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
    wl_stored[pPeer->user_id] = count;
}

static int count_wl_in_inventory(::peer *pPeer)
{
    auto it = std::ranges::find(pPeer->slots, 242, &::slot::id);
    return (it != pPeer->slots.end()) ? it->count : 0;
}

/* ═══════ wl_deposit_confirm ═══════ */
void wl_deposit_confirm(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    int amount = atoi(hPipe["wl_deposit_amount"].c_str());
    int in_inventory = count_wl_in_inventory(pPeer);

    if (amount <= 0)
    { on::ConsoleMessage(event.peer, "`4Invalid amount."); return; }
    if (amount > in_inventory)
    { on::ConsoleMessage(event.peer, "`4You don't have that many World Locks."); return; }

    modify_item_inventory(event, ::slot(242, -amount));

    int stored = load_wl_stored(pPeer);
    stored += amount;
    save_wl_stored(pPeer, stored);

    on::ConsoleMessage(event.peer, std::format("`2Deposited `w{}`` World Lock(s). Total stored: `w{}``", amount, stored));
}

/* ═══════ wl_withdraw_confirm ═══════ */
void wl_withdraw_confirm(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    int amount = atoi(hPipe["wl_withdraw_amount"].c_str());
    int stored = load_wl_stored(pPeer);

    if (amount <= 0)
    { on::ConsoleMessage(event.peer, "`4Invalid amount."); return; }
    if (amount > stored)
    { on::ConsoleMessage(event.peer, "`4You don't have that many World Locks stored."); return; }

    stored -= amount;
    save_wl_stored(pPeer, stored);

    modify_item_inventory(event, ::slot(242, amount));

    on::ConsoleMessage(event.peer, std::format("`2Withdrew `w{}`` World Lock(s). Remaining stored: `w{}``", amount, stored));
}
