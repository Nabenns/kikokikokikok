#include "pch.hpp"
#include "on/Action.hpp"
#include "find.hpp"
#include "warp.hpp"
#include "punch.hpp"
#include "skin.hpp"
#include "sb.hpp"
#include "who.hpp"
#include "me.hpp"
#include "news.hpp"
#include "weather.hpp"
#include "ghost.hpp"
#include "ageworld.hpp"
#include "admin.hpp"
#include "gameplay.hpp"
#include "guild.hpp"
#include "achievements.hpp"
#include "systems.hpp"
#include "activity.hpp"
#include "__command.hpp"

/* if you plan to use this outside of this file, please include in __command.hpp (^-^) - and just make it a void. */
auto help_return = [](ENetEvent& event, const std::string_view text) 
{
    send_action(*event.peer, "log", "msg|>> Commands: /find /warp {world} /punch {id} /skin {id} /sb {msg} /who /me {msg} /news /weather {id} /ghost /ageworld /give {id} {count} /setgems {id} /ban {id} /unban {id} /mute {id} /unmute {id} /kick {id} /setrole {id} {role} /daily /title /ignore {id} /unignore {id} /friend_add {id} /friend_remove {id} /friend_list /pet_info /quest_list /guild {create|invite|leave|info|motd} /achievements /wave /dance /love /sleep /facepalm /fp /smh /yes /no /omg /idk /shrug /furious /rolleyes /foldarms /stubborn /fold /dab /sassy /dance2 /march /grumpy /shy \\0");
};

std::unordered_map<std::string_view, std::function<void(ENetEvent&, const std::string_view)>> cmd_pool
{
    {"help", help_return },
    {"?", help_return },
    {"find", &find},
    {"warp", &warp},
    {"punch", &punch},
    {"skin", &skin},
    {"sb", &sb},
    {"who", &who},
    {"me", &me},
    {"news", &news},
    {"weather", &weather},
    {"ghost", &ghost},
    {"ageworld", &ageworld},
    {"give", &command::give},
    {"setgems", &command::setgems},
    {"ban", &command::ban},
    {"unban", &command::unban},
    {"mute", &command::mute},
    {"unmute", &command::unmute},
    {"kick", &command::kick},
    {"setrole", &command::setrole},
    {"daily", &command::daily},
    {"title", &command::title},
    {"ignore", &command::ignore},
    {"unignore", &command::unignore},
    {"friend_add", &command::friend_add},
    {"friend_remove", &command::friend_remove},
    {"friend_list", &command::friend_list},
    {"pet_info", &command::pet_info},
    {"quest_list", &command::quest_list},
    {"guild", &guild},
    {"achievements", &achievements},
    {"casino", &command::casino},
    {"leaderboard", &command::leaderboard},
    {"bounty", &command::bounty},
    {"refer", &command::refer},
    {"surgery", &command::surgery},
    {"race", &command::race},
    {"duel", &command::duel},
    {"guildwar", &command::guildwar},
    {"broadcast", &command::broadcast},
    {"wave", &on::Action}, {"dance", &on::Action}, {"love", &on::Action}, {"sleep", &on::Action}, {"facepalm", &on::Action}, {"fp", &on::Action}, 
    {"smh", &on::Action}, {"yes", &on::Action}, {"no", &on::Action}, {"omg", &on::Action}, {"idk", &on::Action}, {"shrug", &on::Action}, 
    {"furious", &on::Action}, {"rolleyes", &on::Action}, {"foldarms", &on::Action}, {"fa", &on::Action}, {"stubborn", &on::Action}, {"fold", &on::Action}, 
    {"dab", &on::Action}, {"sassy", &on::Action}, {"dance2", &on::Action}, {"march", &on::Action}, {"grumpy", &on::Action}, {"shy", &on::Action}
};
