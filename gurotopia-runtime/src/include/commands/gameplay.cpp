#include "pch.hpp"
#include "database/database.hpp"
#include "gameplay.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/SetBux.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"

/* find a connected peer by growid (case-insensitive) */
static ::peer* find_peer_by_growid(const std::string& name)
{
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
    {
        if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
        {
            ::peer *pp = static_cast<::peer*>(p.data);
            std::string a = pp->growid, b = name;
            std::ranges::transform(a, a.begin(), ::tolower);
            std::ranges::transform(b, b.begin(), ::tolower);
            if (a == b) return pp;
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

/* serialize friends array → CSV: name:ignore;name:ignore;... */
static std::string serialize_friends(const std::array<::Friend, 25>& friends)
{
    std::string csv;
    bool first = true;
    for (const ::Friend &f : friends)
    {
        if (f.name.empty()) continue;
        if (!first) csv += ";";
        csv += std::format("{}:{}", f.name, to_char(f.ignore));
        first = false;
    }
    return csv;
}

/* parse friends CSV → populate friends array */
void load_friends(::peer *pPeer)
{
    if (pPeer->friends_csv.empty()) return;
    for (::Friend &f : pPeer->friends)
        f = ::Friend{};

    std::vector<std::string> entries = readch(pPeer->friends_csv, ';');
    std::size_t idx = 0;
    for (const std::string& entry : entries)
    {
        if (idx >= pPeer->friends.size()) break;
        std::vector<std::string> parts = readch(entry, ':');
        if (parts.size() >= 1 && !parts[0].empty())
        {
            pPeer->friends[idx].name = parts[0];
            pPeer->friends[idx].ignore = (parts.size() >= 2) ? (parts[1] == "1") : false;
            ++idx;
        }
    }
}

/* ═══════ /daily ═══════ */
void command::daily(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::time_t now = std::time(0);
    if (now - pPeer->last_daily < 86400)
    {
        long remaining = 86400 - (now - pPeer->last_daily);
        long hours = remaining / 3600;
        long minutes = (remaining % 3600) / 60;
        on::ConsoleMessage(event.peer, std::format("You can claim your next daily reward in `w{}h {}m``.", hours, minutes));
        return;
    }

    pPeer->last_daily = now;

    /* random gems: 50-500 */
    int gems = 50 + (std::rand() % 451);
    pPeer->gems += gems;
    on::SetBux(event);
    pPeer->mysql_update<signed>("gems", pPeer->gems);

    /* save last_daily via raw SQL (FROM_UNIXTIME) */
    mysql_query(db, std::format("UPDATE peer SET last_daily = FROM_UNIXTIME({}) WHERE growid = '{}'", now, pPeer->growid).c_str());

    /* pick a random item with rarity 1-10, give 1-3 count */
    std::vector<const ::item*> pool;
    for (const ::item &it : items)
        if (it.rarity >= 1 && it.rarity <= 10)
            pool.push_back(&it);

    if (!pool.empty())
    {
        const ::item *chosen = pool[std::rand() % pool.size()];
        short count = 1 + (std::rand() % 3);
        modify_item_inventory(event, ::slot(chosen->id, count));
        on::ConsoleMessage(event.peer, std::format("`2Daily Reward!`` You received `w{} gems`` and `w{}x {}``.", gems, count, chosen->raw_name));
    }
    else
    {
        on::ConsoleMessage(event.peer, std::format("`2Daily Reward!`` You received `w{} gems``.", gems));
    }
}

/* ═══════ /title ═══════ */
void command::title(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* build available titles based on stats */
    std::string title_buttons;
    title_buttons += "add_button|title_Beginner|`oBeginner`` (always available)|noflags|0|0|\n";
    if (pPeer->gems >= 1000)
        title_buttons += "add_button|title_Gem Hoarder|`oGem Hoarder`` (requires 1000 gems)|noflags|0|0|\n";
    if (pPeer->fires_removed >= 10)
        title_buttons += "add_button|title_Firefighter|`oFirefighter`` (requires 10 fires removed)|noflags|0|0|\n";
    if (pPeer->level[0] >= 20)
        title_buttons += "add_button|title_Veteran|`oVeteran`` (requires level 20)|noflags|0|0|\n";
    if (pPeer->level[0] >= 50)
        title_buttons += "add_button|title_Legend|`oLegend`` (requires level 50)|noflags|0|0|\n";

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wTitle Selection``|left|758|\n"
            "add_spacer|small|\n"
            "{}"
            "add_spacer|small|\n"
            "add_textbox|Currently equipped: `w{}``|left|\n"
            "add_quick_exit|\n"
            "end_dialog|title_select|||\n",
            title_buttons,
            pPeer->title.empty() ? "None" : pPeer->title
        )
    });
}

