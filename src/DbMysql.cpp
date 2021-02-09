#include <stdexcept>
#include <ctime>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include "DbMysql.h"


using namespace std;

DbMysql::DbMysql()
{
    m_Log = nullptr;
    m_open = false;
}

DbMysql::DbMysql(const string& host, int port, const string& name, const string& user, const string& pwd)
{
    Init(host, port, name, user, pwd);
    m_Log = nullptr;
    m_open = false;
}

DbMysql::~DbMysql()
{
    if(m_open) Disconnect();
}

void DbMysql::SetLogger(SimpleLog* log)
{
    m_Log = log;
}

void DbMysql::Init(const std::string& host, int port, const std::string& name, const std::string& user, const std::string& pwd)
{
    m_host = host;
    m_name = name;
    m_user = user;
    m_pwd = pwd;
    m_port = port;
    if(m_port==0) m_port=3306;
}

bool DbMysql::Connect()
{
    bool ok=true;
    MYSQL *hCnx=nullptr;


	//Initialisation du gestionnaire de la connexion à la base de données mySQL
	m_hCnx=mysql_init(nullptr);
	if(m_hCnx==nullptr) ok=false;

	//Connexion au serveur mySQL
	if(ok) hCnx = mysql_real_connect(m_hCnx,m_host.c_str(),m_user.c_str(),m_pwd.c_str(),NULL,m_port,NULL,0);
	if(hCnx==nullptr) ok=false;

	//Sélection de la base
	if(ok) ok = (mysql_select_db(m_hCnx, m_name.c_str())==0);

	if(!ok)
    {
        m_lastError = string((char *)mysql_error(m_hCnx));
        if(m_Log) LOG_ERROR(m_Log) << "Unable to connect to MySql : " << m_lastError;
    }

	m_open = ok;
    return ok;
}

void DbMysql::Disconnect()
{
    mysql_close(m_hCnx);
	m_hCnx = nullptr;
	m_open = false;
}

bool DbMysql::AddValue(const string& table, const string& value)
{
    ostringstream str;


    str << "INSERT INTO `" << table << "`";
    str << " VALUES ('"<<toSQLDate()<<"', '"<<value<<"')";

    return Query(str.str());
}

string DbMysql::toSQLDate(time_t myTime)
{
    char buff[20];

    if(myTime==0) time(&myTime);
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&myTime));
    return string(buff);
}

bool DbMysql::CreateTable(const string& table, const string& type)
{
    ostringstream str;

    str << "CREATE TABLE `" << table << "` (time_sec timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP, value " << type << " NOT NULL, PRIMARY KEY (time_sec)) ENGINE=InnoDB";
    return Query(str.str());
}

bool DbMysql::IsTableExist(const string& table)
{
    ostringstream str;

    str << "SHOW TABLES LIKE '" << table << "'";
    return (QueryCount(str.str())>0);
}

string DbMysql::GetLastError()
{
    return m_lastError;
}

unsigned long DbMysql::QueryCount(const string& query)
{
    ostringstream str;
    string req;
    int ret;
    bool close = false;
    unsigned long nb = -1;


    if(!m_open)
    {
        if(!Connect()) return false;
        close = true;
    }

    if(m_Log) LOG_VERBOSE(m_Log) << "Execute SQL : " << query;
	ret = mysql_query(m_hCnx, query.c_str());
	if(ret==0)
	{
        MYSQL_RES *result = mysql_store_result(m_hCnx);
        if(result)
        {
            nb = mysql_num_rows(result);
            mysql_free_result(result);
        }
        else
        {
            str << "Impossible d'obtenir les résultats de " <<query<< " : " <<mysql_error(m_hCnx);
            m_lastError = str.str();
            if(m_Log) LOG_ERROR(m_Log) << m_lastError;
        }
	}
	else
    {
	    str << "Echec de la requête " <<query<< " : " <<mysql_error(m_hCnx);
		m_lastError = str.str();
		if(m_Log) LOG_ERROR(m_Log) << m_lastError;
    }

	if(close) Disconnect();

    return nb;
}

bool DbMysql::Query(const string& query)
{
    ostringstream str;
    string req;
    int ret;
    bool close = false;


    if(!m_open)
    {
        if(!Connect()) return false;
        close = true;
    }

    if(m_Log) LOG_VERBOSE(m_Log) << "Execute SQL : " << query;
	ret = mysql_query(m_hCnx, query.c_str());
	if(ret!=0)
	{
	    str << "Echec de la requête " <<query<< " : " <<mysql_error(m_hCnx);
		m_lastError = str.str();
		if(m_Log) LOG_ERROR(m_Log) << m_lastError;
	}

	if(close) Disconnect();

    return (ret==0);
}
