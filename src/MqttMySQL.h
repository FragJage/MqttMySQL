#ifndef MQTTMYSQL_H
#define MQTTMYSQL_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include "MqttDaemon.h"
#include "MqttBridge.h"
#include "DbMysql.h"

struct MysqlQueue
{
    MysqlQueue(std::string table, std::string format, std::string value) : Table(table), Format(format), Value(value) {};
    std::string Table;
    std::string Format;
    std::string Value;
};

class MqttMySQL : public MqttDaemon, public IForwardMessage
{
    public:
        MqttMySQL();
        ~MqttMySQL();

		int DaemonLoop(int argc, char* argv[]);
        void IncomingMessage(const std::string& topic, const std::string& message);
        void on_forward(const std::string& identifier, const std::string& topic, const std::string& message);

    private:
        std::string m_MySQLServer;
        int m_MySQLPort;
        std::string m_MySQLDb;
        std::vector<MqttBridge*> m_MqttClients;
        std::set<std::string> m_TableExist;
        DbMysql m_DbMysql;
        std::map<std::string, std::map<std::string, std::string>> m_Formats;
        std::map<std::string, std::map<std::string, std::string>> m_TableNames;

		void DaemonConfigure(SimpleIni& iniFile);
		void CustomSectionConfigure(SimpleIni& iniFile, std::string sectionName);
		std::string ExtractFilter(std::string key);
		void CheckTable(const std::string& table, const std::string& topic);
		std::string SearchMap(const std::map<std::string, std::string>& mapFilter, const std::string& topic);
		std::string GetTableName(const std::string& identifier, const std::string& topic);
		std::string GetTableFormat(const std::string& identifier, const std::string& topic);
		void SaveMysqlValues();

        std::queue<MysqlQueue> m_MysqlQueue;
		std::mutex m_MysqlQueueAccess;
};
#endif // MQTTMYSQL_H
