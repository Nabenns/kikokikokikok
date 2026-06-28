#pragma once

namespace command
{
    extern void surgery(ENetEvent& event, const std::string_view text);
    extern void race(ENetEvent& event, const std::string_view text);
    extern void duel(ENetEvent& event, const std::string_view text);
    extern void guildwar(ENetEvent& event, const std::string_view text);
    extern void broadcast(ENetEvent& event, const std::string_view text);
}
