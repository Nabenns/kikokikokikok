#pragma once

namespace command
{
    extern void daily(ENetEvent& event, const std::string_view text);
    extern void title(ENetEvent& event, const std::string_view text);
    extern void ignore(ENetEvent& event, const std::string_view text);
    extern void unignore(ENetEvent& event, const std::string_view text);
    extern void friend_add(ENetEvent& event, const std::string_view text);
    extern void friend_remove(ENetEvent& event, const std::string_view text);
    extern void friend_list(ENetEvent& event, const std::string_view text);
    extern void pet_info(ENetEvent& event, const std::string_view text);
    extern void quest_list(ENetEvent& event, const std::string_view text);
}

/* parse friends CSV → populate friends array */
extern void load_friends(::peer *pPeer);
