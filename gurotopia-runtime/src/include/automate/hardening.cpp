#include "pch.hpp"
#include "hardening.hpp"
#include "database/database.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include <map>

/* ═══════ Auto-save worlds ═══════ */
void autosave_worlds()
{
    for (::world &w : worlds)
    {
        try { w.mysql_save(); } catch (...) {}
    }
}

/* ═══════ Backup database ═══════ */
void backup_database()
{
    /* use mysqldump via system call - simple but effective */
    std::string cmd = "mysqldump -u root gurotopia > /root/gtps-host/backup_$(date +%Y%m%d_%H%M%S).sql 2>/dev/null";
    std::system(cmd.c_str());
}

/* ═══════ Anti-cheat: speed hack detection ═══════ */
bool anticheat_speedhack(ENetEvent& event, state& state)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return false;

    /* calculate distance moved since last position */
    float dx = state.pos.x - pPeer->pos.x;
    float dy = state.pos.y - pPeer->pos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    /* normal movement speed is ~6 tiles per packet, allow up to 15 for lag */
    if (dist > 15.0f)
    {
        /* flag as potential speed hack - reset position */
        state.pos = pPeer->pos;
        return true; /* speed hack detected */
    }

    return false;
}

/* ═══════ Rate limiter ═══════ */
bool rate_limit_check(ENetEvent& event)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return false;

    /* track packet timestamps per peer */
    struct rate_data {
        std::chrono::steady_clock::time_point window_start{};
        int packet_count{0};
    };
    static std::map<::peer*, rate_data> rate_map;

    auto& rd = rate_map[pPeer];
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - rd.window_start);

    /* reset window every second */
    if (elapsed.count() > 1000)
    {
        rd.window_start = now;
        rd.packet_count = 0;
    }

    rd.packet_count++;

    /* allow max 30 packets per second */
    if (rd.packet_count > 30)
    {
        return true; /* rate limited */
    }

    return false;
}
