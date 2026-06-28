#pragma once

namespace command
{
    extern void casino(ENetEvent& event, const std::string_view text);
    extern void leaderboard(ENetEvent& event, const std::string_view text);
    extern void bounty(ENetEvent& event, const std::string_view text);
    extern void refer(ENetEvent& event, const std::string_view text);
}
