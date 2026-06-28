#pragma once

namespace action
{ 
    extern void trade(ENetEvent& event, const std::string& header, const std::string_view target = "");
}
