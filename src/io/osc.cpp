#include "osc.h"
#include "../tools/text.h"

Osc::Osc() {
}

Osc::~Osc() {
    stop();
}

bool Osc::isListening() const{
    return listenSocket != nullptr;
}

bool Osc::start(int _port, std::function<void(const std::string &_cmd, std::mutex &_mutex)> _runCmd, bool _verbose) {
    if (listenSocket)
        return true;

    m_port = _port;
    m_runCmd = _runCmd;
    m_verbose = _verbose;
        
    // // manually set larger buffer size instead of oscpack per-message size
    // if (UdpSocket::GetUdpBufferSize() == 0)
    //    UdpSocket::SetUdpBufferSize(65535);
        
    // create socket
    UdpListeningReceiveSocket *socket = nullptr;
    try {
        IpEndpointName name(IpEndpointName::ANY_ADDRESS, m_port);
        socket = new UdpListeningReceiveSocket(name, this);
        auto deleter = [](UdpListeningReceiveSocket*socket){
            // tell the socket to shutdown
            socket->Break();
            delete socket;
        };
        auto newPtr = std::unique_ptr<UdpListeningReceiveSocket, decltype(deleter)>(socket, deleter);
        listenSocket = std::move(newPtr);
    }

    catch(std::exception &e){
        std::string what = e.what();
        // strip endline as ofLogError already adds one
        if (!what.empty() && what.back() == '\n') {
            what = what.substr(0, what.size()-1);
        }
        std::cerr << "// Osc, couldn't create receiver on port " << m_port << ": " << what << std::endl;
        if (socket != nullptr){
            delete socket;
            socket = nullptr;
        }
        return false;
    }

    std::cout << "// OSC listening at localhost:" << m_port << std::endl;

    listenThread = std::thread([this]{
        while (listenSocket) {
            try{
                listenSocket->Run();
            }
            catch (std::exception &e){
                std::cerr << "Osc, " << e.what() << std::endl;
            }
        }
    });

    // detach thread so we don't have to wait on it before creating a new socket
    // or on destruction, the custom deleter for the socket unique_ptr already
    // does the right thing
    listenThread.detach();
        
    return true;
}

void Osc::stop() {
    listenSocket.reset();
}

void Osc::ProcessMessage( const osc::ReceivedMessage& _m, const IpEndpointName& _remoteEndpoint ){
    try {
        osc::ReceivedMessage::const_iterator arg = _m.ArgumentsBegin();
        
        // Convert OSC message to CSV
        std::string line;

        std::vector<std::string> address = split(std::string(_m.AddressPattern()), '/');
        for (size_t i = 0; i < address.size(); i++) {
            if (i != 0)
                line += "_";
            line += address[i];
        }

        while (arg != _m.ArgumentsEnd()) {
            if ( arg->IsBool() ) {
                line += "," + std::string( (arg->AsBoolUnchecked())? "on" : "off" );
            }
            // else if ( arg->IsChar() ) {
            //     line += "," + std::string( arg->AsCharUnchecked() );
            // }
            else if ( arg->IsInt32() ) {
                line += "," + toString( arg->AsInt32Unchecked() );
            }
            else if ( arg->IsInt64() ) {
                line += "," + toString( arg->AsInt64Unchecked() );
            }
            else if ( arg->IsFloat() ) {
                line += "," + toString( arg->AsFloatUnchecked(), 8 );
            }
            else if ( arg->IsDouble() ) {
                line += "," + toString( arg->AsDoubleUnchecked(), 8 );
            }
            // else if(arg->IsRgbaColor()){
            //      line += "," + arg->AsRgbaColorUnchecked();
            // }
            else if ( arg->IsString() ) {
                line += "," + std::string( arg->AsStringUnchecked() );
            }
            arg++;
        }

        if (m_verbose) 
            std::cout << line << std::endl;

        m_runCmd(line, m_mutex); 
    }
    catch( osc::Exception& e ) {
        // any parsing errors such as unexpected argument types, or 
        // missing arguments get thrown as exceptions.
        std::cout   << "error while parsing message: "
                    << _m.AddressPattern() << ": " << e.what() << "\n";
    }
}