/* ═══════ /ignore {growid} ═══════ */
void command::ignore(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /ignore `w{growid}``"); return; }

    std::string target_name = args[1];

    /* find in friends array */
    for (::Friend &f : pPeer->friends)
    {
        if (f.name == target_name)
        {
            f.ignore = true;
            pPeer->friends_csv = serialize_friends(pPeer->friends);
            pPeer->mysql_update<std::string>("friends", pPeer->friends_csv);
            on::ConsoleMessage(event.peer, std::format("You are now ignoring `w{}``.", target_name));
            return;
        }
    }

    /* not found — add new entry */
    for (::Friend &f : pPeer->friends)
    {
        if (f.name.empty())
        {
            f.name = target_name;
            f.ignore = true;
            pPeer->friends_csv = serialize_friends(pPeer->friends);
            pPeer->mysql_update<std::string>("friends", pPeer->friends_csv);
            on::ConsoleMessage(event.peer, std::format("You are now ignoring `w{}``.", target_name));
            return;
        }
    }

    on::ConsoleMessage(event.peer, "`4Your friends list is full (25/25). Remove someone first.``");
}

/* ═══════ /unignore {growid} ═══════ */
void command::unignore(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /unignore `w{growid}``"); return; }

    std::string target_name = args[1];

    for (::Friend &f : pPeer->friends)
    {
        if (f.name == target_name)
        {
            f.ignore = false;
            pPeer->friends_csv = serialize_friends(pPeer->friends);
            pPeer->mysql_update<std::string>("friends", pPeer->friends_csv);
            on::ConsoleMessage(event.peer, std::format("You are no longer ignoring `w{}``.", target_name));
            return;
        }
    }

    on::ConsoleMessage(event.peer, std::format("`4{} is not in your friends list.``", target_name));
}

/* ═══════ /friend_add {growid} ═══════ */
void command::friend_add(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /friend_add `w{growid}``"); return; }

    std::string target_name = args[1];

    /* check if already in friends array */
    for (const ::Friend &f : pPeer->friends)
    {
        if (f.name == target_name)
        {
            on::ConsoleMessage(event.peer, std::format("`w{}`` is already your friend.", target_name));
            return;
        }
    }

    /* find empty slot (max 25) */
    for (::Friend &f : pPeer->friends)
    {
        if (f.name.empty())
        {
            f.name = target_name;
            f.ignore = false;
            pPeer->friends_csv = serialize_friends(pPeer->friends);
            pPeer->mysql_update<std::string>("friends", pPeer->friends_csv);
            on::ConsoleMessage(event.peer, std::format("Added `w{}`` to your friends list.", target_name));

            /* notify target if online */
            ::peer *target = find_peer_by_growid(target_name);
            if (target)
            {
                ENetPeer *tp = find_enet_peer(target);
                if (tp)
                    on::ConsoleMessage(tp, std::format("`{}{}`` added you as a friend!", pPeer->prefix, pPeer->growid));
            }
            return;
        }
    }

    on::ConsoleMessage(event.peer, "`4Your friends list is full (25/25). Remove someone first.``");
}

/* ═══════ /friend_remove {growid} ═══════ */
void command::friend_remove(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /friend_remove `w{growid}``"); return; }

    std::string target_name = args[1];

    for (::Friend &f : pPeer->friends)
    {
        if (f.name == target_name)
        {
            f = ::Friend{}; /* reset to default */
            pPeer->friends_csv = serialize_friends(pPeer->friends);
            pPeer->mysql_update<std::string>("friends", pPeer->friends_csv);
            on::ConsoleMessage(event.peer, std::format("Removed `w{}`` from your friends list.", target_name));
            return;
        }
    }

    on::ConsoleMessage(event.peer, std::format("`4{} is not in your friends list.``", target_name));
}

