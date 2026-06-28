#include "pch.hpp"
#include "action/respawn.hpp"
#include "automate/hardening.hpp"
#include "on/ConsoleMessage.hpp"

#include "movement.hpp"

void movement(ENetEvent& event, state state) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* anti-cheat: detect speed hack - reset position if detected */
    if (anticheat_speedhack(event, state))
    {
        /* flag the player and reset position */
        state.pos = pPeer->pos;
        on::ConsoleMessage(event.peer, "`4Anti-Cheat: Abnormal movement detected. Position reset.``");
    }

    pPeer->pos = state.pos;
    pPeer->facing_left = state.peer_state & peer_state::S_MOVE_LEFT;

    /* add fireproof only take away 1 hp instead of 2 */
    if (state.peer_state & peer_state::S_LAVA_HIT) pPeer->pain_hp -= 2;
    if (pPeer->pain_hp <= 0) action::respawn(event, ""), pPeer->pain_hp = 10;
    
    state.netid = pPeer->netid;
    state_visuals(*event.peer, std::move(state)); // finished.
}
