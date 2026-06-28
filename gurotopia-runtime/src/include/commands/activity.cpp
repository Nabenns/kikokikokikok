#include "pch.hpp"
#include "database/database.hpp"
#include "activity.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/SetBux.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include "tools/ransuu.hpp"
#include "proton/Variant.hpp"
#include <map>

/* find a connected peer by growid (case-insensitive) */
static ::peer* find_peer_by_growid(const std::string& name, ENetPeer** out_enet = nullptr)
{
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
    {
        if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
        {
            ::peer *pp = static_cast<::peer*>(p.data);
            std::string a = pp->growid, b = name;
            std::ranges::transform(a, a.begin(), ::tolower);
            std::ranges::transform(b, b.begin(), ::tolower);
            if (a == b)
            {
                if (out_enet) *out_enet = &p;
                return pp;
            }
        }
    }
    return nullptr;
}

/* ═══════ /surgery ═══════ */
void command::surgery(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* check cooldown (1 hour) - reuse last_daily field for surgery cooldown */
    std::time_t now = std::time(0);
    if (now - pPeer->last_daily < 3600)
    {
        long remaining = 3600 - (now - pPeer->last_daily);
        long mins = remaining / 60;
        on::ConsoleMessage(event.peer, std::format("You can perform surgery again in `w{}m``.", mins));
        return;
    }

    ransuu rng;

    struct step { const char* name; const char* correct; };
    const step steps[] = {
        {"Scalpel", "cut"},
        {"Anesthesia", "inject"},
        {"Suture", "stitch"},
        {"Bandage", "wrap"}
    };

    int step1 = rng[{0, 3}];
    int step2 = rng[{0, 3}];

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wSurgery Mini-Game``|left|1258|\n"
            "add_spacer|small|\n"
            "add_textbox|`oA patient needs emergency surgery! Perform the steps correctly.``|left|\n"
            "add_spacer|small|\n"
            "add_textbox|`wStep 1: {}`` - Choose action:|left|\n"
            "add_button|surg_1_cut|Cut|noflags|0|0|\n"
            "add_button|surg_1_inject|Inject|noflags|0|0|\n"
            "add_button|surg_1_stitch|Stitch|noflags|0|0|\n"
            "add_button|surg_1_wrap|Wrap|noflags|0|0|\n"
            "add_spacer|small|\n"
            "add_textbox|`wStep 2: {}`` - Choose action:|left|\n"
            "add_button|surg_2_cut|Cut|noflags|0|0|\n"
            "add_button|surg_2_inject|Inject|noflags|0|0|\n"
            "add_button|surg_2_stitch|Stitch|noflags|0|0|\n"
            "add_button|surg_2_wrap|Wrap|noflags|0|0|\n"
            "add_spacer|small|\n"
            "embed_data|step1|{}\n"
            "embed_data|step2|{}\n"
            "add_quick_exit|\n"
            "end_dialog|surgery|Cancel|Perform Surgery|\n",
            steps[step1].name, steps[step2].name,
            step1, step2
        )
    });
}

/* ═══════ /race ═══════ */
void command::race(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    struct race_state {
        std::chrono::steady_clock::time_point start_time{};
        bool active{false};
    };
    static std::map<std::string, race_state> races;

    std::string world_name = pPeer->recent_worlds.back();
    std::string key = std::format("{}_{}", world_name, pPeer->growid);

    if (races[key].active)
    {
        /* finish race */
        auto elapsed = std::chrono::steady_clock::now() - races[key].start_time;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

        races[key].active = false;
        races[key].start_time = {};

        ransuu rng;
        int reward = 50 + rng[{0, 200}];

        pPeer->gems += reward;
        on::SetBux(event);
        pPeer->mysql_update<signed>("gems", pPeer->gems);

        on::ConsoleMessage(event.peer, std::format("`2Race finished! Time: `w{}ms`` - You earned `w{} gems``!", ms, reward));

        peers(world_name, PEER_SAME_WORLD, [&pPeer, &ms, &reward](ENetPeer& peer) {
            ::peer *pp = static_cast<::peer*>(peer.data);
            if (pp && pp != pPeer)
                on::ConsoleMessage(&peer, std::format("`o`{}{}`` finished a race in `w{}ms`` and earned `w{} gems``!",
                    pPeer->prefix, pPeer->growid, ms, reward));
        });
        return;
    }

    /* start race */
    races[key].active = true;
    races[key].start_time = std::chrono::steady_clock::now();

    on::ConsoleMessage(event.peer, "`2Race started! Reach a checkpoint and use /race again to finish!``");

    peers(world_name, PEER_SAME_WORLD, [&pPeer](ENetPeer& peer) {
        ::peer *pp = static_cast<::peer*>(peer.data);
        if (pp && pp != pPeer)
            on::ConsoleMessage(&peer, std::format("`o`{}{}`` started a race!``", pPeer->prefix, pPeer->growid));
    });
}

