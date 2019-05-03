#include "MqttBridge.h"

using namespace std;

MqttBridge::MqttBridge(const string& identifier, const string& server, const string& topic, IForwardMessage* forwardCb) : m_Identifier(identifier), m_Callback(forwardCb)
{
	SetServer(server, "");
	Connect();
	Subscribe(topic);
}

MqttBridge::~MqttBridge()
{
	Disconnect();
}

void MqttBridge::on_message(const string& topic, const string& message)
{
	m_Callback->on_forward(m_Identifier, topic, message);
	return;
}
