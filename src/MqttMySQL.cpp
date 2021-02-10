#include <iostream>
#include <algorithm>
#include <regex>
#ifdef WIN32
#include <WinSock2.h>		// To stop windows.h including winsock.h
#endif
#include "StringTools.h"
#include "MqttMySQL.h"


using namespace std;

MqttMySQL::MqttMySQL() : MqttDaemon("mysql", "MqttMySQL")
{
    m_DbMysql.SetLogger(m_Log);
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

string MqttMySQL::ExtractFilter(string key)
{
    size_t pos = key.find('(');
    if(pos==string::npos) return "default";
    if(key.back()!=')')
    {
        LOG_WARNING(m_Log) << "Bad filter, missing ')' in : " << key;
        return "";
    }
    return key.substr(pos+1, key.length()-pos-2);
}

void MqttMySQL::CustomSectionConfigure(SimpleIni& iniFile, string sectionName)
{
    LOG_ENTER;

    string key;
    string value;
    string filter;

    string server = iniFile.GetValue(sectionName, "server", "");
    string topic = iniFile.GetValue(sectionName, "topic", "");

    if((server=="")||(topic==""))
    {
        LOG_WARNING(m_Log) << "Keywords server and topic are mandatory in section " << sectionName;
        LOG_EXIT_KO;
        return;
    }

    for (SimpleIni::KeyIterator itKey = iniFile.beginKey(sectionName); itKey != iniFile.endKey(sectionName); ++itKey)
    {
        key = (*itKey);
        value = iniFile.GetValue(sectionName, key, "");
        if(key == "server") continue;
        if(key == "topic") continue;
        if(key.substr(0, 6) == "format")
        {
            filter = ExtractFilter(key);
            if(filter!="") m_Formats[sectionName][filter] = value;
            continue;
        }
        if(key.substr(0, 5) == "table")
        {
            filter = ExtractFilter(key);
            if(filter!="") m_TableNames[sectionName][filter] = value;
            continue;
        }
    }

    LOG_VERBOSE(m_Log) << "Section " << sectionName << " : Subscript to " << server << " topic " << topic;
    MqttBridge* pMqttBridge = new MqttBridge(sectionName, server, topic, this);
    m_MqttClients.emplace_back(pMqttBridge);

	LOG_EXIT_OK;
}

void MqttMySQL::DaemonConfigure(SimpleIni& iniFile)
{
    LOG_ENTER;

    for (SimpleIni::SectionIterator itSection = iniFile.beginSection(); itSection != iniFile.endSection(); ++itSection)
	{
		if ((*itSection) == "mqtt") continue;
		if ((*itSection) == "log") continue;
		if ((*itSection) == "mqttlog") continue;
		if ((*itSection) == "mysql")
        {
            m_MySQLServer = iniFile.GetValue("mysql", "server", "127.0.0.1");
            m_MySQLPort = iniFile.GetValue("mysql", "port", 3306);
            m_MySQLDb = iniFile.GetValue("mysql", "database", "");
            LOG_VERBOSE(m_Log) << "MySQL server " << m_MySQLServer << ":" << m_MySQLPort << ", database " << m_MySQLDb;

            string user = iniFile.GetValue("mysql", "user", "");
            string pass = iniFile.GetValue("mysql", "password", "");
            m_DbMysql.Init(m_MySQLServer, m_MySQLPort, m_MySQLDb, user, pass);
            continue;
        }
        CustomSectionConfigure(iniFile, *itSection);
    }

	LOG_EXIT_OK;
}

void MqttMySQL::CheckTable(const string& table, const string& format)
{
    if(m_TableExist.find(table) != m_TableExist.end()) return;

    if(m_DbMysql.IsTableExist(table))
    {
        m_TableExist.insert(table);
        return;
    }

    LOG_VERBOSE(m_Log) << "Create table " << table << " with type " << format;
    if(m_DbMysql.CreateTable(table, format))
    {
        m_TableExist.insert(table);
    }
    else
    {
        LOG_ERROR(m_Log) << "Unable to create table " << table << " with type " << format << " : " << m_DbMysql.GetLastError();
    }
}

string MqttMySQL::SearchMap(const map<string, string>& mapFilter, const string& topic)
{
    map<string, string>::const_iterator it = mapFilter.cbegin();
    map<string, string>::const_iterator itEnd = mapFilter.cend();
    while (it != itEnd)
    {
        regex filter(it->first);
        if(regex_match(topic, filter))
        {
            LOG_VERBOSE(m_Log) << "Filter " << it->first << " match for topic " << topic << " : " << it->second;
            return it->second;
        }
        it++;
    }
    it = mapFilter.find("default");
    if (it == itEnd) return "";
    LOG_VERBOSE(m_Log) << "Filter default found for topic " << topic << " : " << it->second;
    return it->second;
}

string MqttMySQL::GetTableName(const string& identifier, const string& topic)
{
    string table = SearchMap(m_TableNames[identifier], topic);
    if(table == "")
    {
        LOG_VERBOSE(m_Log) << "No filter found from "<< identifier << " for topic " << topic << " to identify table name";
        return "";
    }

    if(table == "auto")
    {
        table = topic;
        return StringTools::ReplaceAll(table, "/", "_");
    }

	if(table.find('#')==string::npos) return table;
    string name(table);
    vector<string> segs = StringTools::Split(topic, '/');
    for(size_t i=0; i<segs.size(); i++) name = StringTools::ReplaceAll(name, "#"+to_string(i+1), segs[i]);
    return name;
}

string MqttMySQL::GetTableFormat(const string& identifier, const string& topic)
{
    string format = SearchMap(m_Formats[identifier], topic);
    if(format == "") format = "float";
    return format;
}

void MqttMySQL::on_forward(const string& identifier, const string& topic, const string& message)
{
    LOG_VERBOSE(m_Log) << "Mqtt receive for section " << identifier << " : " << topic << " => " << message;

    string table = GetTableName(identifier, topic);
    if(table == "") return;
    string format = GetTableFormat(identifier, topic);
    string value = message;

    lock_guard<mutex> lock(m_MysqlQueueAccess);
    m_MysqlQueue.emplace(table, format, value);

    LOG_VERBOSE(m_Log) << "Exit on_forward";
}

void MqttMySQL::SaveMysqlValues()
{
	lock_guard<mutex> lock(m_MysqlQueueAccess);
	while (!m_MysqlQueue.empty())
	{
		MysqlQueue& mysqlQueue = m_MysqlQueue.front();
        m_DbMysql.Connect();
        CheckTable(mysqlQueue.Table, mysqlQueue.Format);
        LOG_VERBOSE(m_Log) << "Send to MySQL => " << mysqlQueue.Table << "(" << mysqlQueue.Format << ") value=" << mysqlQueue.Value;
        m_DbMysql.AddValue(mysqlQueue.Table, mysqlQueue.Value);
        m_DbMysql.Disconnect();
		m_MysqlQueue.pop();
	}
}

void MqttMySQL::IncomingMessage(const string& topic, const string& message)
{
    LOG_VERBOSE(m_Log) << "IncomingMessage";
	return;
}

int MqttMySQL::DaemonLoop(int argc, char* argv[])
{
	LOG_ENTER;

	Subscribe(GetMainTopic() + "command/#");
	LOG_VERBOSE(m_Log) << "Subscript to : " << GetMainTopic() + "command/#";

	bool bStop = false;
	while(!bStop)
    {
        if(WaitFor(250)==Service::STATUS_CHANGED)
        {
            if(Service::Get()->GetStatus() == Service::StatusKind::STOP) bStop = true;
        }
        SaveMysqlValues();
    }

	LOG_EXIT_OK;
    return 0;
}