/* ═══════ /friend_list ═══════ */
void command::friend_list(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* build friends list text */
    std::string friends_text;
    bool any = false;
    for (const ::Friend &f : pPeer->friends)
    {
        if (f.name.empty()) continue;
        any = true;

        /* check if online */
        bool online = false;
        peers("", PEER_ALL, [&f, &online](ENetPeer& peer) {
            ::peer *pOthers = static_cast<::peer*>(peer.data);
            if (pOthers->growid == f.name)
                online = true;
        });

        std::string status = online ? "`2Online``" : "`4Offline``";
        std::string ignored = f.ignore ? " `4(ignored)``" : "";
        friends_text += std::format("add_textbox|`w{}`` - {}{}|left|\n", f.name, status, ignored);
    }

    if (!any)
        friends_text = "add_textbox|`oYou have no friends. Use /friend_add {growid} to add one.``|left|\n";

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wFriends List``|left|1366|\n"
            "add_spacer|small|\n"
            "{}"
            "add_spacer|small|\n"
            "add_quick_exit|\n"
            "end_dialog|friend_list|Close||\n",
            friends_text
        )
    });
}

/* ═══════ /pet_info ═══════ */
void command::pet_info(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->pet_id == 0)
    {
        on::ConsoleMessage(event.peer, "You don't have a pet equipped. Hatch one from a `wBattle Pet Cage`` or get one from the store!");
        return;
    }

    /* next level requirement: 100 * current_level */
    int next_req = 100 * pPeer->pet_level;

    send_varlist(event.peer, {
        "OnDialogRequest",
        create_dialog()
            .set_default_color("`o")
            .add_label_with_icon("big", "`wPet Info``", pPeer->pet_id)
            .add_spacer("small")
            .add_textbox(std::format("Name: `w{}``", pPeer->pet_name.empty() ? "Unnamed" : pPeer->pet_name))
            .add_textbox(std::format("Level: `w{}``", pPeer->pet_level))
            .add_textbox(std::format("XP: `w{}`` / `w{}``", pPeer->pet_xp, next_req))
            .add_spacer("small")
            .add_quick_exit()
            .end_dialog("pet_info", "Close", "")
    });
}

/* ═══════ /quest_list ═══════ */
void command::quest_list(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* quest definitions: {id, name, target} */
    struct quest_def { int id; std::string name; int target; };
    const quest_def defs[] = {
        {0, "Break 50 blocks", 50},
        {1, "Collect 100 gems", 100},
        {2, "Reach level 5", 5}
    };

    /* parse quests CSV: quest_id:progress:completed;... */
    int progress[3] = {0, 0, 0};
    bool completed[3] = {false, false, false};
    if (!pPeer->quests.empty())
    {
        std::vector<std::string> entries = readch(pPeer->quests, ';');
        for (const std::string& entry : entries)
        {
            std::vector<std::string> parts = readch(entry, ':');
            if (parts.size() >= 3)
            {
                int qid = std::stoi(parts[0]);
                if (qid >= 0 && qid < 3)
                {
                    progress[qid] = std::stoi(parts[1]);
                    completed[qid] = (parts[2] == "1");
                }
            }
        }
    }

    /* sync progress with current stats */
    progress[2] = std::min((int)pPeer->level[0], defs[2].target);
    if (pPeer->level[0] >= 5) completed[2] = true;

    /* build quest list text */
    std::string quest_text;
    for (int i = 0; i < 3; ++i)
    {
        std::string status = completed[i] ? "`2COMPLETE``" : std::format("`w{}``/`w{}``", progress[i], defs[i].target);
        quest_text += std::format("add_textbox|`w{}`` - {}|left|\n", defs[i].name, status);
    }

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wDaily Quests``|left|758|\n"
            "add_spacer|small|\n"
            "{}"
            "add_spacer|small|\n"
            "add_textbox|`oQuests reset daily. Complete them for rewards!``|left|\n"
            "add_quick_exit|\n"
            "end_dialog|quest_list|Close||\n",
            quest_text
        )
    });
}
