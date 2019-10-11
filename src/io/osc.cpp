#include "osc.h"
#include "../tools/text.h"

void MyPacketListener::ProcessMessage( const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint ){
    try {
        osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
        
        // Convert OSC message to CSV
        std::string line;

        std::vector<std::string> address = split(std::string(m.AddressPattern()), '/');
        for (size_t i = 0; i < address.size(); i++) {
            if (i != 0)
                line += "_";
            line += address[i];
        }

        while (arg != m.ArgumentsEnd()) {
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
                line += "," + toString( arg->AsFloatUnchecked(), 3 );
            }
            else if ( arg->IsDouble() ) {
                line += "," + toString( arg->AsDoubleUnchecked(), 6 );
            }
            // else if(arg->IsRgbaColor()){
            //      line += "," + arg->AsRgbaColorUnchecked();
            // }
            else if ( arg->IsString() ) {
                line += "," + std::string( arg->AsStringUnchecked() );
            }
            arg++;
        }


        std::cout << line << std::endl;
        runCmd(line, m_mutex); 
    }
    catch( osc::Exception& e ) {
        // any parsing errors such as unexpected argument types, or 
        // missing arguments get thrown as exceptions.
        std::cout   << "error while parsing message: "
                    << m.AddressPattern() << ": " << e.what() << "\n";
    }
}
