/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/applications-module.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"
namespace ns3 {

class DynamicIpServer : public Application{
public:
	DynamicIpServer()
    : m_socket(0),
      m_running(false),
      m_packetsSent(0),
      m_port(0)
    {}
	void Setup(uint16_t port);
private:
	void StartApplication (void);
    void StopApplication (void);
    void HandleRead (Ptr<Socket> socket);

    Ptr<Socket>     m_socket;
    bool            m_running;
    uint32_t        m_packetsSent;
    Ipv4Address     m_local;
    uint16_t        m_port;
};

}