#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

#include "achievements.hpp"

/* ═══════ Stats CSV helpers ═══════
 * Stats stored in peer.stats CSV: stat_name:value;stat_name:value;...
 */

struct StatEntry { std::string name; int value; };

static std::vector<StatEntry> parse_stats(const std::string &csv)
{
    std::vector<StatEntry> out;
    if (csv.empty()) return out;

    std::vector<std::string> entries = readch(csv, ';');
    for (const auto &entry : entries)
    {
        if (entry.empty()) continue;
        std::vector<std::string> kv = readch(entry, ':');
        if (kv.size() >= 2)
            out.push_back({ kv[0], atoi(kv[1].c_str()) });
    }
    return out;
}

static int get_stat(const std::vector<StatEntry> &stats, const std::string &name, int default_val = 0)
{
    for (const auto &s : stats)
        if (s.name == name) return s.value;
    return default_val;
}

static void set_stat(std::vector<StatEntry> &stats, const std::string &name, int value)
{
    for (auto &s : stats)
        if (s.name == name) { s.value = value; return; }
    stats.push_back({ name, value });
}

static std::string serialize_stats(const std::vector<StatEntry> &stats)
{
    std::string out;
    for (std::size_t i = 0; i < stats.size(); ++i)
    {
        if (i > 0) out += ";";
        out += std::format("{}:{}", stats[i].name, stats[i].value);
    }
    return out;
}

/* build a progress bar string: [████░░░░] 50/100 */
static std::string progress_bar(int current, int target)
{
    int pct = (target <= 0) ? 100 : std::min(100, (current * 100) / target);
    int filled = pct / 10;
    std::string bar = "`2";
    for (int i = 0; i < 10; ++i)
        bar += (i < filled) ? "█" : "`4░";
    bar += "``";
    return std::format("{} {}/{}", bar, std::min(current, target), target);
}

/* ═══════ /achievements ═══════ */
void achievements(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* load stats from DB */
    std::string stats_csv = pPeer->mysql_select<std::string>("stats");
    if (auto pos = stats_csv.find('\0'); pos != std::string::npos) stats_csv.resize(pos);

    std::vector<StatEntry> stats = parse_stats(stats_csv);

    /* ensure all known stats exist */
    static constexpr std::array stat_names = {
        "blocks_broken", "blocks_placed", "gems_earned",
        "items_dropped", "worlds_visited", "level_reached", "fires_removed"
    };
    for (const char *name : stat_names)
        set_stat(stats, name, get_stat(stats, name, 0));

    /* sync fires_removed and level from live peer data */
    set_stat(stats, "fires_removed", pPeer->fires_removed);
    set_stat(stats, "level_reached", pPeer->level.front());

    /* save back to DB in case stats changed */
    pPeer->mysql_update<std::string>("stats", serialize_stats(stats));

    /* achievement definitions: name, description, stat_name, target */
    struct Achievement { const char *name; const char *desc; const char *stat; int target; };
    static constexpr std::array<Achievement, 7> achievements_list = {{
        { "First Block",  "Break your first block",         "blocks_broken",  1 },
        { "Digger",       "Break 100 blocks",               "blocks_broken",  100 },
        { "Builder",      "Place 100 blocks",               "blocks_placed",  100 },
        { "Rich",         "Earn 1000 gems total",           "gems_earned",    1000 },
        { "Explorer",     "Visit 10 different worlds",      "worlds_visited", 10 },
        { "Survivor",     "Reach level 10",                 "level_reached",  10 },
        { "Firefighter",  "Put out 10 fires",               "fires_removed",  10 }
    }};

    int unlocked = 0;
    std::string ach_list;
    for (const auto &ach : achievements_list)
    {
        int current = get_stat(stats, ach.stat, 0);
        bool done = current >= ach.target;
        if (done) ++unlocked;

        const char *icon = done ? "`2" : "`o";
        std::string status = done ? "`2✔ COMPLETE``" : progress_bar(current, ach.target);

        ach_list += std::format(
            "add_label_with_icon|small|{}{} ({}/{})``|left|{}|\n"
            "add_textbox|  `o{}``|left|\n"
            "add_smalltext|  {}|\n"
            "add_spacer|small|\n",
            icon, ach.name, done ? 1 : 0, 1, done ? 5024 : 0,
            ach.desc,
            status
        );
    }

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wAchievements ({}/{})``|left|5024|\n"
            "add_spacer|small|\n"
            "{}"
            "add_spacer|small|\n"
            "end_dialog|achievements|Close|OK|\n"
            "add_quick_exit|\n",
            unlocked, (int)achievements_list.size(),
            ach_list
        )
    });
}
