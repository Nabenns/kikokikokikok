#include "pch.hpp"
#include <map>
#include "donation_box.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/SetBux.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

/* in-memory donation tracking per world+pos */
struct donation_state {
    int total_gems{0};
    int last_donor_uid{0};
    std::string last_donor_name{};
};
static std::map<std::string, donation_state> donation_boxes;

static std::string dkey(const std::string& wname, short tx, short ty)
{
    return std::format("{}_{}_{}", wname, tx, ty);
}

void donation_box_return(ENetEvent& event, const ::hPipe& hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::string key = dkey(world->name, tilex, tiley);

    std::string amount_str = hPipe["donate_amount"];
    int amount = atoi(amount_str.c_str());
    if (amount <= 0) amount = 1;
    if (amount > pPeer->gems) amount = pPeer->gems;
    if (amount <= 0)
    {
        on::ConsoleMessage(event.peer, "`4You don't have any gems to donate!``");
        return;
    }

    pPeer->gems -= amount;
    on::SetBux(event);
    pPeer->mysql_update<signed>("gems", pPeer->gems);

    donation_boxes[key].total_gems += amount;
    donation_boxes[key].last_donor_uid = pPeer->user_id;
    donation_boxes[key].last_donor_name = pPeer->growid;

    /* credit world owner if online */
    if (world->owner)
    {
        peers("", PEER_ALL, [&world, &amount, &pPeer](ENetPeer& peer) {
            ::peer *pp = static_cast<::peer*>(peer.data);
            if (pp && pp->user_id == world->owner)
            {
                pp->gems += amount;
                ENetEvent fake{ .peer = &peer };
                on::SetBux(fake);
                pp->mysql_update<signed>("gems", pp->gems);
                on::ConsoleMessage(&peer, std::format("`2{} donated `w{} gems`` to your donation box!``",
                    pPeer->growid, amount));
            }
        });
    }

    on::ConsoleMessage(event.peer, std::format("`2You donated `w{} gems`` to the donation box!`` Thank you for your generosity!", amount));
}
