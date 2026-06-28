#include "pch.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"

#include "guild_invite.hpp"

/* ═══════ guild_invite dialog return ═══════ */
void guild_invite(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    std::string btn = hPipe["buttonClicked"];

    if (btn == "guild_invite_accept")
    {
        if (pPeer->guild_id != 0)
        { on::ConsoleMessage(event.peer, "`4You are already in a guild."); return; }

        /* find any guild that has an online leader who invited us —
           we search for the leader who is online and not in a guild already */
        int best_guild_id = 0;
        std::string best_leader_name;

        for (ENetPeer &p : std::span(host->peers, host->peerCount))
        {
            if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
            {
                ::peer *pp = static_cast<::peer*>(p.data);
                if (pp->guild_id == 0) continue;

                /* check if this peer is the leader of their guild */
                int leader = 0;
                ::hStmt hCheck{ "SELECT leader, name FROM guild WHERE id = ? LIMIT 1" };
                MYSQL_BIND param = make_bind_in(pp->guild_id);
                mysql_stmt_bind_param(hCheck.pStmt, &param);

                std::string g_name;
                MYSQL_BIND results[2] = {
                    make_bind_out(leader),
                    make_bind_out(g_name)
                };
                mysql_stmt_bind_result(hCheck.pStmt, results);
                mysql_stmt_execute(hCheck.pStmt);
                if (mysql_stmt_fetch(hCheck.pStmt) == 0 && leader == pp->user_id)
                {
                    best_guild_id = pp->guild_id;
                    if (auto pos = g_name.find('\0'); pos != std::string::npos) g_name.resize(pos);
                    best_leader_name = pp->growid;
                    on::ConsoleMessage(&p, std::format("`2`w{}`` joined your guild!", pPeer->growid));
                    break;
                }
            }
        }

        if (best_guild_id == 0)
        { on::ConsoleMessage(event.peer, "`4Guild invite is no longer valid."); return; }

        pPeer->guild_id = best_guild_id;
        pPeer->mysql_update<signed>("guild_id", best_guild_id);

        on::ConsoleMessage(event.peer, std::format("`2You joined the guild led by `w{}``!", best_leader_name));
    }
    else /* guild_invite_decline or close */
    {
        /* notify any online leader that the invite was declined */
        for (ENetPeer &p : std::span(host->peers, host->peerCount))
        {
            if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
            {
                ::peer *pp = static_cast<::peer*>(p.data);
                if (pp->guild_id == 0) continue;

                int leader = 0;
                ::hStmt hCheck{ "SELECT leader FROM guild WHERE id = ? LIMIT 1" };
                MYSQL_BIND param = make_bind_in(pp->guild_id);
                mysql_stmt_bind_param(hCheck.pStmt, &param);
                MYSQL_BIND result = make_bind_out(leader);
                mysql_stmt_bind_result(hCheck.pStmt, &result);
                mysql_stmt_execute(hCheck.pStmt);
                if (mysql_stmt_fetch(hCheck.pStmt) == 0 && leader == pp->user_id)
                {
                    on::ConsoleMessage(&p, std::format("`4`w{}`` declined your guild invite.", pPeer->growid));
                    break;
                }
            }
        }
    }
}
