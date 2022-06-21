/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/applications-module.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"
namespace ns3 {

class DynamicIpServer : public Application{
public:
    DynamicIpServer(
    );
	  void Setup(uint16_t port, bool c2);
    void SendPacket(Ptr<Socket> socket, std::vector<uint8_t> send_mes, uint32_t size);
    void SendSchedule (Time times, Ptr<Socket> socket, std::vector<uint8_t> send_mes, uint32_t size);
    void SocketClose(Ptr<Socket> socket);
    void SocketCloseSchedule (Time times, Ptr<Socket> socket);
private:
	  void StartApplication (void);
    void StopApplication (void);
    void HandleRead (Ptr<Socket> socket);

    void HandleAccept(Ptr<Socket> socket, const Address& from);

    Ptr<Socket>     m_socket;
    bool            m_running;
    uint32_t        m_packetsSent;
    Ipv4Address     m_local;
    uint16_t        m_port;
    bool            m_c2;
    std::list<Ptr<Socket>> m_socketList;
};

}