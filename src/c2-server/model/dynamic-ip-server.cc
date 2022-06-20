
#include "dynamic-ip-server.h"

#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"
#include "ns3/address-utils.h"
#include "ns3/udp-socket.h"

namespace ns3 {

void DynamicIpServer::Setup(uint16_t port){
     m_port = port;
}
void DynamicIpServer::StartApplication(void){
    m_packetsSent = 0;
        if (m_socket == 0)
        {
            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket (GetNode (), tid);
            InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
            if (m_socket->Bind (local) == -1)
            {
                NS_FATAL_ERROR ("Failed to bind socket");
            }
            if (addressUtils::IsMulticast (m_local))
            {
                Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
                if (udpSocket)
                {
                    // equivalent to setsockopt (MCAST_JOIN_GROUP)
                    udpSocket->MulticastJoinGroup (0, m_local);
                }
                else
                {
                    NS_FATAL_ERROR ("Error: Failed to join multicast group");
                }
            }
        }
      m_socket->SetRecvCallback (MakeCallback (&DynamicIpServer::HandleRead, this));

}
void DynamicIpServer::StopApplication (void)
{
  m_running = false;
  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}
void DynamicIpServer::HandleRead (Ptr<Socket> socket){
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  std::string message;
  while ((packet = socket->RecvFrom (from))){
    
    socket->GetSockName (localAddress);
    
    int packet_size = packet->GetSize();
    uint8_t received_message[packet_size];
    memset(received_message, 0, packet_size);
    packet->CopyData (received_message, packet_size);
    NS_LOG_UNCOND ("[client]: " << received_message);
    if(strncmp((char*)received_message, "absolute", sizeof("absolute")) == 0){
      message = "relative";
    }else if(strncmp((char*)received_message, "relative", sizeof("relative")) == 0){
      message = "absolute";
    }else{
      message = "please, absolute / relative";
    }
    std::vector<uint8_t> vect(message.begin(), message.end());
    uint8_t *p = &vect[0];
    packet = Create<Packet> (p, message.size());
    socket->SendTo (packet, 0, from);
  }
}

}
int 
main (int argc, char *argv[])
{
  std::string mode = "ConfigureLocal";
  std::string tapName = "thetap";
}
