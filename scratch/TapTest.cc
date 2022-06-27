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
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/tap-bridge-module.h"

#include "ns3/ipv4-l3-protocol.h"

#include "ns3/ipv4.h"

#include "ns3/dynamic-ip-server.h"
#include "ns3/dynamic-ip-callback-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TapDumbbellExample");

#define TELNET_MESSAGE_MAX_SIZE 5
#define MESSAGE_LIST_MAX_SIZE 3


class MyApp : public Application 
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (Ptr<Socket> socket, Time times, std::vector<uint8_t> send_mes);
  void SendPacket (Ptr<Socket> socket, std::vector<uint8_t> send_mes);
  void SendConnectMirai_Start(void);
  void SendConnectMirai_Name(Ptr<Socket> socket);
  void SendConnectMirai_0000(Ptr<Socket> socket);


  Ptr<Socket>       m_socket;
  Address           m_peer;
  uint32_t          m_packetSize;
  uint32_t          m_nPackets;
  DataRate          m_dataRate;
  EventId           m_sendEvent;
  bool              m_running;
  uint32_t          m_packetsSent;
  SequenceNumber32  sequence_num;
};

MyApp::MyApp ()
  : m_socket (0), 
    m_peer (), 
    m_packetSize (0), 
    m_nPackets (0), 
    m_dataRate (0), 
    m_sendEvent (), 
    m_running (false), 
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
  sequence_num = 0;

}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendConnectMirai_Start ();
}

void 
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void 
MyApp::SendPacket (Ptr<Socket> socket, std::vector<uint8_t> send_mes)
{
  uint8_t *p = &send_mes[0];
  //TcpHeader head;
  //head.SetFlags(24);
  Ptr<Packet> packet = Create<Packet> (p, send_mes.size());
  //packet->AddHeader(head);
  //NS_LOG_UNCOND("sEND");
  socket->Send (packet, 0);
}

void
MyApp::SendConnectMirai_Start(void)
{
  NS_LOG_UNCOND ("[SendPacket A]");
  SendPacket(m_socket, {0x00, 0x00, 0x00, 0x01});
  m_socket->SetRecvCallback (MakeCallback (&MyApp::SendConnectMirai_Name, this));
  ScheduleTx(m_socket, Seconds(5), {0x00, 0x00, 0x00, 0x01});
}

void
MyApp::SendConnectMirai_Name(Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while(packet = socket->RecvFrom (from)){
    Simulator::Cancel(m_sendEvent);
    NS_LOG_UNCOND ("[SendPacket B]");
    SendPacket(socket, {0x00});
    m_socket->SetRecvCallback (MakeCallback (&MyApp::SendConnectMirai_0000, this));
  }
}

void
MyApp::SendConnectMirai_0000(Ptr<Socket> socket)
{
  ScheduleTx(socket, Seconds(9), {0x00, 0x00});
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  std::string message;
  if(packet = socket->RecvFrom (from)){
    socket->GetSockName (localAddress);
    int packet_size = packet->GetSize();
    uint8_t received_message[packet_size];
    memset(received_message, 0, packet_size);
    packet->CopyData (received_message, packet_size);
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
    if(packet_size > 0){
      Simulator::Cancel(m_sendEvent);
    }
    if(hex.find("556e61626c6520746f2063") != std::string::npos){
      m_socket->Close ();
      m_socket->Bind ();
      m_socket->Connect (m_peer);
    }else if(hex.find("313034396c") != std::string::npos){
    }

  }
}

void 
MyApp::ScheduleTx (Ptr<Socket> socket ,Time times, std::vector<uint8_t> send_mes)
{
  if (m_running)
    {
      m_sendEvent = Simulator::Schedule (times, &MyApp::SendPacket, this, socket, send_mes);
    }
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


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
  // Create Dummy csma
  //

  //Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodesLeft.Get (1), Ipv4RawSocketFactory::GetTypeId ());
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodesLeft.Get (1), TcpSocketFactory::GetTypeId ());

  Address sinkAddress (InetSocketAddress ("74.11.180.205", 1312));

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, 5, DataRate ("1Mbps"));
  nodesLeft.Get (1)->AddApplication (app);
  app->SetStartTime (Seconds (1.));

  //
  // Default GW
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

  //
  // log_config
  //

  csmaLeft.EnablePcapAll ("fdas9Left", false);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (60.));
  Simulator::Run ();
  Simulator::Destroy ();
}
