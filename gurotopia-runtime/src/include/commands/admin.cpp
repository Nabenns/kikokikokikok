#include "pch.hpp"
#include "database/database.hpp"
#include "admin.hpp"
#include "https/admin.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/SetBux.hpp"
#include "on/NameChanged.hpp"
#include "tools/string.hpp"

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

/* ═══════ /give {growid} {itemid} {count} ═══════ */
void command::give(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR) { on::ConsoleMessage(event.peer, "`4You don't have permission."); return; }

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 4) { on::ConsoleMessage(event.peer, "Usage: /give `w{growid} {itemid} {count}`"); return; }

    std::string target_name = args[1];
    short item_id = (short)std::stoi(args[2]);
    short count = (short)std::stoi(args[3]);
    if (count < 1) count = 1;
    if (count > 200) count = 200;

    ::peer *target = find_peer_by_growid(target_name);
    if (!target) { on::ConsoleMessage(event.peer, std::format("Player `w{}`` is not online.", target_name)); return; }

    ENetPeer *tp = find_enet_peer(target);
    if (!tp) { on::ConsoleMessage(event.peer, "Target peer not found."); return; }

    /* create a fake event for the target peer */
    ENetEvent fake{ .peer = tp };
    modify_item_inventory(fake, ::slot(item_id, count));
    on::ConsoleMessage(event.peer, std::format("Gave `w{}x {}`` to `w{}``.", count, item_id, target_name));
    on::ConsoleMessage(tp, std::format("You received `w{}x {}`` from `{}{}``.", count, item_id, pPeer->prefix, pPeer->growid));
    audit_log_write(pPeer->user_id, "give", target->user_id,
        std::format("item_id={} count={}", item_id, count));
}

/* ═══════ /setgems {growid} {amount} ═══════ */
void command::setgems(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR) { on::ConsoleMessage(event.peer, "`4You don't have permission."); return; }

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 3) { on::ConsoleMessage(event.peer, "Usage: /setgems `w{growid} {amount}`"); return; }

    std::string target_name = args[1];
    signed amount = std::stoi(args[2]);
    if (amount < 0) amount = 0;

    ::peer *target = find_peer_by_growid(target_name);
    if (!target) { on::ConsoleMessage(event.peer, std::format("Player `w{}`` is not online.", target_name)); return; }

    target->gems = amount;
    ENetPeer *tp = find_enet_peer(target);
    if (tp) { ENetEvent fake{ .peer = tp }; on::SetBux(fake); }
    target->mysql_update<signed>("gems", target->gems);
    on::ConsoleMessage(event.peer, std::format("Set `w{}``'s gems to `w{}``.", target_name, amount));
    audit_log_write(pPeer->user_id, "setgems", target->user_id, std::format("gems={}", amount));
}

/* ═══════ /ban {growid} ═══════ */
void command::ban(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR) { on::ConsoleMessage(event.peer, "`4You don't have permission."); return; }

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /ban `w{growid}`"); return; }

    std::string target_name = args[1];
    /* update DB */
    ::hStmt hStmt{ "UPDATE peer SET banned = 1 WHERE growid = ?" };
    MYSQL_BIND param = make_bind_in(target_name);
    mysql_stmt_bind_param(hStmt.pStmt, &param);
    mysql_stmt_execute(hStmt.pStmt);

    /* kick if online */
    ::peer *target = find_peer_by_growid(target_name);
    if (target)
    {
        ENetPeer *tp = find_enet_peer(target);
        if (tp)
        {
            on::ConsoleMessage(tp, "`4You have been banned.");
            enet_peer_disconnect_later(tp, 0);
        }
    }
    on::ConsoleMessage(event.peer, std::format("Banned `w{}``.", target_name));
    audit_log_write(pPeer->user_id, "ban", target ? target->user_id : 0, std::format("target={}", target_name));
}

/* ═══════ /unban {growid} ═══════ */
void command::unban(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR) { on::ConsoleMessage(event.peer, "`4You don't have permission."); return; }

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /unban `w{growid}`"); return; }

    std::string target_name = args[1];
    ::hStmt hStmt{ "UPDATE peer SET banned = 0 WHERE growid = ?" };
    MYSQL_BIND param = make_bind_in(target_name);
    mysql_stmt_bind_param(hStmt.pStmt, &param);
    mysql_stmt_execute(hStmt.pStmt);

    on::ConsoleMessage(event.peer, std::format("Unbanned `w{}``.", target_name));
    audit_log_write(pPeer->user_id, "unban", 0, std::format("target={}", target_name));
}

