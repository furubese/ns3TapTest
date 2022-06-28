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

  Ptr<DynamicIpServer> server_app_23 = CreateObject<DynamicIpServer> ();
  server_app_23->Setup (23, false);
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
