
#include "dynamic-ip-server.h"

#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"
#include "ns3/address-utils.h"

#define TELNET_MESSAGE_MAX_SIZE 5
#define MESSAGE_LIST_MAX_SIZE 3

namespace ns3 {
DynamicIpServer::DynamicIpServer(
)
: m_socket(0),
          m_running(false),
          m_packetsSent(0),
          m_port(0)
{}
void DynamicIpServer::Setup(uint16_t port){
     m_port = port;
}
void DynamicIpServer::StartApplication(void){
    m_packetsSent = 0;
        if (m_socket == 0)
        {
            TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
            m_socket = Socket::CreateSocket (GetNode (), tid);
            InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
            m_socket->Listen();
            if (m_socket->Bind (local) == -1)
            {
                NS_FATAL_ERROR ("Failed to bind socket");
            }
        }
      m_socket->SetRecvCallback (MakeCallback (&DynamicIpServer::HandleRead, this));
      m_socket->SetAcceptCallback(
        MakeNullCallback<bool, Ptr<Socket>, const Address &> (), 
        MakeCallback(&DynamicIpServer::HandleAccept, this)
      );
}
void DynamicIpServer::StopApplication (void)
{
  m_running = false;
  while(!m_socketList.empty()){
    Ptr<Socket> acceptedSocket = m_socketList.front();
    m_socketList.pop_front();
    acceptedSocket->Close();
  }
  if (m_socket)
  {
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
}

void DynamicIpServer::HandleAccept(Ptr<Socket> socket, const Address& from){
  socket->SetRecvCallback (MakeCallback (&DynamicIpServer::HandleRead, this));
  m_socketList.push_back(socket);
}
void DynamicIpServer::SendPacket(Ptr<Socket> socket, std::vector<uint8_t> send_mes, uint32_t size){
  uint8_t *p = &send_mes[0];
  socket->Send(Create<Packet> (p, size));
}

void DynamicIpServer::SendSchedule (Time times, Ptr<Socket> socket, std::vector<uint8_t> send_mes, uint32_t size)
{
  if (m_running)
    {
      Simulator::Schedule (times, &DynamicIpServer::SendPacket, this, socket, send_mes, size);
    }
}

void DynamicIpServer::SocketClose(Ptr<Socket> socket)
{
  socket->Close();
}

void DynamicIpServer::SocketCloseSchedule (Time times, Ptr<Socket> socket)
{
  Simulator::Schedule (times, &DynamicIpServer::SocketClose, this, socket);
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
    
    /*uint8t -> HEXstring*/
    std::string hex;
    std::string received_message_strings;
    for(int i=0; i<packet_size; i++){
      char hexstr[4];
      char str[4];
      sprintf(hexstr, "%02x", received_message[i]);
      hex += hexstr;
      //hex += " ";
      sprintf(str, "%c", received_message[i]);
      received_message_strings += str;
    }
    /**/

    NS_LOG_UNCOND ("[client]: " << received_message_strings);
    NS_LOG_UNCOND ("[ size ]: " << packet_size);
    NS_LOG_UNCOND (" [ HEX ]: " << hex);

    std::vector<uint8_t> send_mes_8t;

    if(hex == 
    "fffd03fffb18fffb1ffffb20fffb21fffb22fffb27fffd05fffb23"
    ){
      send_mes_8t = {0xff, 0xfd, 0x18, 0xff, 0xfd, 0x20, 0xff, 0xfd, 0x23, 0xff, 0xfd, 0x27};
    }else{
      //Watch Dog
      send_mes_8t = {0x00, 0x00};
    }

    SendSchedule(Seconds(0.1), socket, send_mes_8t, send_mes_8t.size());
    //6820010703353535
    std::vector<uint8_t> send_mes_8t_1 = {0x00, 0x13, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x01, 0x0a, 0x01, 0x01, 0x01, 0x20, 0x01, 0x07, 0x03, 0x35, 0x35, 0x35};
    SendSchedule(Seconds(5), socket, send_mes_8t_1, send_mes_8t_1.size());
  }
}

}
