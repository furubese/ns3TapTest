/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/tap-bridge-module.h"

#include "ns3/ipv4-l3-protocol.h"

#include "ns3/ipv4.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TapTest");

class MyServer : public Application{
    public:
	MyServer();
	void Setup(uint16_t port);
    private:
	void StartApplication (void);
        void StopApplication (void);
        void HandleRead (Ptr<Socket> socket);

        Ptr<Socket>     m_socket;
        bool            m_running;
        uint32_t        m_packetsSent;
        Ipv4Address           m_local;
        uint16_t          m_port;
};


MyServer::MyServer()
    : m_socket(0),
      m_running(false),
      m_packetsSent(0),
      m_port(0)
{}
void MyServer::Setup(uint16_t port){
     m_port = port;
}
void MyServer::StartApplication(void){
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
      m_socket->SetRecvCallback (MakeCallback (&MyServer::HandleRead, this));

}
void MyServer::StopApplication (void)
{
  m_running = false;
  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}
void MyServer::HandleRead (Ptr<Socket> socket){
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

class NodeIpAdder{
  public:
    NodeIpAdder(NetDeviceContainer NetDevice, Ipv4Address SendToIpFilter){
      this -> setNetDevice(NetDevice);
      this -> setSendToIP(SendToIpFilter);
    }
    void callback(Ptr<const Packet> p, Ptr<Ipv4> protcol, uint32_t interface);
    void setSendToIP(Ipv4Address ip){ m_SendToIP = ip; }
    void setNetDevice(NetDeviceContainer NetDevice){ m_NetDevice = NetDevice; }
  private:
    NetDeviceContainer m_NetDevice;
    Ipv4Address m_SendToIP;

};

void NodeIpAdder::callback(Ptr<const Packet> p, Ptr<Ipv4> protcol, uint32_t interface){
  Ipv4Header header;
  p->PeekHeader (header);
  NS_LOG_UNCOND ("======================================");
  NS_LOG_UNCOND ("S:" << header.GetSource() );
  NS_LOG_UNCOND ("D:" << header.GetDestination ());
  if(m_SendToIP == header.GetDestination()){return;}
  Ptr<NetDevice> device_app = m_NetDevice.Get(0);
  Ptr<Node> node = device_app->GetNode ();
  Ptr<Ipv4> ipv4proto = node->GetObject<Ipv4> ();
  int32_t ifIndex = ipv4proto->GetInterfaceForDevice (device_app);
  if(ifIndex == -1){
    ifIndex = ipv4proto->AddInterface (device_app);
  }

  ipv4proto->RemoveAddress(ifIndex, 0);

  const Ipv4Address ipv4address_tmp = header.GetDestination ();
  const Ipv4Mask ipv4_mask = Ipv4Mask ("/24");

  Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (ipv4address_tmp, ipv4_mask);

  ipv4proto->AddAddress (ifIndex, ipv4Addr);
  ipv4proto->SetMetric (ifIndex, 0);
  ipv4proto->SetUp (ifIndex);
}

int 
main (int argc, char *argv[])
{
  std::string mode = "ConfigureLocal";
  std::string tapName = "thetap";

  CommandLine cmd (__FILE__);
  cmd.AddValue ("mode", "Mode setting of TapBridge", mode);
  cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  cmd.Parse (argc, argv);

  LogComponentEnable("UdpL4Protocol", LOG_LEVEL_INFO);

  //GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  NodeContainer nodesLeft;
  nodesLeft.Create (2);

  InternetStackHelper internetLeft;
  internetLeft.Install (nodesLeft);

  CsmaHelper csmaLeft;

  ObjectFactory channelFactoryLeft;
  channelFactoryLeft.SetTypeId ("ns3::CsmaChannel");
  channelFactoryLeft.Set("DataRate", DataRateValue (5000000));
  channelFactoryLeft.Set("Delay", TimeValue (MilliSeconds (2)));
  Ptr<CsmaChannel> channelLeft = channelFactoryLeft.Create()->GetObject<CsmaChannel> ();
  NetDeviceContainer devicesLeft = csmaLeft.Install (nodesLeft, channelLeft);

  Ipv4AddressHelper ipv4Left;
  ipv4Left.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesLeft = ipv4Left.Assign (devicesLeft);

  TapBridgeHelper tapBridge (interfacesLeft.GetAddress (1));
  tapBridge.SetAttribute ("Mode", StringValue (mode));
  tapBridge.SetAttribute ("DeviceName", StringValue (tapName));
  tapBridge.Install (nodesLeft.Get (0), devicesLeft.Get (0));

  //
  // Stick in the point-to-point line between the sides.
  //
  
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  NodeContainer nodes = NodeContainer (nodesLeft.Get (1));
  NetDeviceContainer devices = csma.Install (nodes);
  Ipv4AddressHelper ipv4_app;
  ipv4_app.SetBase ("100.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces_app = ipv4_app.Assign (devices);

  //
  // Node set DefaultGW
  //

  Ptr<NetDevice> DEVICE_LEFT_0 = devicesLeft.Get(1);
  Ptr<Node> NODE_LEFT_0 = DEVICE_LEFT_0->GetNode ();
  Ptr<Ipv4> IPV4PROCL_LEFT_0 = NODE_LEFT_0->GetObject<Ipv4> ();
  int32_t IFINDEX_LEFT_0 = IPV4PROCL_LEFT_0->GetInterfaceForDevice (DEVICE_LEFT_0);
  if(IFINDEX_LEFT_0 == -1){
    IFINDEX_LEFT_0 = IPV4PROCL_LEFT_0->AddInterface (DEVICE_LEFT_0);
  }

  Ipv4StaticRoutingHelper Ipv4StaticRoutingHell;
  Ptr<Ipv4StaticRouting> Ipv4staticRoute = Ipv4StaticRoutingHell.GetStaticRouting(IPV4PROCL_LEFT_0);
  Ipv4staticRoute->SetDefaultRoute("10.1.1.1", IFINDEX_LEFT_0);

  //
  // App Install
  //

  Ptr<MyServer> server_app = CreateObject<MyServer> ();

  server_app->Setup (62000);
  nodesLeft.Get (1)->AddApplication (server_app);
  server_app->SetStartTime (Seconds (1.0));  

  /* inj */
  
  NodeIpAdder adder_App(devices, "150.19.196.156");

  Ptr<Ipv4L3Protocol> ipv4Proto = nodesLeft.Get(1)->GetObject<Ipv4L3Protocol> ();
  ipv4Proto-> TraceConnectWithoutContext("Rx", MakeCallback(&NodeIpAdder:: callback, &adder_App));

  //
  // log_config
  //

  csmaLeft.EnablePcapAll ("tap_test", false);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (60.));
  Simulator::Run ();
  Simulator::Destroy ();
}
