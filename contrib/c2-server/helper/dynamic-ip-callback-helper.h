/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#include "ns3/net-device-container.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/node.h"

namespace ns3 {

class DynamicIpCallbackHelper {
public:
	DynamicIpCallbackHelper(
        Ptr<NetDevice> NetDevice,
        Ipv4Address SendToIpFilter
        );
    void setNetDevice(Ptr<NetDevice> NetDevice);
    void setSendToIP(Ipv4Address ip);
    void SetToCallback(Ptr<Node> a_node, DynamicIpCallbackHelper &thisobject);
private:
    void callback(Ptr<const Packet> p, Ptr<Ipv4> protcol, uint32_t interface);
    Ptr<NetDevice> m_NetDevice;
    Ipv4Address m_SendToIP;
};
}