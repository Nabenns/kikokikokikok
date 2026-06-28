#include "pch.hpp"

#include "holiday.hpp"

u_char holiday = H_NONE;

std::tm localtime()
{
    std::time_t now  = std::time(nullptr);

    return *std::localtime(&now);
}

void check_for_holiday()
{
    std::tm time = localtime();

    if (time.tm_mon == 1/*feb*/ && (time.tm_mday >= 13 && time.tm_mday <= 13+7)) holiday = H_VALENTINES;
    if (time.tm_mon == 2/*march*/ && (time.tm_mday >= 14 && time.tm_mday <= 14+7)) holiday = H_PATRICKS;
    /* WinterFest: December 1-31 */
    if (time.tm_mon == 11/*dec*/) holiday = H_WINTERFEST;
    /* Halloween: October 15-31 */
    if (time.tm_mon == 9/*oct*/ && time.tm_mday >= 15) holiday = H_HALLOWEEN;
}

std::string game_theme_string()
{
    switch (holiday)
    {
        case H_VALENTINES: return "valentines-theme";
        case H_PATRICKS:   return "patricks-theme";
        case H_G4G:        return "g4g-theme";
        case H_WINTERFEST: return "winter-theme";
        case H_HALLOWEEN:  return "halloween-theme";
    }
    return "";
}

std::pair<std::string, std::string> holiday_greeting()
{
    switch (holiday)
    {
        case H_VALENTINES: return {"`5Valentine's Week!``", "`4Happy Valentine's Week!``"};
        case H_PATRICKS:   return {"`5St. Patrick's Week!``", "`2Happy St. Patrick's Day!``"};
        case H_G4G:        return {"`5Grow4Good Week!``", "It's `2Grow4Good!`` Complete tasks, earn rewards and contribute to a great cause!"};
        case H_WINTERFEST: return {"`5WinterFest!``", "`bHappy WinterFest!`` Collect winter items and spread the holiday cheer!"};
        case H_HALLOWEEN:  return {"`5Halloween!``", "`4Happy Halloween!`` Trick or treat - collect spooky items and costumes!"};
    }
    return {"", ""};
}

std::string holiday_banner()
{
    switch (holiday)
    {
        case H_VALENTINES: return "interface/large/gui_valentine_banner.rttex";
        case H_WINTERFEST: return "interface/large/gui_winter_banner.rttex";
        case H_HALLOWEEN:  return "interface/large/gui_halloween_banner.rttex";
    }
    return "interface/large/news_banner.rttex"; // @note default:
}