#ifndef MQTTBRIDGE_H
#define MQTTBRIDGE_H

#include <string>
#include <set>
#include "MqttBase.h"
#include "IForwardMessage.h"

class MqttBridge : public MqttBase
{
    public:
		MqttBridge(const std::string& identifier, const std::string& server, const std::string& topic, IForwardMessage* forwardCb);
        ~MqttBridge();
		void on_message(const std::string& topic, const std::string& message);

	private:
		std::string m_Identifier;
		IForwardMessage* m_Callback;
};

#endif // MQTTBRIDGE_H
