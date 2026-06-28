#include "pch.hpp"
#include "donation_box.hpp"
#include "database/database.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/SetBux.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"

void action::donation_box(ENetEvent& event, const std::string& header)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);
    ::hPipe hPipe{ header };
    std::string buttonClicked = hPipe["buttonClicked"];
    std::string tilex = hPipe["tilex"];
    std::string tiley = hPipe["tiley"];

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    short tx = (short)atoi(tilex.c_str());
    short ty = (short)atoi(tiley.c_str());
    ::block& block = world->blocks[cord(tx, ty)];

    if (buttonClicked.empty())
    {
        /* wrench open — show dialog */
        send_varlist(event.peer, {"OnDialogRequest",
            std::format("set_default_color|`o\n"
                "add_label_with_icon|big|`wDonation Box``|left|1276|\n"
                "add_spacer|small|\n"
                "add_textbox|`oDonate gems to the world owner.``|left|\n"
                "add_spacer|small|\n"
                "add_text_input|donate_amount|Amount|1|5|\n"
                "embed_data|tilex|{}\n"
                "embed_data|tiley|{}\n"
                "add_button|donate|`2Donate``|noflags|0|0|\n"
                "add_quick_exit|\n"
                "end_dialog|popup||Close|\n", tx, ty)
        });
        return;
    }

    if (buttonClicked == "donate")
    {
        int amount = atoi(hPipe["donate_amount"].c_str());
        if (amount < 1 || amount > pPeer->gems)
        {
            on::ConsoleMessage(event.peer, "`oInvalid donation amount.");
            return;
        }
        pPeer->gems -= amount;
        on::SetBux(event);
        pPeer->mysql_update<signed>("gems", pPeer->gems);

        /* give gems to world owner if online */
        if (world->owner)
        {
            for (ENetPeer* p : peers("", PEER_ALL))
            {
                ::peer* tp = static_cast<::peer*>(p->data);
                if (tp && tp->user_id == world->owner)
                {
                    tp->gems += amount;
                    ENetEvent e2{};
                    e2.peer = p;
                    on::SetBux(e2);
                    tp->mysql_update<signed>("gems", tp->gems);
                    on::ConsoleMessage(p, std::format("`2{} donated {} gems to you!", pPeer->growid, amount));
                    break;
                }
            }
        }
        on::ConsoleMessage(event.peer, std::format("`oYou donated {} gems. Thank you for your generosity!", amount));
    }
}
