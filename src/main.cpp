#include <iostream>
#include <exception>
#include "MqttMySQL.h"

using namespace std;

int main(int argc, char* argv[])
{
    int res = 0;

    try
    {
        MqttMySQL mqttMySQL;

        Service* pService = Service::Create("MqttMySQL", "Log mqtt messages into MySQL", &mqttMySQL);
        res = pService->Start(argc, argv);
        Service::Destroy();
    }
    catch(const exception &e)
    {
        std::cout << e.what();
    }

    return res;
}