/* ═══════ /duel {growid} ═══════ */
void command::duel(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /duel `w{growid}``"); return; }

    std::string target_name = args[1];
    ENetPeer *target_enet = nullptr;
    ::peer *target = find_peer_by_growid(target_name, &target_enet);
    if (!target || !target_enet)
    { on::ConsoleMessage(event.peer, std::format("`4Player `w{}`` is not online.``", target_name)); return; }

    if (target == pPeer) { on::ConsoleMessage(event.peer, "`4You can't duel yourself!``"); return; }

    /* must be in same world */
    if (target->recent_worlds.back() != pPeer->recent_worlds.back())
    { on::ConsoleMessage(event.peer, "`4You must be in the same world to duel!``"); return; }

    /* send duel challenge dialog to target */
    send_varlist(target_enet, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wCard Duel Challenge``|left|758|\n"
            "add_spacer|small|\n"
            "add_textbox|`{}{}`` has challenged you to a card duel!|left|\n"
            "add_textbox|`oWinner gets `w100 gems`` from the loser.``|left|\n"
            "add_spacer|small|\n"
            "embed_data|challenger|{}\n"
            "add_button|duel_accept|`2Accept``|noflags|0|0|\n"
            "add_button|duel_decline|`4Decline``|noflags|0|0|\n"
            "add_quick_exit|\n"
            "end_dialog|duel_challenge|Close|Cancel|\n",
            pPeer->prefix, pPeer->growid, pPeer->growid
        )
    });

    on::ConsoleMessage(event.peer, std::format("`oSent duel challenge to `w{}``.", target_name));
}

/* ═══════ /guildwar {guild_name} ═══════ */
void command::guildwar(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /guildwar `w{guild_name}``"); return; }

    if (pPeer->guild_id == 0) { on::ConsoleMessage(event.peer, "`4You are not in a guild.``"); return; }

    std::string target_guild = args[1];

    /* verify target guild exists */
    int target_guild_id = 0;
    std::string target_leader_name;
    {
        ::hStmt hStmt{ "SELECT id, leader FROM guild WHERE name = ? LIMIT 1" };
        MYSQL_BIND param = make_bind_in(target_guild);
        mysql_stmt_bind_param(hStmt.pStmt, &param);
        MYSQL_BIND results[2] = {
            make_bind_out(target_guild_id),
            make_bind_out(target_leader_name)
        };
        mysql_stmt_bind_result(hStmt.pStmt, results);
        mysql_stmt_execute(hStmt.pStmt);
        if (mysql_stmt_fetch(hStmt.pStmt) != 0)
        {
            on::ConsoleMessage(event.peer, std::format("`4Guild `w{}`` does not exist.``", target_guild));
            return;
        }
        if (auto pos = target_leader_name.find('\0'); pos != std::string::npos) target_leader_name.resize(pos);
    }

    if (target_guild_id == pPeer->guild_id)
    { on::ConsoleMessage(event.peer, "`4You can't war your own guild!``"); return; }

    /* get own guild name */
    std::string own_guild_name;
    {
        ::hStmt hStmt{ "SELECT name FROM guild WHERE id = ? LIMIT 1" };
        MYSQL_BIND param = make_bind_in(pPeer->guild_id);
        mysql_stmt_bind_param(hStmt.pStmt, &param);
        MYSQL_BIND result = make_bind_out(own_guild_name);
        mysql_stmt_bind_result(hStmt.pStmt, &result);
        mysql_stmt_execute(hStmt.pStmt);
        mysql_stmt_fetch(hStmt.pStmt);
        if (auto pos = own_guild_name.find('\0'); pos != std::string::npos) own_guild_name.resize(pos);
    }

    /* notify all members of both guilds who are online */
    int notified = 0;
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
    {
        if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
        {
            ::peer *pp = static_cast<::peer*>(p.data);
            if (pp && (pp->guild_id == pPeer->guild_id || pp->guild_id == target_guild_id))
            {
                on::ConsoleMessage(&p, std::format("`5GUILD WAR: `w{}`` vs `w{}``! Prepare for battle!",
                    own_guild_name, target_guild));
                notified++;
            }
        }
    }

    on::ConsoleMessage(event.peer, std::format("`2Declared guild war on `w{}``! `w{}`` players notified.",
        target_guild, notified));
}

/* ═══════ /broadcast {message} ═══════ */
void command::broadcast(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) { on::ConsoleMessage(event.peer, "`4Only developers can broadcast.``"); return; }

    /* extract message (everything after "broadcast ") */
    std::string msg;
    std::size_t pos = std::string(text).find(" ");
    if (pos != std::string::npos)
        msg = std::string(text).substr(pos + 1);
    if (msg.empty()) { on::ConsoleMessage(event.peer, "Usage: /broadcast `w{message}``"); return; }

    /* send to all connected peers */
    peers("", PEER_ALL, [&msg, &pPeer](ENetPeer& peer) {
        on::ConsoleMessage(&peer, std::format("`5[BROADCAST] `{}{}``: {}``", pPeer->prefix, pPeer->growid, msg));
    });

    on::ConsoleMessage(event.peer, std::format("`2Broadcast sent to all players: `w{}``", msg));
}
