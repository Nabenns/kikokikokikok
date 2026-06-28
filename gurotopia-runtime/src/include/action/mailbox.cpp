#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

#include "mailbox.hpp"

/* ═══════ action::mailbox ═══════ */
void action::mailbox(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* count unread mail */
    int unread = 0;
    {
        ::hStmt hStmt{ "SELECT COUNT(*) FROM mailbox WHERE recipient_uid = ? AND is_read = 0" };
        MYSQL_BIND param = make_bind_in(pPeer->user_id);
        mysql_stmt_bind_param(hStmt.pStmt, &param);
        MYSQL_BIND result = make_bind_out(unread);
        mysql_stmt_bind_result(hStmt.pStmt, &result);
        mysql_stmt_execute(hStmt.pStmt);
        mysql_stmt_fetch(hStmt.pStmt);
    }

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wMailbox``|left|6146|\n"
            "add_spacer|small|\n"
            "add_textbox|`oYou have `w{}`` unread `2mail(s)``.|left|\n"
            "add_spacer|small|\n"
            "add_button|mail_inbox|`wInbox ({})``|noflags|0|0|\n"
            "add_button|mail_compose|`wSend Mail``|noflags|0|0|\n"
            "add_spacer|small|\n"
            "end_dialog|mailbox|Close|Cancel|\n"
            "add_quick_exit|\n",
            unread, unread
        )
    });
}
