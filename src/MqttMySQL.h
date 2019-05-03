#ifndef MQTTMYSQL_H
#define MQTTMYSQL_H

#include <string>
#include <vector>
#include <map>
#include "MqttDaemon.h"
#include "MqttBridge.h"
#include "DbMysql.h"

class MqttMySQL : public MqttDaemon, public IForwardMessage
{
    public:
        MqttMySQL();
        ~MqttMySQL();

		int DaemonLoop(int argc, char* argv[]);
        void on_message(const std::string& topic, const std::string& message);
        void on_forward(const std::string& identifier, const std::string& topic, const std::string& message);

    private:
        bool m_bPause;
        std::string m_MySQLServer;
        int m_MySQLPort;
        std::string m_MySQLDb;
        bool m_FullName;
        std::vector<MqttBridge*> m_MqttClients;
        std::map<std::string, std::string> m_TableFormat;
        std::set<std::string> m_TableExist;
        DbMysql m_DbMysql;

		void DaemonConfigure(SimpleIni& iniFile);
		void ConfigureFormat(SimpleIni& iniFile);
		void CheckTable(const std::string& table, const std::string& topic);
		std::string SearchFormat(const std::string& topic);
};
#endif // MQTTMYSQL_H
