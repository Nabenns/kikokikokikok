#include "pch.hpp"

#include "metrics.hpp"
#include "database/world.hpp"

ServerMetrics gMetrics;

std::string metrics_json()
{
    unsigned long long uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - gMetrics.start_time
    ).count();

    // count online players
    unsigned total_players = 0;
    for (std::size_t i = 0; i < host->peerCount; ++i)
        if (host->peers[i].state == ENET_PEER_STATE_CONNECTED)
            total_players++;

    // count active worlds
    unsigned total_worlds = worlds.size();

    return std::format(
        "uptime_seconds:{}\n"
        "total_players_online:{}\n"
        "total_worlds_active:{}\n"
        "total_logins:{}\n"
        "total_blocks_broken:{}\n",
        uptime,
        total_players,
        total_worlds,
        gMetrics.total_logins,
        gMetrics.total_blocks_broken
    );
}