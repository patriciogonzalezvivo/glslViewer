#pragma once

#include "oscpack/osc/OscReceivedElements.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/ip/UdpSocket.h"

#include <iostream>
#include <cstring>
#include <string>
#include <mutex>

class MyPacketListener : public osc::OscPacketListener {
public:
    std::function<void(const std::string &_cmd, std::mutex &_mutex)> runCmd;

protected:
    virtual void ProcessMessage( const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint );

private:
    std::mutex  m_mutex;
};
