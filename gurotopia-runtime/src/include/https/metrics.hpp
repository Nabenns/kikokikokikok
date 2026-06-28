#pragma once

#include <chrono>

class ServerMetrics
{
public:
    std::chrono::steady_clock::time_point start_time{std::chrono::steady_clock::now()};

    unsigned long long total_logins{0};
    unsigned long long total_blocks_broken{0};
};

extern ServerMetrics gMetrics;

/* return a JSON-friendly string with current metrics */
extern std::string metrics_json();