#include "pch.hpp"
#include "systems.hpp"
#include "database/database.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/SetBux.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include <chrono>
#include <ctime>

using namespace std::chrono;

/* ---- helper: find online peer by growid ---- */
static ::peer* find_online_peer(const std::string& growid)
{
    for (ENetPeer* p : peers("", PEER_ALL))
    {
        ::peer* pp = static_cast<::peer*>(p->data);
        if (pp && pp->growid == growid) return pp;
    }
    return nullptr;
}

/* ====== CASINO ====== */
void command::casino(ENetEvent& event, const std::string_view text)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);
    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 3)
    {
        send_varlist(event.peer, {"OnDialogRequest",
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wCasino``|left|758|\n"
            "add_spacer|small|\n"
            "add_textbox|`oWelcome to the Casino! Pick a game:``|left|\n"
            "add_spacer|small|\n"
            "add_textbox|`oUsage: /casino {coin|dice|wheel} {bet}``\n"
            "add_spacer|small|\n"
            "add_textbox|`oCoin Flip — 2x payout (50% win)``\n"
            "add_textbox|`oDice Roll — 6x payout (16.7% win)``\n"
            "add_textbox|`oRoulette — 36x payout (2.8% win)``\n"
            "add_spacer|small|\n"
            "add_textbox|`oYou have `w" + std::to_string(pPeer->gems) + "`` gems.``\n"
            "add_quick_exit|\n"
            "end_dialog|popup||Close|\n"
        });
        return;
    }
    std::string game = args[1];
    int bet = atoi(args[2].c_str());
    if (bet < 1 || bet > pPeer->gems)
    {
        on::ConsoleMessage(event.peer, "`oInvalid bet.");
        return;
    }
    std::string msg;
    if (game == "coin")
    {
        bool win = rand() % 2;
        if (win) { pPeer->gems += bet; msg = "`2You won " + std::to_string(bet) + " gems! (Coin flip)"; }
        else { pPeer->gems -= bet; msg = "`4You lost " + std::to_string(bet) + " gems. (Coin flip)"; }
    }
    else if (game == "dice")
    {
        int roll = (rand() % 6) + 1;
        if (roll == 6) { pPeer->gems += bet * 5; msg = "`2Rolled a 6! You won " + std::to_string(bet * 5) + " gems!"; }
        else { pPeer->gems -= bet; msg = "`4Rolled a " + std::to_string(roll) + ". You lost " + std::to_string(bet) + " gems."; }
    }
    else if (game == "wheel")
    {
        int num = rand() % 37;
        if (num == 0) { pPeer->gems += bet * 35; msg = "`2Hit 0! You won " + std::to_string(bet * 35) + " gems!"; }
        else { pPeer->gems -= bet; msg = "`4Wheel landed on " + std::to_string(num) + ". You lost " + std::to_string(bet) + " gems."; }
    }
    else { on::ConsoleMessage(event.peer, "`oUnknown game. Use coin, dice, or wheel."); return; }
    on::ConsoleMessage(event.peer, msg);
    on::SetBux(event);
    pPeer->mysql_update<signed>("gems", pPeer->gems);
}

/* ====== LEADERBOARD ====== */
void command::leaderboard(ENetEvent& event, const std::string_view text)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);
    ::hStmt hStmt{ "SELECT growid, gems, level FROM peer ORDER BY gems DESC LIMIT 10" };
    int idx = 0;
    std::string rows;
    MYSQL_BIND bind[3];
    std::string gid; gid.resize(1024, '\0'); int gems{}; int lvl{};
    bind[0] = make_bind_out(gid);
    bind[1] = make_bind_out(gems);
    bind[2] = make_bind_out(lvl);
    mysql_stmt_bind_result(hStmt.pStmt, bind);
    mysql_stmt_execute(hStmt.pStmt);
    mysql_stmt_store_result(hStmt.pStmt);
    while (!mysql_stmt_fetch(hStmt.pStmt))
    {
        std::string name(gid);
        auto n = name.find('\0');
        if (n != std::string::npos) name = name.substr(0, n);
        rows += std::format("add_textbox|`w{}. {} — `2{} gems`o, `wlevel {}``|\n", ++idx, name, gems, lvl);
    }
    send_varlist(event.peer, {"OnDialogRequest",
        "set_default_color|`o\n"
        "add_label_with_icon|big|`wLeaderboard — Top Players``|left|1366|\n"
        "add_spacer|small|\n" + rows +
        "add_spacer|small|\n"
        "add_quick_exit|\n"
        "end_dialog|popup||Close|\n"
    });
}

/* ====== BOUNTY ====== */
void command::bounty(ENetEvent& event, const std::string_view text)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);
    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 3)
    {
        on::ConsoleMessage(event.peer, "`oUsage: /bounty {growid} {gems_amount}");
        return;
    }
    std::string target = args[1];
    int amount = atoi(args[2].c_str());
    if (amount < 1 || amount > pPeer->gems)
    {
        on::ConsoleMessage(event.peer, "`oInvalid bounty amount.");
        return;
    }
    pPeer->gems -= amount;
    on::SetBux(event);
    pPeer->mysql_update<signed>("gems", pPeer->gems);
    std::string msg = std::format("`4Bounty placed! ${}`` gems on `w{}`` by `w{}``", amount, target, pPeer->growid);
    peers("", PEER_ALL, [&msg](ENetPeer& p){
        send_action(p, "log", msg.c_str());
    });
    on::ConsoleMessage(event.peer, "`oBounty posted! Kill them in a duel to claim.");
}

/* ====== REFER A FRIEND ====== */
void command::refer(ENetEvent& event, const std::string_view text)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);
    std::vector<std::string> args = readch(std::string(text), ' ');
    if (args.size() < 2)
    {
        on::ConsoleMessage(event.peer, "`oUsage: /refer {growid} — refer a friend for bonus gems!");
        return;
    }
    std::string referrer = args[1];
    ::hStmt hStmt{ "SELECT uid FROM peer WHERE growid = ?" };
    MYSQL_BIND in[1];
    make_bind_in(referrer);
    mysql_stmt_bind_param(hStmt.pStmt, in);
    mysql_stmt_execute(hStmt.pStmt);
    int ref_uid{};
    MYSQL_BIND out[1] = { make_bind_out(ref_uid) };
    mysql_stmt_bind_result(hStmt.pStmt, out);
    if (mysql_stmt_fetch(hStmt.pStmt))
    {
        on::ConsoleMessage(event.peer, "`oThat player doesn't exist.");
        return;
    }
    pPeer->gems += 500;
    on::SetBux(event);
    pPeer->mysql_update<signed>("gems", pPeer->gems);
    ::hStmt up{ "UPDATE peer SET gems = gems + 500 WHERE uid = ?" };
    MYSQL_BIND uin[1];
    make_bind_in(ref_uid);
    mysql_stmt_bind_param(up.pStmt, uin);
    mysql_stmt_execute(up.pStmt);
    on::ConsoleMessage(event.peer, "`2Referral successful! You both got 500 gems!");
    if (auto* tp = find_online_peer(referrer))
    {
        ENetEvent e2{}; e2.peer = nullptr;
        /* notify if online */
    }
}
