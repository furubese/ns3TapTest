/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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

NS_LOG_COMPONENT_DEFINE ("TapDumbbellExample");

#define TELNET_MESSAGE_MAX_SIZE 5
#define MESSAGE_LIST_MAX_SIZE 3

typedef struct {
  std::string hex_mes[MESSAGE_LIST_MAX_SIZE];
  std::vector<uint8_t> uint8t_mes[MESSAGE_LIST_MAX_SIZE];
}message_list;

uint16_t
CalculateHeaderChecksum (uint16_t size, Ipv4Address m_source, Ipv4Address m_destination, uint8_t m_protocol)
{
  Buffer buf = Buffer ((2 * Address::MAX_SIZE) + 8);
  buf.AddAtStart ((2 * Address::MAX_SIZE) + 8);
  Buffer::Iterator it = buf.Begin ();
  uint32_t hdrSize = 0;

  WriteTo (it, m_source);
  WriteTo (it, m_destination);
  if (Ipv4Address::IsMatchingType (m_source))
    {
      it.WriteU8 (0); /* protocol */
      it.WriteU8 (m_protocol); /* protocol */
      it.WriteU8 (size >> 8); /* length */
      it.WriteU8 (size & 0xff); /* length */
      hdrSize = 12;
    }
  else if (Ipv6Address::IsMatchingType (m_source))
    {
      it.WriteU16 (0);
      it.WriteU8 (size >> 8); /* length */
      it.WriteU8 (size & 0xff); /* length */
      it.WriteU16 (0);
      it.WriteU8 (0);
      it.WriteU8 (m_protocol); /* protocol */
      hdrSize = 40;
    }

  it = buf.Begin ();
  /* we don't CompleteChecksum ( ~ ) now */
  return ~(it.CalculateIpChecksum (hdrSize));
}

class MyServer : public Application 
{
  public:
    MyServer ();

    void Setup (uint16_t port);
  private:
    void StartApplication (void);
    void StopApplication (void);

    void HandleRead (Ptr<Socket> socket);
    void HandleAccept(Ptr<Socket> socket, const Address& from);
    void SendPacket(Ptr<Socket> socket, std::vector<uint8_t> send_mes, uint32_t size);
    void SendSchedule (Time times, Ptr<Socket> socket, std::vector<uint8_t> send_mes, uint32_t size);
    void SocketClose (Ptr<Socket> socket);
    void SocketCloseSchedule (Time times, Ptr<Socket> socket);
  
    Ptr<Socket>     m_socket;
    bool            m_running;
    uint32_t        m_packetsSent;
    Ipv4Address 	  m_local;
    uint16_t 	      m_port;
    std::list<Ptr<Socket>> m_socketList;
    
    message_list    m_mes_list[TELNET_MESSAGE_MAX_SIZE];
};

MyServer::MyServer ()
  : m_socket (0), 
    m_running (true), 
    m_packetsSent (0),
    m_port(0)
{
  m_socketList.clear();
  for(size_t i=0; i<TELNET_MESSAGE_MAX_SIZE; i++){
    for(size_t j=0; j<MESSAGE_LIST_MAX_SIZE; j++){
      m_mes_list[i].hex_mes[j] = "";
      m_mes_list[i].uint8t_mes[j] = {};
    }
  }
  m_mes_list[0].hex_mes[0]    = "fffd03fffb18fffb1ffffb20fffb21fffb22fffb27fffd05fffb23";
  m_mes_list[0].uint8t_mes[0] = {0xff, 0xfd, 0x18, 0xff, 0xfd, 0x20, 0xff, 0xfd, 0x23, 0xff, 0xfd, 0x27};
  m_mes_list[0].uint8t_mes[1] = {0xff, 0xfd, 0x03, 0xff, 0xfd, 0x1f, 0xff, 0xff, 0xfd, 0x21, 0xff, 0xfe, 0x22, 0xff, 0xfb, 0x05, 0xff, 0xfa, 0x20, 0x01, 0xff, 0xf0, 0xff, 0xf0, 0xff, 0xfa, 0x23, 0x01, 0xff, 0xf0, 0xff, 0xfa, 0x27, 0x01, 0xff, 0xf0, 0xff, 0xfa, 0x18, 0x01, 0xff, 0xf0};
  
}
void MyServer::Setup (uint16_t port)
{
  m_port = port;
}
void MyServer::StartApplication (void)
{

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
  m_socket->SetRecvCallback (MakeCallback (&MyServer::HandleRead, this));
  m_socket->SetAcceptCallback(
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (), 
    MakeCallback(&MyServer::HandleAccept, this));
}
void MyServer::StopApplication (void)
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

void MyServer::HandleAccept(Ptr<Socket> socket, const Address& from){
  socket->SetRecvCallback (MakeCallback (&MyServer::HandleRead, this));
  m_socketList.push_back(socket);
}

void MyServer::SendPacket(Ptr<Socket> socket, std::vector<uint8_t> send_mes, uint32_t size){
  uint8_t *p = &send_mes[0];
  socket->Send(Create<Packet> (p, size));
}

void MyServer::SendSchedule (Time times, Ptr<Socket> socket, std::vector<uint8_t> send_mes, uint32_t size)
{
  if (m_running)
    {
      Simulator::Schedule (times, &MyServer::SendPacket, this, socket, send_mes, size);
    }
}

void MyServer::SocketClose(Ptr<Socket> socket)
{
  socket->Close();
}

