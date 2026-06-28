#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

#include "guild.hpp"

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

/* ═══════ /guild ═══════ */
void guild(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2)
    {
        on::ConsoleMessage(event.peer, "Usage: /guild `w{create|invite|leave|info|motd} [args]``");
        return;
    }

    std::string sub = args[1];

    /* ── /guild create {name} ── */
    if (sub == "create")
    {
        if (args.size() < 3)
        { on::ConsoleMessage(event.peer, "Usage: /guild create `w{name}``"); return; }

        std::string guild_name = args[2];
        if (pPeer->guild_id != 0)
        { on::ConsoleMessage(event.peer, "`4You are already in a guild. Leave first."); return; }

        /* check name uniqueness */
        {
            ::hStmt hCheck{ "SELECT 1 FROM guild WHERE name = ? LIMIT 1" };
            MYSQL_BIND param = make_bind_in(guild_name);
            mysql_stmt_bind_param(hCheck.pStmt, &param);
            mysql_stmt_execute(hCheck.pStmt);
            if (!mysql_stmt_store_result(hCheck.pStmt) && mysql_stmt_num_rows(hCheck.pStmt) > 0)
            { on::ConsoleMessage(event.peer, std::format("`4Guild `w{}`` already exists.", guild_name)); return; }
        }

        /* INSERT INTO guild */
        ::hStmt hInsert{ "INSERT INTO guild (name, leader) VALUES (?, ?)" };
        MYSQL_BIND params[2] = {
            make_bind_in(guild_name),
            make_bind_in(pPeer->user_id)
        };
        mysql_stmt_bind_param(hInsert.pStmt, params);
        mysql_stmt_execute(hInsert.pStmt);

        /* get the insert_id */
        int guild_id = (int)mysql_stmt_insert_id(hInsert.pStmt);

        /* UPDATE peer SET guild_id */
        pPeer->guild_id = guild_id;
        pPeer->mysql_update<signed>("guild_id", guild_id);

        on::ConsoleMessage(event.peer, std::format("`2Guild `w{}`` created! You are the leader.", guild_name));
    }

    /* ── /guild invite {growid} ── */
    else if (sub == "invite")
    {
        if (args.size() < 3)
        { on::ConsoleMessage(event.peer, "Usage: /guild invite `w{growid}``"); return; }

        if (pPeer->guild_id == 0)
        { on::ConsoleMessage(event.peer, "`4You are not in a guild."); return; }

        std::string target_name = args[2];
        ENetPeer *target_enet = nullptr;
        ::peer *target = find_peer_by_growid(target_name, &target_enet);
        if (!target || !target_enet)
        { on::ConsoleMessage(event.peer, std::format("`4Player `w{}`` is not online.", target_name)); return; }

        if (target->guild_id != 0)
        { on::ConsoleMessage(event.peer, std::format("`4`w{}`` is already in a guild.", target_name)); return; }

        /* send invite dialog to target */
        send_varlist(target_enet, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wGuild Invite``|left|5024|\n"
                "add_spacer|small|\n"
                "add_textbox|`{}{}`` has invited you to join their guild. Accept?|left|\n"
                "add_spacer|small|\n"
                "add_button|guild_invite_accept|`2Accept``|noflags|0|0|\n"
                "add_button|guild_invite_decline|`4Decline``|noflags|0|0|\n"
                "end_dialog|guild_invite|Close|Cancel|\n"
                "add_quick_exit|\n",
                pPeer->prefix, pPeer->growid
            )
        });

        on::ConsoleMessage(event.peer, std::format("`oSent guild invite to `w{}``.", target_name));
    }

    /* ── /guild leave ── */
    else if (sub == "leave")
    {
        if (pPeer->guild_id == 0)
        { on::ConsoleMessage(event.peer, "`4You are not in a guild."); return; }

        int old_guild = pPeer->guild_id;

        /* check if leader — if so, disband */
        {
            int leader = 0;
            ::hStmt hCheck{ "SELECT leader FROM guild WHERE id = ? LIMIT 1" };
            MYSQL_BIND param = make_bind_in(old_guild);
            mysql_stmt_bind_param(hCheck.pStmt, &param);
            MYSQL_BIND result = make_bind_out(leader);
            mysql_stmt_bind_result(hCheck.pStmt, &result);
            mysql_stmt_execute(hCheck.pStmt);
            mysql_stmt_fetch(hCheck.pStmt);

            if (leader == pPeer->user_id)
            {
                /* leader leaves — disband guild */
                ::hStmt hDel{ "DELETE FROM guild WHERE id = ?" };
                MYSQL_BIND del_param = make_bind_in(old_guild);
                mysql_stmt_bind_param(hDel.pStmt, &del_param);
                mysql_stmt_execute(hDel.pStmt);

                /* set all members' guild_id to 0 */
                ::hStmt hClear{ "UPDATE peer SET guild_id = 0 WHERE guild_id = ?" };
                MYSQL_BIND clear_param = make_bind_in(old_guild);
                mysql_stmt_bind_param(hClear.pStmt, &clear_param);
                mysql_stmt_execute(hClear.pStmt);

                /* update online members */
                for (ENetPeer &p : std::span(host->peers, host->peerCount))
                    if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
                    {
                        ::peer *pp = static_cast<::peer*>(p.data);
                        if (pp->guild_id == old_guild)
                        {
                            pp->guild_id = 0;
                            on::ConsoleMessage(&p, "`4Your guild has been disbanded.");
                        }
                    }

                on::ConsoleMessage(event.peer, "`2Guild disbanded.");
                return;
            }
        }

        pPeer->guild_id = 0;
        pPeer->mysql_update<signed>("guild_id", 0);
        on::ConsoleMessage(event.peer, "`2You left the guild.");
    }

    /* ── /guild info ── */
    else if (sub == "info")
    {
        if (pPeer->guild_id == 0)
        { on::ConsoleMessage(event.peer, "`4You are not in a guild."); return; }

        /* fetch guild info */
        std::string g_name, g_motd;
        int g_leader{};
        {
            ::hStmt hInfo{ "SELECT name, leader, motd FROM guild WHERE id = ? LIMIT 1" };
            MYSQL_BIND param = make_bind_in(pPeer->guild_id);
            mysql_stmt_bind_param(hInfo.pStmt, &param);

            MYSQL_BIND results[3] = {
                make_bind_out(g_name),
                make_bind_out(g_leader),
                make_bind_out(g_motd)
            };
            mysql_stmt_bind_result(hInfo.pStmt, results);
            mysql_stmt_execute(hInfo.pStmt);
            if (mysql_stmt_fetch(hInfo.pStmt) != 0)
            { on::ConsoleMessage(event.peer, "`4Guild not found."); return; }

            auto trim_null = [](std::string &s) {
                if (auto pos = s.find('\0'); pos != std::string::npos) s.resize(pos);
            };
            trim_null(g_name);
            trim_null(g_motd);
        }

        /* fetch members */
        std::string member_list;
        {
            ::hStmt hMembers{ "SELECT growid FROM peer WHERE guild_id = ?" };
            MYSQL_BIND param = make_bind_in(pPeer->guild_id);
            mysql_stmt_bind_param(hMembers.pStmt, &param);

            std::string member_name;
            MYSQL_BIND result = make_bind_out(member_name);
            mysql_stmt_bind_result(hMembers.pStmt, &result);
            mysql_stmt_execute(hMembers.pStmt);
            mysql_stmt_store_result(hMembers.pStmt);

            while (!mysql_stmt_fetch(hMembers.pStmt))
            {
                if (auto pos = member_name.find('\0'); pos != std::string::npos) member_name.resize(pos);
                member_list += std::format("add_textbox|`w{}``|left|\n", member_name);
                member_name.assign(1024, '\0');
            }
        }

        if (member_list.empty()) member_list = "add_textbox|`oNo members found.``|left|\n";

        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wGuild: {}``|left|5024|\n"
                "add_spacer|small|\n"
                "add_textbox|`oLeader: `wUID #{}``|left|\n"
                "add_textbox|`oMOTD: `w{}``|left|\n"
                "add_spacer|small|\n"
                "add_label|small|`$Members:``|left|\n"
                "{}"
                "add_spacer|small|\n"
                "end_dialog|guild_info|Close|OK|\n"
                "add_quick_exit|\n",
                g_name,
                g_leader,
                g_motd.empty() ? "No message set" : g_motd,
                member_list
            )
        });
    }

    /* ── /guild motd {text} ── */
    else if (sub == "motd")
    {
        if (pPeer->guild_id == 0)
        { on::ConsoleMessage(event.peer, "`4You are not in a guild."); return; }

        /* verify leader */
        int leader = 0;
        {
            ::hStmt hCheck{ "SELECT leader FROM guild WHERE id = ? LIMIT 1" };
            MYSQL_BIND param = make_bind_in(pPeer->guild_id);
            mysql_stmt_bind_param(hCheck.pStmt, &param);
            MYSQL_BIND result = make_bind_out(leader);
            mysql_stmt_bind_result(hCheck.pStmt, &result);
            mysql_stmt_execute(hCheck.pStmt);
            mysql_stmt_fetch(hCheck.pStmt);
        }
        if (leader != pPeer->user_id)
        { on::ConsoleMessage(event.peer, "`4Only the guild leader can set the MOTD."); return; }

        /* extract motd text (everything after "guild motd ") */
        std::string motd_text;
        std::size_t pos = std::string(text).find("motd ");
        if (pos != std::string::npos)
            motd_text = std::string(text).substr(pos + 5);
        if (motd_text.empty())
        { on::ConsoleMessage(event.peer, "Usage: /guild motd `w{text}``"); return; }

        ::hStmt hUpdate{ "UPDATE guild SET motd = ? WHERE id = ?" };
        MYSQL_BIND params[2] = {
            make_bind_in(motd_text),
            make_bind_in(pPeer->guild_id)
        };
        mysql_stmt_bind_param(hUpdate.pStmt, params);
        mysql_stmt_execute(hUpdate.pStmt);

        on::ConsoleMessage(event.peer, std::format("`2Guild MOTD updated to: `w{}``", motd_text));
    }

    else
    {
        on::ConsoleMessage(event.peer, "Usage: /guild `w{create|invite|leave|info|motd} [args]``");
    }
}
