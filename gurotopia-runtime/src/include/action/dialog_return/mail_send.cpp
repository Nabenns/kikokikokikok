#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

#include "mail_send.hpp"

/* ═══════ mail_send dialog return ═══════ */
void mail_send(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    std::string target_growid = hPipe["mail_target"];
    std::string message = hPipe["mail_message"];
    short item_id = (short)atoi(hPipe["mail_item_id"].c_str());
    short item_count = (short)atoi(hPipe["mail_item_count"].c_str());

    if (target_growid.empty())
    { on::ConsoleMessage(event.peer, "`4Recipient GrowID cannot be empty."); return; }

    /* look up recipient uid by growid */
    int recipient_uid = 0;
    ::hStmt hLookup{ "SELECT uid FROM peer WHERE growid = ? LIMIT 1" };
    MYSQL_BIND param = make_bind_in(target_growid);
    mysql_stmt_bind_param(hLookup.pStmt, &param);
    MYSQL_BIND result = make_bind_out(recipient_uid);
    mysql_stmt_bind_result(hLookup.pStmt, &result);
    mysql_stmt_execute(hLookup.pStmt);
    if (mysql_stmt_fetch(hLookup.pStmt) != 0)
    { on::ConsoleMessage(event.peer, std::format("`4Player `w{}`` not found.", target_growid)); return; }

    /* if attaching item, remove from sender's inventory */
    if (item_id > 0 && item_count > 0)
    {
        auto it = std::ranges::find(pPeer->slots, item_id, &::slot::id);
        if (it == pPeer->slots.end() || it->count < item_count)
        { on::ConsoleMessage(event.peer, "`4You don't have enough of that item to attach."); return; }

        modify_item_inventory(event, ::slot(item_id, -item_count));
    }

    /* INSERT INTO mailbox */
    ::hStmt hInsert{
        "INSERT INTO mailbox (recipient_uid, sender_name, message, item_id, item_count) VALUES (?, ?, ?, ?, ?)"
    };
    MYSQL_BIND params[5] = {
        make_bind_in(recipient_uid),
        make_bind_in(pPeer->growid),
        make_bind_in(message),
        make_bind_in(item_id),
        make_bind_in(item_count)
    };
    mysql_stmt_bind_param(hInsert.pStmt, params);
    mysql_stmt_execute(hInsert.pStmt);

    on::ConsoleMessage(event.peer, std::format("`oMail sent to `w{}``!", target_growid));
}
