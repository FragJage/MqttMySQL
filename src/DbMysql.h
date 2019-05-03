#ifndef DBMYSQL_H
#define DBMYSQL_H

#include "mysql.h"

class DbMysql
{
    public:
        DbMysql();
        DbMysql(const std::string& host, int port, const std::string& name, const std::string& user, const std::string& pwd);
        ~DbMysql();

        void Init(const std::string& host, int port, const std::string& name, const std::string& user, const std::string& pwd);
        bool Connect();
        void Disconnect();
        bool AddValue(const std::string& table, const std::string& value);
        bool IsTableExist(const std::string& table);
        bool CreateTable(const std::string& table, const std::string& type);
        std::string GetLastError();

    private:
        bool Query(const std::string& query);
        unsigned long QueryCount(const std::string& query);
        std::string toSQLDate(time_t time=0);
        std::string m_lastError;
        std::string m_host;
        int m_port;
        bool m_open;
        std::string m_name;
        std::string m_user;
        std::string m_pwd;
        MYSQL *m_hCnx;
};
#endif
