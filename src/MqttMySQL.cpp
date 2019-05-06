#include <iostream>
#include <algorithm>
#ifdef WIN32
#include <WinSock2.h>		// To stop windows.h including winsock.h
#endif
#include "StringTools.h"
#include "MqttMySQL.h"


using namespace std;

MqttMySQL::MqttMySQL() : MqttDaemon("influx", "MqttMySQL")
{
    m_FullName = false;
}

MqttMySQL::~MqttMySQL()
{
    vector<MqttBridge*>::iterator it;

    for(it=m_MqttClients.begin(); it!=m_MqttClients.end(); ++it)
    {
        delete(*it);
    }
    m_MqttClients.clear();
}

void MqttMySQL::ConfigureFormat(SimpleIni& iniFile)
{
    string value;

    for (SimpleIni::KeyIterator itKey = iniFile.beginKey("format"); itKey != iniFile.endKey("format"); ++itKey)
    {
        value = iniFile.GetValue("format", *itKey, "");
        m_TableFormat[*itKey] = value;
        LOG_DEBUG(m_Log) << "Set format to " << value << " for topic " << *itKey;
    }
}

void MqttMySQL::DaemonConfigure(SimpleIni& iniFile)
{
    LOG_ENTER;

	for (SimpleIni::SectionIterator itSection = iniFile.beginSection(); itSection != iniFile.endSection(); ++itSection)
	{
		if ((*itSection) == "mqtt") continue;
		if ((*itSection) == "log") continue;
		if ((*itSection) == "mysql")
        {
            m_MySQLServer = iniFile.GetValue("mysql", "server", "127.0.0.1");
            m_MySQLPort = iniFile.GetValue("mysql", "port", 3306);
            m_MySQLDb = iniFile.GetValue("mysql", "database", "");
            LOG_VERBOSE(m_Log) << "MySQL server " << m_MySQLServer << ":" << m_MySQLPort << ", database " << m_MySQLDb;

            string user = iniFile.GetValue("mysql", "user", "");
            string pass = iniFile.GetValue("mysql", "password", "");
            m_DbMysql.Init(m_MySQLServer, m_MySQLPort, m_MySQLDb, user, pass);

            string format  = iniFile.GetValue("mysql", "tablename", "sensorname");
            if(StringTools::IsEqualCaseInsensitive(format, "fullname"))
            {
                m_FullName = true;
                LOG_VERBOSE(m_Log) << "Table name is full sensor name.";
            }
            else
            {
                m_FullName = false;
                LOG_VERBOSE(m_Log) << "Table name is sensor name.";
            }
            continue;
        }
		if ((*itSection) == "format")
        {
            ConfigureFormat(iniFile);
            continue;
        }

        string name    = (*itSection);
        string server  = iniFile.GetValue(name, "server", "");
        string topic   = iniFile.GetValue(name, "topic", "");

        if((server=="")||(topic==""))
        {
            LOG_WARNING(m_Log) << "Keywords server and topic are mandatory in section " << name;
            continue;
        }

        LOG_VERBOSE(m_Log) << "Section " << name << " : Subscript to " << server << " topic " << topic;
        MqttBridge* pMqttBridge = new MqttBridge(name, server, topic, this);
        m_MqttClients.emplace_back(pMqttBridge);
    }

	LOG_EXIT_OK;
}

string MqttMySQL::SearchFormat(const string& topic)
{
    string type = "float";
    map<string, string>::iterator it = m_TableFormat.find(topic);
    LOG_DEBUG(m_Log) << "Search type for topic " << topic;
    if(it == m_TableFormat.end())
    {
        size_t pos = topic.find_last_of('/');
        if(pos != string::npos)
        {
            string generic = topic.substr(0, pos)+"#";
            LOG_DEBUG(m_Log) << "Search type for topic " << generic;
            it = m_TableFormat.find(generic);
        }
    }
    if(it != m_TableFormat.end()) type = it->second;

    LOG_VERBOSE(m_Log) << "Found type " << type << " for topic " << topic;
    return type;
}

void MqttMySQL::CheckTable(const string& table, const string& topic)
{
    if(m_TableExist.find(table) != m_TableExist.end()) return;

    if(m_DbMysql.IsTableExist(table))
    {
        m_TableExist.insert(table);
        return;
    }

    string type = SearchFormat(topic);

    LOG_VERBOSE(m_Log) << "Create table " << table << " with type " << type;
    if(m_DbMysql.CreateTable(table, type))
    {
        m_TableExist.insert(table);
    }
    else
    {
        LOG_ERROR(m_Log) << "Unable to create table " << table << " with type " << type << " : " << m_DbMysql.GetLastError();
    }
}

void MqttMySQL::on_forward(const string& identifier, const string& topic, const string& message)
{
    if(m_bPause) return;

    LOG_VERBOSE(m_Log) << "Mqtt receive for rule " << identifier << " : " << topic << " => " << message;

    size_t pos;
    string name(topic);
    if(m_FullName)
    {
        name = StringTools::ReplaceAll(name, "/", ".");
    }
    else
    {
        pos = name.find_last_of('/');
        if(pos != string::npos) name = name.substr(pos+1);
    }

    string value = message;
    m_DbMysql.Connect();
    CheckTable(name, topic);
    LOG_VERBOSE(m_Log) << "Send to MySQL => " << name << " value=" << value;
    m_DbMysql.AddValue(name, value);
    m_DbMysql.Disconnect();
}

void MqttMySQL::on_message(const string& topic, const string& message)
{
	LOG_VERBOSE(m_Log) << "Mqtt receive " << topic << " : " << message;

	string mainTopic = GetMainTopic();
	if (topic.substr(0, mainTopic.length()) != mainTopic)
	{
		LOG_WARNING(m_Log) << "Receive topic not for me (" << mainTopic << ")";
		return;
	}

	if ( (topic.substr(mainTopic.length(), 7) != "command") ||(topic.length() != mainTopic.length() + 7) )
	{
		LOG_WARNING(m_Log) << "Receive topic but not a command (waiting " << mainTopic + "command" << ")";
		return;
	}

	//TO DO service administration

	return;
}

int MqttMySQL::DaemonLoop(int argc, char* argv[])
{
	LOG_ENTER;

	Subscribe(GetMainTopic() + "command/#");
	LOG_VERBOSE(m_Log) << "Subscript to : " << GetMainTopic() + "command/#";

	bool bStop = false;
	m_bPause = false;
	while(!bStop)
    {
		int cond = Service::Get()->Wait();
		if (cond == Service::STATUS_CHANGED)
		{
			switch (Service::Get()->GetStatus())
			{
                case Service::StatusKind::PAUSE:
                    m_bPause = true;
                    break;
                case Service::StatusKind::START:
                    m_bPause = false;
                    break;
                case Service::StatusKind::STOP:
                    bStop = true;
                    break;
			}
		}
    }

	LOG_EXIT_OK;
    return 0;
}
