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

  Ptr<DynamicIpServer> server_app = CreateObject<DynamicIpServer> ();

  server_app->Setup (1302);
  nodesRight.Get (0)->AddApplication (server_app);
  server_app->SetStartTime (Seconds (1.0));  

  /* inj */
  
  DynamicIpCallbackHelper c2_callback_helper(devices, "10.1.1.1");
  c2_callback_helper.SetToCallback(nodesLeft.Get(1), c2_callback_helper);

  //Ptr<Ipv4L3Protocol> ipv4Proto = nodesLeft.Get(1)->GetObject<Ipv4L3Protocol> ();
  //ipv4Proto-> TraceConnectWithoutContext("Rx", MakeCallback(&NodeIpAdder:: callback, &adder_App));


  //
  // log_config
  //

  csmaLeft.EnablePcapAll ("fdas9Left", false);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (60.));
  Simulator::Run ();
  Simulator::Destroy ();
}
