#include "pch.hpp"
#include "on/ConsoleMessage.hpp"

#include "title_select.hpp"

void title_select(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::string clicked = hPipe["buttonClicked"];
    if (clicked.empty()) return;

    /* button IDs are prefixed with "title_" */
    if (clicked.starts_with("title_"))
    {
        std::string title = clicked.substr(6); /* strip "title_" prefix */
        pPeer->title = title;
        pPeer->mysql_update<std::string>("title", pPeer->title);
        on::ConsoleMessage(event.peer, std::format("You equipped the title `w{}``.", title));
    }
}