/* ═══════ /mute {growid} ═══════ */
void command::mute(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR) { on::ConsoleMessage(event.peer, "`4You don't have permission."); return; }

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /mute `w{growid}`"); return; }

    std::string target_name = args[1];
    ::hStmt hStmt{ "UPDATE peer SET muted = 1 WHERE growid = ?" };
    MYSQL_BIND param = make_bind_in(target_name);
    mysql_stmt_bind_param(hStmt.pStmt, &param);
    mysql_stmt_execute(hStmt.pStmt);

    ::peer *target = find_peer_by_growid(target_name);
    if (target) target->muted = true;

    on::ConsoleMessage(event.peer, std::format("Muted `w{}``.", target_name));
    audit_log_write(pPeer->user_id, "mute", target ? target->user_id : 0, std::format("target={}", target_name));
}

/* ═══════ /unmute {growid} ═══════ */
void command::unmute(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR) { on::ConsoleMessage(event.peer, "`4You don't have permission."); return; }

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /unmute `w{growid}`"); return; }

    std::string target_name = args[1];
    ::hStmt hStmt{ "UPDATE peer SET muted = 0 WHERE growid = ?" };
    MYSQL_BIND param = make_bind_in(target_name);
    mysql_stmt_bind_param(hStmt.pStmt, &param);
    mysql_stmt_execute(hStmt.pStmt);

    ::peer *target = find_peer_by_growid(target_name);
    if (target) target->muted = false;

    on::ConsoleMessage(event.peer, std::format("Unmuted `w{}``.", target_name));
    audit_log_write(pPeer->user_id, "unmute", target ? target->user_id : 0, std::format("target={}", target_name));
}

/* ═══════ /kick {growid} ═══════ */
void command::kick(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < MODERATOR) { on::ConsoleMessage(event.peer, "`4You don't have permission."); return; }

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2) { on::ConsoleMessage(event.peer, "Usage: /kick `w{growid}`"); return; }

    std::string target_name = args[1];
    ::peer *target = find_peer_by_growid(target_name);
    if (!target) { on::ConsoleMessage(event.peer, std::format("Player `w{}`` is not online.", target_name)); return; }

    /* don't kick staff unless you're dev */
    if (target->role >= MODERATOR && pPeer->role < DEVELOPER)
    { on::ConsoleMessage(event.peer, "`4You cannot kick a staff member."); return; }

    ENetPeer *tp = find_enet_peer(target);
    if (tp)
    {
        on::ConsoleMessage(tp, "`4You have been kicked.");
        enet_peer_disconnect_later(tp, 0);
    }
    on::ConsoleMessage(event.peer, std::format("Kicked `w{}``.", target_name));
    audit_log_write(pPeer->user_id, "kick", target->user_id, std::format("target={}", target_name));
}

/* ═══════ /setrole {growid} {role} ═══════ */
void command::setrole(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->role < DEVELOPER) { on::ConsoleMessage(event.peer, "`4Only developers can set roles."); return; }

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 3) { on::ConsoleMessage(event.peer, "Usage: /setrole `w{growid} {0=player|1=mod|2=dev}`"); return; }

    std::string target_name = args[1];
    int new_role = std::stoi(args[2]);
    if (new_role < 0 || new_role > 2) { on::ConsoleMessage(event.peer, "`4Invalid role. Use 0=player, 1=mod, 2=dev."); return; }

    /* update DB */
    ::hStmt hStmt{ "UPDATE peer SET role = ? WHERE growid = ?" };
    MYSQL_BIND params[2] = {
        make_bind_in(new_role),
        make_bind_in(target_name)
    };
    mysql_stmt_bind_param(hStmt.pStmt, params);
    mysql_stmt_execute(hStmt.pStmt);

    /* update online peer */
    ::peer *target = find_peer_by_growid(target_name);
    if (target)
    {
        target->role = (u_char)new_role;
        target->prefix = (target->role == MODERATOR) ? "#@" : (target->role == DEVELOPER) ? "8@" : std::string("w");
        ENetPeer *tp = find_enet_peer(target);
        if (tp) { ENetEvent fake{ .peer = tp }; on::NameChanged(fake); }
    }
    on::ConsoleMessage(event.peer, std::format("Set `w{}``'s role to `w{}``.", target_name, new_role));
    audit_log_write(pPeer->user_id, "setrole", target ? target->user_id : 0, 
        std::format("target={} role={}", target_name, new_role));
}
