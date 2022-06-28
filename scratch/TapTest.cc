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

enum message_state{
      USERNAME,
      PASSWORD,
      SUCCESS,
      CONNECT
};

typedef struct {
  std::string hex_mes[MESSAGE_LIST_MAX_SIZE];
  std::vector<uint8_t> uint8t_mes[MESSAGE_LIST_MAX_SIZE];
}message_list;

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
    void HandleClose(Ptr<Socket> socket);

  
    Ptr<Socket>     m_socket;
    bool            m_running;
    uint32_t        m_packetsSent;
    Ipv4Address 	  m_local;
    uint16_t 	      m_port;
    std::list<Ptr<Socket>> m_socketList;
    message_state   m_state;

    
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
      m_mes_list[i].uint8t_mes[j] = {0x00};
    }
  }
  m_mes_list[0].hex_mes[0]    = "fffd03fffb18fffb1ffffb20fffb21fffb22fffb27fffd05fffb23";
  m_mes_list[0].uint8t_mes[0] = {0xff, 0xfd, 0x18, 0xff, 0xfd, 0x20, 0xff, 0xfd, 0x23, 0xff, 0xfd, 0x27};
  m_mes_list[0].uint8t_mes[1] = {0xff, 0xfd, 0x03, 0xff, 0xfd, 0x1f, 0xff, 0xff, 0xfd, 0x21, 0xff, 0xfe, 0x22, 0xff, 0xfb, 0x05, 0xff, 0xfa, 0x20, 0x01, 0xff, 0xf0, 0xff, 0xf0, 0xff, 0xfa, 0x23, 0x01, 0xff, 0xf0, 0xff, 0xfa, 0x27, 0x01, 0xff, 0xf0, 0xff, 0xfa, 0x18, 0x01, 0xff, 0xf0};

}
void MyServer::Setup (uint16_t port)
{
  m_port = port;
  m_state = USERNAME;
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
  //m_socket->SetCloseCallbacks(
    //MakeCallback(&MyServer::HandleClose, this),
    //MakeCallback(&MyServer::HandleClose, this));
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
  if(m_socketList.empty()){
    socket->SetRecvCallback (MakeCallback (&MyServer::HandleRead, this));
    m_socketList.push_back(socket);
    NS_LOG_UNCOND (" [ Connection Success ] ");
    std::vector<uint8_t> send_mes_8t;
    send_mes_8t = {0xff, 0xfd, 0x01};
    SendSchedule(Seconds(0.1), socket, send_mes_8t, send_mes_8t.size());
  }
}
void MyServer::HandleClose(Ptr<Socket> socket){
  StopApplication();
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
    //NS_LOG_UNCOND (" [ Client<Rx:Telnet> ] " << hex);

    std::vector<uint8_t> send_mes_8t;

    if(hex == "fffc01" || std::string(hex).find("0d0a") != std::string::npos){
      if      (m_state == USERNAME){
        send_mes_8t = {0x55, 0x73, 0x65, 0x72, 0x6e, 0x61, 0x6d, 0x65, 0x3a};
        if(std::string(received_message_strings).find("root") != std::string::npos){
          m_state = PASSWORD;
        }
      }
      else if (m_state == PASSWORD){
        send_mes_8t = {0x50, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64, 0x3a};
        m_state = SUCCESS;
      }
      else if (m_state == SUCCESS){
        send_mes_8t = {0xff, 0xfb, 03, 0x57,0x65,0x6c,0x63,0x6f,0x6d,0x20,0x6e,0x73,0x33,0x20,0x4e,0x6f,0x64,0x65, 0x0d, 0x0a, 0x72, 0x6f, 0x6f, 0x74, 0x40, 0x6e, 0x73,0x33,0x4e,0x6f,0x64,0x65,0x3a,0x7e,0x24};
        m_state = CONNECT;
      }
      else if (m_state == CONNECT){
        send_mes_8t = {0x55, 0x73, 0x65, 0x72, 0x6e, 0x61, 0x6d, 0x65, 0x3a, 0x20};
      }
      //std::vector<uint8_t> send_mes_8t_1 = {0x0d, 0x0a};
      //SendSchedule(Seconds(0.02), socket, send_mes_8t_1, send_mes_8t_1.size());
      SendSchedule(Seconds(0.04), socket, send_mes_8t, send_mes_8t.size());
    }else if(hex=="fffd01"){  // Do Message
      //Welcome ns3 Node
      //Username:
      send_mes_8t = {0xff, 0xfb, 03, 0x57,0x65,0x6c,0x63,0x6f,0x6d,0x20,0x6e,0x73,0x33,0x20,0x4e,0x6f,0x64,0x65, 0x0d, 0x0a, 0x55, 0x73, 0x65, 0x72, 0x6e, 0x61, 0x6d, 0x65, 0x3a};
      SendSchedule(Seconds(0.1), socket, send_mes_8t, send_mes_8t.size());
    }//else if(hex == "fffc01"){ //Won't Echo
    //  send_mes_8t = {0xff, 0xfd, 0x01};
    //  SendSchedule(Seconds(0.1), socket, send_mes_8t, send_mes_8t.size());
    //}
  }
}
class NodeIpAdder{
  public:
    NodeIpAdder(NetDeviceContainer NetDevice, Ipv4Address SendToIP){
      this -> setSendToIP(SendToIP);
      this -> setNetDevice(NetDevice);
    }
    bool callback(Ptr<Packet> p, Ipv4Header &header);
    void setSendToIP(Ipv4Address ip){ m_SendToIP = ip; }
    void setNetDevice(NetDeviceContainer NetDevice){ m_NetDevice = NetDevice; }
  private:
    NetDeviceContainer m_NetDevice;
    Ipv4Address m_SendToIP;

};

bool NodeIpAdder::callback(Ptr<Packet> p, Ipv4Header &header){
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
  p->AddHeader(header);

  return true;
}

///////////////////////////////////////

///////////////////////////////////////

///////////////////////////////////////

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
  CsmaHelper dummy_csma;
  NetDeviceContainer dummy_Devices;
  dummy_Devices = dummy_csma.Install (nodesLeft.Get (1));
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.2.0", "255.255.255.192");

  Ipv4InterfaceContainer interfaces = ipv4.Assign (dummy_Devices);

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

  Ptr<MyServer> server_app_23 = CreateObject<MyServer> ();
  server_app_23->Setup (23);
  nodesLeft.Get (1)->AddApplication (server_app_23);
  server_app_23->SetStartTime (Seconds (1.0));  
  Ptr<DynamicIpServer> server_app_2323 = CreateObject<DynamicIpServer> ();
  server_app_2323->Setup (2323, false);
  nodesLeft.Get (1)->AddApplication (server_app_2323);
  server_app_2323->SetStartTime (Seconds (1.0));
  Ptr<DynamicIpServer> server_app = CreateObject<DynamicIpServer> ();
  server_app->Setup (1302, true);
  nodesLeft.Get (1)->AddApplication (server_app);
  server_app->SetStartTime (Seconds (1.0));


  /* inj */
  
  DynamicIpCallbackHelper c2_callback_helper(dummy_Devices.Get (0), "10.1.1.1");
  c2_callback_helper.SetToCallback(nodesLeft.Get(1), c2_callback_helper);

  //
  // log_config
  //

  csmaLeft.EnablePcapAll ("fdas9Left", false);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (60.));
  Simulator::Run ();
  Simulator::Destroy ();
}
