#ifndef IFORWARDMESSAGE_H
#define IFORWARDMESSAGE_H

#include <string>

class IForwardMessage
{
public:
	virtual ~IForwardMessage() {};
	virtual void on_forward(const std::string& identifier, const std::string& topic, const std::string& message) = 0;
};

#endif // IFORWARDMESSAGE_H
