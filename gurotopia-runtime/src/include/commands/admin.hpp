#pragma once

namespace command
{
    extern void give(ENetEvent& event, const std::string_view text);
    extern void setgems(ENetEvent& event, const std::string_view text);
    extern void ban(ENetEvent& event, const std::string_view text);
    extern void unban(ENetEvent& event, const std::string_view text);
    extern void mute(ENetEvent& event, const std::string_view text);
    extern void unmute(ENetEvent& event, const std::string_view text);
    extern void kick(ENetEvent& event, const std::string_view text);
    extern void setrole(ENetEvent& event, const std::string_view text);
}
