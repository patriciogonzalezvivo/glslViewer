#pragma once

#include "oscpack/osc/OscReceivedElements.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/ip/UdpSocket.h"

#include <iostream>
#include <cstring>
#include <string>

#include <mutex>
#include <thread>
#include <functional>

class Osc : public osc::OscPacketListener {
public:
    Osc();
    ~Osc();

    bool start(int _port, std::function<void(const std::string &_cmd, std::mutex &_mutex)> _runCmd);
    void stop();
    bool isListening() const;


protected:
    // process an incoming osc message and add it to the queue
    virtual void ProcessMessage( const osc::ReceivedMessage& _m, const IpEndpointName& _remoteEndpoint );

    std::function<void(const std::string &_cmd, std::mutex &_mutex)> m_runCmd;
    int m_port;

private:
    // socket to listen on, unique for each port
    // shared between objects if allowReuse is true
    std::unique_ptr<UdpListeningReceiveSocket, std::function<void(UdpListeningReceiveSocket*)>> listenSocket;

    std::thread listenThread;   // listener thread
    std::mutex  m_mutex;
};
