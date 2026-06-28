#include "pch.hpp"

#include "admin.hpp"
#include "database/database.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "database/shouhin.hpp"
#include "server_data.hpp"
#include "on/ConsoleMessage.hpp"

static constexpr int ITEMS_PER_PAGE = 500;

std::string json_escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() * 2);
    for (char c : s)
    {
        switch (c)
        {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                    out += std::format("\\u{:04x}", static_cast<unsigned char>(c));
                else
                    out += c;
                break;
        }
    }
    return out;
}

std::string online_json()
{
    std::string json = "{\n  \"players\": [\n";

    bool first = true;
    for (std::size_t i = 0; i < host->peerCount; ++i)
    {
        if (host->peers[i].state != ENET_PEER_STATE_CONNECTED) continue;
        if (!host->peers[i].data) continue;

        ::peer *pPeer = static_cast<::peer*>(host->peers[i].data);
        if (pPeer->growid.empty()) continue;

        if (!first) json += ",\n";
        first = false;

        std::string world = "EXIT";
        if (!pPeer->recent_worlds[5].empty()) // assume 0-4 could be old, 5 is most recent
        {
            // find the most recent non-empty world
            for (int j = 5; j >= 0; --j)
            {
                if (!pPeer->recent_worlds[j].empty())
                {
                    world = pPeer->recent_worlds[j];
                    break;
                }
            }
        }

        json += std::format(
            "    {{\n"
            "      \"uid\": {},\n"
            "      \"growid\": \"{}\",\n"
            "      \"world\": \"{}\",\n"
            "      \"netid\": {},\n"
            "      \"role\": {},\n"
            "      \"level\": {},\n"
            "      \"gems\": {}\n"
            "    }}",
            pPeer->user_id,
            pPeer->growid,
            world,
            pPeer->netid,
            static_cast<int>(pPeer->role),
            pPeer->level[0],
            pPeer->gems
        );
    }

    json += "\n  ]\n}";
    return json;
}

void broadcast_message(const std::string& admin_growid, const std::string& message)
{
    if (message.empty()) return;

    std::string formatted = std::format("`5[BROADCAST] `w{}``: {}``", admin_growid, message);

    for (std::size_t i = 0; i < host->peerCount; ++i)
    {
        if (host->peers[i].state != ENET_PEER_STATE_CONNECTED) continue;
        if (!host->peers[i].data) continue;

        ::peer *pPeer = static_cast<::peer*>(host->peers[i].data);
        if (pPeer->growid.empty()) continue;

        on::ConsoleMessage(&host->peers[i], formatted);
    }
}

std::string setrole_from_web(int target_uid, int new_role, int admin_uid)
{
    try
    {
        hStmt stmt("UPDATE peer SET role = ? WHERE uid = ?");
        MYSQL_BIND bind[2] = {};
        bind[0] = make_bind_in(new_role);
        bind[1] = make_bind_in(target_uid);
        mysql_stmt_bind_param(stmt.pStmt, bind);
        if (mysql_stmt_execute(stmt.pStmt) != 0)
        {
            return std::format("error:{}", mysql_stmt_error(stmt.pStmt));
        }

        // write audit log
        audit_log_write(admin_uid, "setrole", target_uid, std::format("role set to {}", new_role));

        return "ok";
    }
    catch (const std::exception& e)
    {
        return std::format("error:{}", e.what());
    }
}

void audit_log_write(int admin_uid, const std::string& action, int target_uid, const std::string& detail)
{
    try
    {
        hStmt stmt("INSERT INTO audit_log (admin_uid, action, target_uid, detail) VALUES (?, ?, ?, ?)");
        MYSQL_BIND bind[4] = {};
        bind[0] = make_bind_in(admin_uid);
        bind[1] = make_bind_in(action);
        bind[2] = make_bind_in(target_uid);
        bind[3] = make_bind_in(detail);
        mysql_stmt_bind_param(stmt.pStmt, bind);
        mysql_stmt_execute(stmt.pStmt);
    }
    catch (...) { /* audit log failure should not crash */ }
}

std::string items_json(int page)
{
    int start = page * ITEMS_PER_PAGE;
    int end = std::min(start + ITEMS_PER_PAGE, static_cast<int>(items.size()));
    int total_pages = (static_cast<int>(items.size()) + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;

    std::string json;
    json += std::format("{{\"page\":{},\"total_pages\":{},\"total_items\":{},\"items\":[", page, total_pages, items.size());

    for (int i = start; i < end; ++i)
    {
        const auto& im = items[i];
        if (i > start) json += ",";
        json += std::format(
            "{{\"id\":{},\"name\":\"{}\",\"prop\":{},\"cat\":{},\"type\":{},\"ingredient\":{},"
            "\"collision\":{},\"hits\":{},\"hit_reset\":{},\"cloth_type\":{},\"rarity\":{},"
            "\"tick\":{},\"info\":\"{}\",\"splice\":[{},{}]}}",
            im.id,
            json_escape(im.raw_name),
            static_cast<int>(im.property),
            static_cast<int>(im.cat),
            static_cast<int>(im.type),
            im.ingredient,
            static_cast<int>(im.collision),
            static_cast<int>(im.hits),
            im.hit_reset,
            static_cast<int>(im.cloth_type),
            static_cast<int>(im.rarity),
            im.tick,
            json_escape(im.info),
            im.splice[0],
            im.splice[1]
        );
    }

    json += "]}";
    return json;
}

std::string store_json()
{
    std::string json = "{\"store\":[";

    for (std::size_t i = 0; i < shouhin_tachi.size(); ++i)
    {
        const auto& [tab, sh] = shouhin_tachi[i];
        if (i > 0) json += ",";

        // build items array: [id,amount,id,amount,...]
        std::string items_arr = "[";
        for (std::size_t j = 0; j < sh.im.size(); ++j)
        {
            if (j > 0) items_arr += ",";
            items_arr += std::format("[{},{}]", sh.im[j].first, sh.im[j].second);
        }
        items_arr += "]";

        json += std::format(
            "{{\"tab\":{},\"btn\":\"{}\",\"name\":\"{}\",\"rttx\":\"{}\","
            "\"desc\":\"{}\",\"tex1\":{},\"tex2\":{},\"cost\":{},\"items\":{}}}",
            tab,
            json_escape(sh.btn),
            json_escape(sh.name),
            json_escape(sh.rttx),
            json_escape(sh.description),
            static_cast<int>(sh.tex1),
            static_cast<int>(sh.tex2),
            sh.cost,
            items_arr
        );
    }

    json += "]}";
    return json;
}

std::string config_json()
{
    return std::format(
        "{{\"server\":\"{}\",\"port\":{},\"type\":{},\"type2\":{},\"maint\":\"{}\","
        "\"loginurl\":\"{}\",\"meta\":\"{}\"}}",
        json_escape(gServer_data.server),
        gServer_data.port,
        static_cast<int>(gServer_data.type),
        static_cast<int>(gServer_data.type2),
        json_escape(gServer_data.maint),
        json_escape(gServer_data.loginurl),
        json_escape(gServer_data.meta)
    );
}