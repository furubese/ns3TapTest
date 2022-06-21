
#include "ns3/net-device-container.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ptr.h"
#include "dynamic-ip-callback-helper.h"
#include "ns3/node.h"

namespace ns3 {
    DynamicIpCallbackHelper::DynamicIpCallbackHelper(
        Ptr<NetDevice> NetDevice,
        Ipv4Address SendToIpFilter)
        {
            this -> setNetDevice(NetDevice);
            this -> setSendToIP(SendToIpFilter);
            m_lock = false;
            m_count_lock = 0;
        }
    
    void DynamicIpCallbackHelper::setNetDevice(Ptr<NetDevice> NetDevice){ m_NetDevice = NetDevice; }
    void DynamicIpCallbackHelper::setSendToIP(Ipv4Address ip){ m_SendToIP = ip; }

    void DynamicIpCallbackHelper::SetToCallback(Ptr<Node> a_node, DynamicIpCallbackHelper &thisobject)
    {
        Ptr<Ipv4L3Protocol> ipv4Proto = a_node->GetObject<Ipv4L3Protocol> ();
        ipv4Proto-> TraceConnectWithoutContext("Rx", MakeCallback(&DynamicIpCallbackHelper:: callback, &thisobject));
    }

    void DynamicIpCallbackHelper::callback(Ptr<const Packet> p, Ptr<Ipv4> protcol, uint32_t interface)
    {
        if(m_lock){
            return;
        }
        if(m_count_lock > 2000){
            return;
        }
        m_count_lock += 1;
        Ipv4Header header;
        p->PeekHeader (header);
        //NS_LOG_UNCOND ("======================================");
        //NS_LOG_UNCOND ("S:" << header.GetSource() );
        //NS_LOG_UNCOND ("D:" << header.GetDestination ());
        if(m_SendToIP == header.GetDestination()){return;}
        Ptr<NetDevice> device_app = m_NetDevice;
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

}