void MyServer::SocketCloseSchedule (Time times, Ptr<Socket> socket)
{
  Simulator::Schedule (times, &MyServer::SocketClose, this, socket);
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
    memset(received_message, 0, sizeof(received_message));
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
class NodeIpAdder{
  public:
    NodeIpAdder(NetDeviceContainer NetDevice, Ipv4Address SendToIpFilter){
      this -> setSendToIP(SendToIpFilter);
      this -> setNetDevice(NetDevice);
    }
    void callback(Ptr<const Packet> p,  Ptr<Ipv4> protcol, uint32_t interface);
    void setSendToIP(Ipv4Address ip){ m_SendToIP = ip; }
    void setNetDevice(NetDeviceContainer NetDevice){ m_NetDevice = NetDevice; }
  private:
    NetDeviceContainer m_NetDevice;
    Ipv4Address m_SendToIP;

};

void NodeIpAdder::callback(Ptr<const Packet> p,  Ptr<Ipv4> protcol, uint32_t interface){
  Ipv4Header header;
  p->PeekHeader (header);
  NS_LOG_UNCOND ("======================================");
  NS_LOG_UNCOND ("S:" << header.GetSource() );
  NS_LOG_UNCOND ("D:" << header.GetDestination ());

  if(m_SendToIP != header.GetDestination()){
    Ptr<NetDevice> device_source = m_NetDevice.Get(0);
    Ptr<Node> node_source = device_source->GetNode ();
    Ptr<Ipv4> ipv4proto_source = node_source->GetObject<Ipv4> ();
    int32_t ifIndex_source = ipv4proto_source->GetInterfaceForDevice (device_source);
    if(ifIndex_source == -1){
      ifIndex_source = ipv4proto_source->AddInterface (device_source);
    }

    Ptr<NetDevice> device_app = m_NetDevice.Get(1);
    Ptr<Node> node = device_app->GetNode ();
    Ptr<Ipv4> ipv4proto = node->GetObject<Ipv4> ();
    int32_t ifIndex = ipv4proto->GetInterfaceForDevice (device_app);
    if(ifIndex == -1){
      ifIndex = ipv4proto->AddInterface (device_app);
    }

    ipv4proto_source->RemoveAddress(ifIndex_source, 1);
    ipv4proto->RemoveAddress(ifIndex, 1);

    const Ipv4Address ipv4address_tmp = header.GetDestination ();
    const Ipv4Mask ipv4_mask = Ipv4Mask ("/24");

    /* address collision */
    uint32_t new_adder = ipv4address_tmp.CombineMask (ipv4_mask).Get();
    new_adder = new_adder + 1;
    NS_LOG_UNCOND ("SNODE_NEW_ADDR IS " << Ipv4Address(new_adder));
    for(int i=1; new_adder == ipv4address_tmp.Get() && i <= 5; i++){
      new_adder = new_adder + i;
    }
    /*  */

    Ipv4InterfaceAddress ipv4Addr_source = Ipv4InterfaceAddress (Ipv4Address(new_adder), ipv4_mask);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (ipv4address_tmp, ipv4_mask);

    ipv4proto_source->AddAddress (ifIndex_source, ipv4Addr_source);
    ipv4proto_source->SetMetric (ifIndex_source, 1);
    ipv4proto_source->SetUp (ifIndex_source);

    ipv4proto->AddAddress (ifIndex, ipv4Addr);
    ipv4proto->SetMetric (ifIndex, 1);
    ipv4proto->SetUp (ifIndex);
  }
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

  LogComponentEnable("TcpL4Protocol", LOG_LEVEL_INFO);

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
  // Now, create the right side.
  //
  NodeContainer nodesRight;
  nodesRight.Create (1);

  CsmaHelper csmaRight;
  csmaRight.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csmaRight.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  ObjectFactory channelFactoryRight;
  channelFactoryRight.SetTypeId ("ns3::CsmaChannel");
  channelFactoryRight.Set("DataRate", DataRateValue (5000000));
  channelFactoryRight.Set("Delay", TimeValue (MilliSeconds (2)));
  Ptr<CsmaChannel> channelRight = channelFactoryRight.Create()->GetObject<CsmaChannel> ();
  NetDeviceContainer devicesRight = csmaRight.Install (nodesRight, channelRight);

  InternetStackHelper internetRight;
  internetRight.Install (nodesRight);

  Ipv4AddressHelper ipv4Right;
  ipv4Right.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesRight = ipv4Right.Assign (devicesRight);

  //
  // Stick in the point-to-point line between the sides.
  //
  
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("512kbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NodeContainer nodes = NodeContainer (nodesLeft.Get (1), nodesRight.Get (0));
  NetDeviceContainer devices = p2p.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.2.0", "255.255.255.192");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);  
  
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

  server_app->Setup (1302);
  nodesRight.Get (0)->AddApplication (server_app);
  server_app->SetStartTime (Seconds (1.0));  

  /* inj */
  
  NodeIpAdder adder_App(devices, "10.1.1.1");

  Ptr<Ipv4L3Protocol> ipv4Proto = nodesLeft.Get(1)->GetObject<Ipv4L3Protocol> ();
  ipv4Proto-> TraceConnectWithoutContext("Rx", MakeCallback(&NodeIpAdder:: callback, &adder_App));


  //
  // log_config
  //

  csmaLeft.EnablePcapAll ("fdas9Left", false);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (60.));
  Simulator::Run ();
  Simulator::Destroy ();
}
