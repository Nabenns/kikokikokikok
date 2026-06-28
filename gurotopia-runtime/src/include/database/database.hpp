#pragma once

#include <mysql/mysql.h>
#include <mutex>

extern MYSQL *db;
extern std::mutex db_mutex;

extern void mysql_connect();
extern void db_check_connection();

class hStmt
{
public:
    hStmt(const char *query);
   ~hStmt();

    hStmt           (const hStmt&) = delete;
    hStmt& operator=(const hStmt&) = delete;

    MYSQL_STMT *pStmt;

private:
    std::lock_guard<std::mutex> m_lock;
};


extern MYSQL_BIND make_bind_in(const signed &buffer);
extern MYSQL_BIND make_bind_in(const unsigned &buffer);
extern MYSQL_BIND make_bind_in(const long &buffer);
extern MYSQL_BIND make_bind_in(const long long &buffer);
extern MYSQL_BIND make_bind_in(const float &buffer);
extern MYSQL_BIND make_bind_in(const std::string &buffer);

extern MYSQL_BIND make_bind_out(signed &buffer);
extern MYSQL_BIND make_bind_out(unsigned &buffer);
extern MYSQL_BIND make_bind_out(long &buffer);
extern MYSQL_BIND make_bind_out(long long &buffer);
extern MYSQL_BIND make_bind_out(float &buffer);
extern MYSQL_BIND make_bind_out(std::string &buffer);
