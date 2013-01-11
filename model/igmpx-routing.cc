/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of Trento, Italy
 *                    University of California, Los Angeles, U.S.A.
 *
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
 *
 * Authors: Alessandro Russo <russo@disi.unitn.it>
 *          University of Trento, Italy
 *          University of California, Los Angeles U.S.A.
 */

#include "igmpx-routing.h"

#include <ns3/simulator.h>
#include <ns3/names.h>
#include <ns3/boolean.h>
#include <ns3/double.h>
#include <ns3/enum.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/log.h>
#include <ns3/tag.h>
#include <ns3/snr-tag.h>

#include "ns3/output-stream-wrapper.h"
#include <ns3/socket-factory.h>
#include <ns3/inet-socket-address.h>
#include <ns3/ipv4-header.h>
#include "ns3/ipv4-interface.h"
#include <ns3/ipv4-raw-socket-factory.h>
#include <ns3/ipv4-route.h>
#include <ns3/ipv4-routing-table-entry.h>
#include <ns3/udp-socket-factory.h>
#include <ns3/udp-socket.h>
#include <ns3/udp-header.h>
#include <ns3/udp-l4-protocol.h>


#include <iostream>
#include <limits.h>

namespace ns3
{
  namespace igmpx
  {
    NS_LOG_COMPONENT_DEFINE ("RoutingProtocol");

    NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

    RoutingProtocol::RoutingProtocol () :
        m_mainInterface (0), m_mainAddress (Ipv4Address::GetAny ()),
        m_stopTx (false), m_ipv4 (0), m_identification (0),
        m_routingProtocol (0), m_RoutingTable (0),
        m_startTime (0), m_role (CLIENT)
    {
      m_RoutingTable = Create<Ipv4StaticRouting> ();
      m_socketAddresses.clear ();
      m_igmpGroups.clear ();
    }

    RoutingProtocol::~RoutingProtocol ()
    {
    }

    TypeId
    RoutingProtocol::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::igmpx::RoutingProtocol").
          SetParent<Ipv4RoutingProtocol> ().AddConstructor<RoutingProtocol> ()
            .AddAttribute ("RegisterAsMember", "Register the node as a member of the group. Tuple (group, source, interface).",
                           StringValue ("0,0,0"), MakeStringAccessor (&RoutingProtocol::RegisterInterfaceString),
                           MakeStringChecker ())
            .AddAttribute ("UnRegisterAsMember", "UnRegister the node from the group. Tuple (group, source, interface).",
                           StringValue ("0,0,0"), MakeStringAccessor (&RoutingProtocol::UnregisterInterfaceString), MakeStringChecker ())
            .AddAttribute ("PeerRole", "Peer role.", EnumValue (CLIENT), MakeEnumAccessor (&RoutingProtocol::m_role),
                           MakeEnumChecker (CLIENT, "Node is a client.", ROUTER, "Node is a router."))
            .AddTraceSource ("IgmpxRxControl", "Trace Igmpx packet received.",
                           MakeTraceSourceAccessor (&RoutingProtocol::m_rxControlPacketTrace))
            .AddTraceSource ("IgmpxTxControl", "Trace Igmpx packet sent.",
                           MakeTraceSourceAccessor (&RoutingProtocol::m_txControlPacketTrace));
      return tid;
    }

    int32_t
    RoutingProtocol::GetMainInterface ()
    {
      return m_mainInterface;
    }

    void
    RoutingProtocol::SetInterfaceExclusions (std::set<uint32_t> exceptions)
    {}

    void
    RoutingProtocol::RegisterInterface (Ipv4Address source, Ipv4Address group, uint32_t interface)
    {
      NS_ASSERT (m_role == CLIENT);
      m_startTime = TransmissionDelay (0, IGMP_TIME*1000, Time::MS);
      NS_LOG_DEBUG ("Register interface  " << interface << " for (" << source << "," << group << ") Delay "<<m_startTime.GetSeconds());
      SourceGroupPair sgp (source, group);
      if (m_igmpGroups.find (sgp) == m_igmpGroups.end ())//check whether the SourceGroup pair has been registered
        {
          IgmpState istate (sgp); // Create a new source group element
          m_igmpGroups.insert (std::pair<SourceGroupPair, IgmpState> (sgp, istate));
        }
      // check whether the SourceGroup pair is registered on the given interface, otherwise create a new one.
      if (m_igmpGroups.find (sgp)->second.igmpMessage.find (interface) == m_igmpGroups.find (sgp)->second.igmpMessage.end ())
        {
          m_igmpGroups.find (sgp)->second.igmpMessage.insert (std::pair<uint32_t, Timer> (interface, Timer (Timer::CANCEL_ON_DESTROY)));
          m_igmpGroups.find (sgp)->second.igmpMessage.find (interface)->second.SetFunction (&RoutingProtocol::IgmpReportTimerExpire, this);
          m_igmpGroups.find (sgp)->second.igmpMessage.find(interface)->second.SetArguments (sgp, interface);
          m_igmpGroups.find (sgp)->second.igmpMessage.find(interface)->second.SetDelay (Seconds(IGMP_TIME));
          m_igmpGroups.find (sgp)->second.igmpMessage.find(interface)->second.Schedule (m_startTime);

          m_igmpGroups.find (sgp)->second.igmpRemove.insert (std::pair<uint32_t, Timer> (interface, Timer (Timer::CANCEL_ON_DESTROY)));
          m_igmpGroups.find (sgp)->second.igmpRemove.find(interface)->second.SetFunction (&RoutingProtocol::RemoveRouter, this);
          m_igmpGroups.find (sgp)->second.igmpRemove.find(interface)->second.SetDelay (Seconds (IGMP_TIMEOUT));
          m_igmpGroups.find (sgp)->second.igmpRemove.find(interface)->second.SetArguments (sgp, interface);

          m_igmpGroups.find (sgp)->second.igmpEvent.insert (std::pair<uint32_t, EventId> (interface, EventId () ));
        }
    }

    void
    RoutingProtocol::RegisterInterfaceString (std::string csv)
    {
      NS_LOG_FUNCTION (this);
      Ipv4Address group (Ipv4Address::GetAny ());
      Ipv4Address source (Ipv4Address::GetAny ());
      uint32_t interface = 0;
      std::vector<std::string> tokens;
      Tokenize (csv, tokens, ",");
      if (tokens.size () != 3)
        return;
      source = Ipv4Address (tokens.at (0).c_str ());
      group = Ipv4Address (tokens.at (1).c_str ());
      interface = atoi (tokens.at (2).c_str ());
      tokens.clear ();
      if (source == group && interface == 0)
        return; //skip initialization
      RegisterInterface (source, group, interface);
    }

    void
    RoutingProtocol::UnregisterInterface (Ipv4Address source, Ipv4Address group, uint32_t interface)
    {
      NS_ASSERT (m_role == CLIENT);
      NS_LOG_DEBUG ("UnRegister interface with members for (" << source << "," << group << ") over interface " << interface);
      SourceGroupPair sgp (source, group);
      if (m_igmpGroups.find (sgp)->second.igmpMessage.find (interface) != m_igmpGroups.find (sgp)->second.igmpMessage.end ())
        {
          m_igmpGroups.find (sgp)->second.igmpMessage.find (interface)->second.Cancel (); //cancel timer
          m_igmpGroups.find (sgp)->second.igmpMessage.erase (interface);
        }
    }

    void
    RoutingProtocol::UnregisterInterfaceString (std::string csv)
    {
      NS_LOG_FUNCTION (this);
      Ipv4Address group, source;
      uint32_t interface;
      std::vector<std::string> tokens;
      Tokenize (csv, tokens, ",");
      if (tokens.size () != 3)
        return;
      source = Ipv4Address (tokens.at (0).c_str ());
      group = Ipv4Address (tokens.at (1).c_str ());
      interface = atoi (tokens.at (2).c_str ());
      tokens.clear ();
      if (source == group && interface == 0)
        return; //skip initialization
      UnregisterInterface (source, group, interface);
    }

     void
     RoutingProtocol::RegisterCallback (Callback<void, Ipv4Address, Ipv4Address, uint32_t > callback)
     {
       m_registration = callback;
     }


     void
     RoutingProtocol::UnregisterCallback (Callback<void, Ipv4Address, Ipv4Address, uint32_t > callback)
     {
       m_unregistration = callback;
     }

    Ptr<Ipv4Route>
    RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif,
                                       Socket::SocketErrno &sockerr)
    {
      NS_LOG_FUNCTION (this << m_ipv4->GetObject<Node> ()->GetId () << header.GetDestination () << oif);
      Ptr<Ipv4Route> rtentry;
      sockerr = Socket::ERROR_NOROUTETOHOST;
      return rtentry;
    }

    bool
    RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                                      UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                      LocalDeliverCallback lcb, ErrorCallback ecb)
    {
      return false;
    }

    bool
    RoutingProtocol::IsMyOwnAddress (const Ipv4Address & address) const
    {
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
          j != m_socketAddresses.end (); ++j)
        {
          Ipv4InterfaceAddress iface = j->second;
          if (address == iface.GetLocal ())
            {
              return true;
            }
        }
      return false;
    }

    void
    RoutingProtocol::NotifyInterfaceUp (uint32_t i)
    {
      NS_LOG_FUNCTION (this << i);
      if (m_mainAddress == Ipv4Address ())
        {
          Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
          if (addr != Ipv4Address::GetLoopback ())
            {
              m_mainAddress = addr;
            }
        }
      NS_ASSERT (m_mainAddress != Ipv4Address ());
    }

    void
    RoutingProtocol::NotifyInterfaceDown (uint32_t i)
    {
      NS_LOG_FUNCTION (this << i);
    }

    void
    RoutingProtocol::NotifyAddAddress (uint32_t j, Ipv4InterfaceAddress address)
    {
      NS_LOG_FUNCTION (this << GetObject<Node> ()->GetId ());
      int32_t i = (int32_t) j;
      Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
      if (addr == Ipv4Address::GetLoopback ())
        return;
      // Create a socket to listen only on this Interface/Address
      Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (), Ipv4RawSocketFactory::GetTypeId ());
      socket->SetAttribute ("Protocol", UintegerValue (IGMPX_IP_PROTOCOL_NUM));
      socket->SetAttribute ("IpHeaderInclude", BooleanValue (true));
      socket->SetAllowBroadcast (true);
      InetSocketAddress inetAddr (IGMPX_PORT_NUM);
      socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvIGMPX, this));
      if (socket->Bind (inetAddr))
        {
          NS_FATAL_ERROR ("Failed to bind () IGMPX socket " << addr << ":" << IGMPX_PORT_NUM);
        }
      socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
      m_socketAddresses[socket] = m_ipv4->GetAddress (i, 0);
      NS_LOG_DEBUG ("Socket " << socket << " Device " << m_ipv4->GetNetDevice (i) << " Iface " << i
          << " Addr " << addr
          << " Broad " << addr.GetSubnetDirectedBroadcast (m_ipv4->GetAddress (i, 0).GetMask ())
      );
    }

    void
    RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
    {
      NS_LOG_FUNCTION (this<<interface);
    }

    void
    RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
    {
      NS_ASSERT (ipv4 != 0);
      NS_ASSERT (m_ipv4 == 0);
      m_ipv4 = ipv4;
      m_RoutingTable->SetIpv4 (ipv4);
    }

    Ipv4Address
    RoutingProtocol::GetLocalAddress (int32_t interface)
    {
      NS_ASSERT ((uint32_t)interface<m_ipv4->GetNInterfaces () && interface >= 0);
      return m_ipv4->GetAddress ( (uint32_t) interface, 0).GetLocal ();
    }

    void
    RoutingProtocol::DoDispose ()
    {
      for (std::map<SourceGroupPair, IgmpState>::iterator iter = m_igmpGroups.begin(); iter != m_igmpGroups.end();
          iter++)
        {
          for (std::map<uint32_t, Timer>::iterator iterM = iter->second.igmpMessage.begin();
              iterM != iter->second.igmpMessage.end(); iterM++)
            iterM->second.Cancel();
          iter->second.igmpMessage.clear();
          for (std::map<uint32_t, Timer>::iterator iterR = iter->second.igmpRemove.begin();
              iterR != iter->second.igmpRemove.end(); iterR++)
            iterR->second.Cancel();
          iter->second.igmpRemove.clear();
          for (std::map<uint32_t, EventId>::iterator iterE = iter->second.igmpEvent.begin();
              iterE != iter->second.igmpEvent.end(); iterE++)
            iterE->second.Cancel();
          iter->second.igmpEvent.clear();
        }
      m_igmpGroups.clear();
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::iterator iter = m_socketAddresses.begin ();
          iter != m_socketAddresses.end (); iter++)
        {
          iter->first->Close ();
        }
      m_socketAddresses.clear ();
      m_ipv4 = 0;
      Ipv4RoutingProtocol::DoDispose ();
    }

    void
    RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
    {
//      std::ostream* os = stream->GetStream ();
//      *os << "Group\t Source\t NextHop\t Interface\n";
//      *os << "\nStatic Routing Table:\n";
//      m_RoutingTable->PrintRoutingTable (stream);
    }

    void
    RoutingProtocol::DoStart ()
    {
      #ifdef IGMPTEST
        return;
      #endif
//      if (m_role == ROUTER && m_multicastProtocol == NULL)
//        {
//          Ptr<Ipv4RoutingProtocol> rp_Gw = (m_ipv4->GetRoutingProtocol ());
//          Ptr<Ipv4ListRouting> lrp_Gw = DynamicCast<Ipv4ListRouting> (rp_Gw);
//          for (uint32_t i = 0; i < lrp_Gw->GetNRoutingProtocols (); i++)
//            {
//              int16_t priority;
//              Ptr<Ipv4RoutingProtocol> temp = lrp_Gw->GetRoutingProtocol (i, priority);
//              if (DynamicCast<pimdm::MulticastRoutingProtocol> (temp))
//                {
//                  m_multicastProtocol = DynamicCast<pimdm::MulticastRoutingProtocol> (temp);
//                }
//            }
//        }
    }

    void
    RoutingProtocol::SendPacketIGMPXBroadcast (Ptr<Packet> packet, const IGMPXHeader &message, int32_t interface)
    {
      NS_LOG_FUNCTION (this);
      if (m_stopTx)
        return;
      packet->AddHeader (message);
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i = m_socketAddresses.begin ();
          i != m_socketAddresses.end (); i++)
        {
          if (m_ipv4->GetInterfaceForDevice (i->first->GetBoundNetDevice ()) == interface)
            {
              Ipv4Address bcast = i->second.GetLocal ().GetSubnetDirectedBroadcast (i->second.GetMask ());
              Ipv4Header ipv4header = BuildHeader (GetLocalAddress (interface), bcast, IGMPX_IP_PROTOCOL_NUM,
                  packet->GetSize (), 1, false);
              packet->AddHeader (ipv4header);
              NS_LOG_DEBUG ("Node " << GetLocalAddress (interface) << " is sending to " << bcast << ":" << IGMPX_PORT_NUM << ", Socket " << i->first);
              m_txControlPacketTrace (packet);
              i->first->SendTo (packet, 0, InetSocketAddress (bcast, IGMPX_PORT_NUM));
              break;
            }
        }
    }

    void
    RoutingProtocol::SendPacketIGMPXUnicast (Ptr<Packet> packet, const IGMPXHeader &message, int32_t interface,
                                                  Ipv4Address destination)
    {
      NS_LOG_FUNCTION (this);
      if (m_stopTx)
        return;
      packet->AddHeader (message);
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i = m_socketAddresses.begin ();
          i != m_socketAddresses.end (); i++)
        {
          if (m_ipv4->GetInterfaceForDevice (i->first->GetBoundNetDevice ()) == interface)
            {
              Ipv4Header ipv4header = BuildHeader (GetLocalAddress (interface), destination, IGMPX_IP_PROTOCOL_NUM,
                  packet->GetSize (), 1, false);
              packet->AddHeader (ipv4header);
              NS_LOG_DEBUG ("Node " << GetLocalAddress (interface) << " is sending to " << destination << ":" << IGMPX_PORT_NUM << ", Socket " << i->first);
              m_txControlPacketTrace (packet);
              i->first->SendTo (packet, 0, InetSocketAddress (destination, IGMPX_PORT_NUM));
              break;
            }
        }
    }

    void
    RoutingProtocol::IgmpReportTimerExpire (SourceGroupPair sgp, uint32_t interface)
    {
      NS_LOG_FUNCTION (this << sgp << interface << GetLocalAddress (interface));
      NS_ASSERT (m_role == CLIENT);
      NS_ASSERT (m_igmpGroups.find (sgp) != m_igmpGroups.end ());
      NS_ASSERT (m_igmpGroups.find (sgp)->second.igmpMessage.find(interface) != m_igmpGroups.find (sgp)->second.igmpMessage.end ());
      SendIgmpReport (sgp, interface);
      m_igmpGroups.find (sgp)->second.igmpMessage.find (interface)->second.Schedule ();
    }

    void
    RoutingProtocol::SendIgmpReport (SourceGroupPair sgp, uint32_t interface)
    {
      NS_LOG_FUNCTION (this << sgp << GetLocalAddress (interface) << interface);
      NS_ASSERT (m_role == CLIENT);
      Ipv4Address destination = m_igmpGroups.find(sgp)->second.sourceGroup.nextMulticastAddr;
      Ptr<Packet> packet = Create<Packet>();
      IGMPXHeader report(IGMPX_REPORT);
      IGMPXHeader::IgmpReportMessage &igmpReport = report.GetIgmpReportMessage();
      igmpReport.m_multicastGroupAddr = sgp.groupMulticastAddr;
      igmpReport.m_sourceAddr = sgp.sourceMulticastAddr;
      // Setting the upstream address to any means the client is looking for a router to which associate.
      igmpReport.m_upstreamAddr = destination;
      Time delay = TransmissionDelay();
      NS_LOG_INFO ("Node " << GetLocalAddress (interface) << " SendIgmpReport for "<< sgp
                   << " Interface " << interface << " Destination " << destination
                   << " at " << (Simulator::Now()+delay).GetSeconds()
                   << " Renew " << (Simulator::Now()+m_igmpGroups.find (sgp)->second.igmpMessage.find (interface)->second.GetDelay()).GetSeconds()
                   << " Running "<< m_igmpGroups.find (sgp)->second.igmpEvent.find(interface)->second.IsRunning());
      NS_ASSERT (!m_igmpGroups.find (sgp)->second.igmpEvent.find(interface)->second.IsRunning());
      m_igmpGroups.find (sgp)->second.igmpEvent.find(interface)->second = Simulator::Schedule (delay, &RoutingProtocol::SendPacketIGMPXBroadcast, this, packet, report, interface);
    }

    void
    RoutingProtocol::SendIgmpAccept (SourceGroupPair sgp, uint32_t interface, Ipv4Address clientIP)
    {
      NS_LOG_FUNCTION (this << sgp << interface << clientIP);
      NS_ASSERT (m_role == ROUTER);
      if (m_igmpGroups.find (sgp) != m_igmpGroups.end() &&
          m_igmpGroups.find (sgp)->second.igmpEvent.find(interface)->second.IsRunning())
        return;
      Ptr<Packet> packet = Create<Packet> ();
      IGMPXHeader accept (IGMPX_ACCEPT);
      IGMPXHeader::IgmpAcceptMessage &igmpAccept = accept.GetIgmpAcceptMessage ();
      igmpAccept.m_multicastGroupAddr = sgp.groupMulticastAddr;
      igmpAccept.m_sourceAddr = sgp.sourceMulticastAddr;
      igmpAccept.m_downstreamAddr = clientIP;
      Time delay = TransmissionDelay ();
      NS_LOG_INFO ("Router " << GetLocalAddress (interface) << " accepts clients for " << sgp << " on interface "<< interface << " delay " << delay.GetSeconds());
      if (m_igmpGroups.find (sgp) != m_igmpGroups.end())
        m_igmpGroups.find (sgp)->second.igmpEvent.find(interface)->second  = Simulator::Schedule (delay, &RoutingProtocol::SendPacketIGMPXBroadcast, this, packet, accept, interface);
    }

    void
    RoutingProtocol::RecvIgmpReport (IGMPXHeader::IgmpReportMessage &report, Ipv4Address sender,
                                          Ipv4Address receiver, uint32_t interface, double snr)
    {
      switch (m_role)
        {
        case CLIENT:
          {
            NS_LOG_FUNCTION (this << sender << receiver << interface);
            NS_ASSERT (m_role == CLIENT);
            NS_ASSERT (interface >0 && interface<m_ipv4->GetNInterfaces ());
            Ipv4Address source, group, next;
            source = report.m_sourceAddr;
            group = report.m_multicastGroupAddr;
            next = report.m_upstreamAddr;
            SourceGroupPair sgp (source, group, next);
            Ipv4Address router = m_igmpGroups.find (sgp)->second.sourceGroup.nextMulticastAddr;
            /*
             *  A node receives a report for the same sgp on the desired interface for the same router.
             *  Thus, another node is "refreshing" the subscription for sgp on the interface,
             *  such a node can be seen as a representative.
             *  Since the message has been sent in broadcast, it can be lost, thus:
             */
            if (router == next)
              {
                if (!m_igmpGroups.find(sgp)->second.igmpEvent.find(interface)->second.IsRunning())
                  {
                    // - The node just "shifts" its report messages, rescheduling the event for a few seconds.
                    Time delay = TransmissionDelay((IGMP_TIME * 200), IGMP_TIME * 1000, Time::MS);
                    if (m_igmpGroups.find(sgp)->second.igmpMessage.find(interface)->second.IsRunning())
                      m_igmpGroups.find(sgp)->second.igmpMessage.find(interface)->second.Cancel();
                    m_igmpGroups.find(sgp)->second.igmpMessage.find(interface)->second.Schedule(delay);
                    NS_LOG_INFO ("Client " << GetLocalAddress (interface) << " receives report for "<< sgp
                        << " on interface " << interface << " reschedule report at " << (Simulator::Now()+delay).GetSeconds());
                  }
                else
                  {
                    /*
                     * - The node is going to sent the report messages, cancel it.
                     * It the report will be lost (No accept), some client which have
                     * rescheduled it report, will refresh the subscription.
                     */
                    m_igmpGroups.find(sgp)->second.igmpEvent.find(interface)->second.Cancel();
                  }
              }
            break;
          }
        case ROUTER:
          {
            NS_LOG_FUNCTION (this << sender << receiver << interface);
            NS_ASSERT (m_role == ROUTER);
            NS_ASSERT (interface >0 && interface<m_ipv4->GetNInterfaces ());
            Ipv4Address source, group, next;
            source = report.m_sourceAddr;
            group = report.m_multicastGroupAddr;
            next = report.m_upstreamAddr;
            SourceGroupPair sgp (source, group);
            if (report.m_upstreamAddr == Ipv4Address::GetAny ())
              { // The client is looking for a ROUTER to register
                NS_LOG_INFO ("Router " << GetLocalAddress (interface) << " receives generic report from " << sender <<" ("<<snr<< ") member for " << sgp);
                SendIgmpAccept (sgp, interface, Ipv4Address::GetAny ());
              }
            else if (IsMyOwnAddress (report.m_upstreamAddr))
              { // The client provides the ROUTER address to register
                if (m_igmpGroups.find(sgp) != m_igmpGroups.end()
                    && m_igmpGroups.find(sgp)->second.igmpRemove.find(interface)
                        != m_igmpGroups.find(sgp)->second.igmpRemove.end())
                  NS_LOG_INFO ("Router " << GetLocalAddress (interface) << " receives renew report from " << sender <<" ("<<snr<< ") member for " << sgp);
                if (m_igmpGroups.find (sgp) == m_igmpGroups.end ())
                  { //add a source-group pair
                    m_igmpGroups.insert (std::pair<SourceGroupPair, IgmpState> (sgp, IgmpState (sgp)));
                    NS_LOG_DEBUG ("Adding Source-Group (" << source << "," << group << ") to the map for " << sender << ".");
                  }
                if (m_igmpGroups.find (sgp)->second.igmpRemove.find (interface)
                    == m_igmpGroups.find (sgp)->second.igmpRemove.end ())
                  {
                    /*
                     * The router didn't have any client on the interface.
                     * Now, there is a client, thus the corresponding interface is added.
                     * Note that the routers use the Timer to clean the clients list.
                     */
                    NS_LOG_DEBUG ("Adding Interface " << interface << " to the map for "<<sender);
                    NS_LOG_DEBUG ("Set the timer to remove clients");
                    m_igmpGroups.find (sgp)->second.igmpRemove.insert (std::pair<uint32_t, Timer> (interface, Timer (Timer::CANCEL_ON_DESTROY)));
                    m_igmpGroups.find (sgp)->second.igmpRemove.find (interface)->second.SetFunction (&RoutingProtocol::RemoveClient, this);
                    m_igmpGroups.find (sgp)->second.igmpRemove.find (interface)->second.SetDelay (Seconds (IGMP_TIMEOUT));
                    m_igmpGroups.find (sgp)->second.igmpRemove.find (interface)->second.SetArguments (sgp, interface);

                    m_igmpGroups.find (sgp)->second.igmpEvent.insert (std::pair<uint32_t, EventId> (interface, EventId () ));

                    if (!m_registration.IsNull())
                      m_registration (source, group, interface);
                    NS_LOG_INFO ("Router " << GetLocalAddress (interface) << " register client " << sender <<" ("<<snr<< ") as member of " << sgp);
                  }
                //Note that the routers use the Timer to clean the clients list.
                if (m_igmpGroups.find (sgp)->second.igmpRemove.find (interface)->second.IsRunning ())
                  m_igmpGroups.find (sgp)->second.igmpRemove.find (interface)->second.Cancel ();
                m_igmpGroups.find (sgp)->second.igmpRemove.find (interface)->second.Schedule ();
                SendIgmpAccept(sgp, interface, sender);
              }
            else
              {
                NS_LOG_INFO ("Message for another router " << next);
              }
            break;
          }
        default:
          {
            NS_ASSERT_MSG (false, "State not valid "<< m_role);
            break;
          }
        }
    }

    void
    RoutingProtocol::RecvIgmpAccept (IGMPXHeader::IgmpAcceptMessage &accept, Ipv4Address sender,
                                          Ipv4Address receiver, uint32_t interface, double snr)
    {
      switch (m_role)
        {
        case CLIENT:
          {
            NS_LOG_FUNCTION (this);
            NS_ASSERT (m_role == CLIENT);
            NS_ASSERT (interface >0 && interface<m_ipv4->GetNInterfaces ());
            Ipv4Address source, group, downstream;
            source = accept.m_sourceAddr;
            group = accept.m_multicastGroupAddr;
            downstream = accept.m_downstreamAddr; // for future use.
            SourceGroupPair sgp (source, group);
            NS_LOG_INFO ("Node " << sender << " accepts " << receiver << " for " << sgp);
            if (m_igmpGroups.find (sgp) == m_igmpGroups.end ())
              {
                NS_LOG_DEBUG ("Client " << receiver << " receives accept: SKIP because is not interested in "<<sgp);
                return;//not interested in the group
              }
            Ipv4Address router = m_igmpGroups.find (sgp)->second.sourceGroup.nextMulticastAddr;
            double rsnr = m_igmpGroups.find (sgp)->second.sourceGroup.snrNext;
            if (sender == router)
              {
                /*
                 * Receive an accept from the associated router.
                 * The client updates the SNR, if lower than threshold, restart the association process.
                 */
                NS_LOG_DEBUG ("Client " << receiver << " receives accept: UPDATE " << router << " (" << rsnr << ") to " << router << " (" << snr << ")");
                m_igmpGroups.find (sgp)->second.sourceGroup.snrNext = snr;
                if (m_igmpGroups.find (sgp)->second.igmpMessage.find (interface)->second.IsRunning ())
                  m_igmpGroups.find (sgp)->second.igmpMessage.find (interface)->second.Cancel ();
                if (snr < IGMP_SNR_THRESHOLD)
                  { // if SNR lower threshold, reset the router and restart the process for a new one.
                    m_igmpGroups.find (sgp)->second.sourceGroup.nextMulticastAddr = Ipv4Address::GetAny();
                    m_igmpGroups.find (sgp)->second.sourceGroup.snrNext = 0.0;
                    if (m_igmpGroups.find(sgp)->second.igmpEvent.find(interface)->second.IsRunning())
                      m_igmpGroups.find(sgp)->second.igmpEvent.find(interface)->second.Cancel();
                    m_igmpGroups.find (sgp)->second.igmpMessage.find (interface)->second.Schedule (TransmissionDelay());
                    NS_LOG_DEBUG ("Client " << receiver << " receives accept: SNR too low, looking for a new candidate");
                  }
                else
                  { // router's SNR is fine, reschedule report msg.
                    Time delay = TransmissionDelay(IGMP_TIME*1000, 2000*IGMP_TIME, Time::MS);
                    NS_LOG_DEBUG ("Client " << receiver << " receives accept: reschedule the report message to " << router
//                        << " from "<< (Simulator::Now()+m_igmpGroups.find(sgp)->second.igmpMessage.find(interface)->second.GetDelay()-m_igmpGroups.find(sgp)->second.igmpMessage.find(interface)->second.GetDelayLeft()).GetSeconds()
                        << " to " <<(Simulator::Now()+delay).GetSeconds() <<" d="<<delay.GetSeconds()<< "["<<IGMP_TIME<<":"<<(2*IGMP_TIME)<<"]");
                    if (m_igmpGroups.find(sgp)->second.igmpEvent.find(interface)->second.IsRunning())
                      m_igmpGroups.find(sgp)->second.igmpEvent.find(interface)->second.Cancel();
                    m_igmpGroups.find(sgp)->second.igmpMessage.find(interface)->second.Schedule (delay);
                    if (m_igmpGroups.find(sgp)->second.igmpRemove.find(interface)->second.IsRunning()) //cancel old timer
                      m_igmpGroups.find(sgp)->second.igmpRemove.find(interface)->second.Cancel();
                    m_igmpGroups.find(sgp)->second.igmpRemove.find(interface)->second.Schedule ();
                  }
                NS_LOG_INFO ("Node " << receiver << " reg. to "<<router<<": SAME ROUTER");
              }
            else if ( (rsnr / snr) < IGMP_SNR_RATIO)
              {
                /*
                 * Receive an accept from a different router router.
                 * Such a router has a better SNR than the one associated,
                 * change the associated router to the new one,
                 * sending an association message.
                 */
                m_igmpGroups.find(sgp)->second.sourceGroup.nextMulticastAddr = sender; // set this candidate
                m_igmpGroups.find(sgp)->second.sourceGroup.snrNext = snr; // set candidate's SNR
                NS_LOG_DEBUG ("Client " << receiver << " receives accept: CHANGE router from " << router
                    << " (" << rsnr << ") -> to " << sender << " (" << snr << ")");
                if (m_igmpGroups.find (sgp)->second.igmpRemove.find(interface)->second.IsRunning ()) //cancel old timer
                  m_igmpGroups.find (sgp)->second.igmpRemove.find(interface)->second.Cancel ();
                m_igmpGroups.find (sgp)->second.igmpRemove.find(interface)->second.Schedule ();
              }
            else
              {
                NS_LOG_DEBUG ("Client " << receiver << " receives accept: LOWER router from "<< sender << " (" << snr << ") << of " << router << " (" << rsnr <<")");
              }
            break;
          }
        case ROUTER:
          {
            break;
          }
        default:
          {
            NS_ASSERT_MSG (false, "State not valid "<< m_role);
            break;
          }
        }
    }

    void
    RoutingProtocol::RemoveRouter (SourceGroupPair sgp, uint32_t interface)
    {
      NS_LOG_FUNCTION (this << sgp << interface);
      NS_ASSERT(m_role == CLIENT);
      NS_ASSERT(interface >0 && interface<m_ipv4->GetNInterfaces ());
      NS_LOG_INFO ("Remove router " << m_igmpGroups.find (sgp)->second.sourceGroup.nextMulticastAddr << " for " << sgp);
      m_igmpGroups.find(sgp)->second.sourceGroup.nextMulticastAddr = Ipv4Address::GetAny();
      m_igmpGroups.find(sgp)->second.sourceGroup.snrNext = 0;
      m_igmpGroups.find(sgp)->second.igmpEvent.find(interface)->second.Cancel();
      Simulator::ScheduleNow(&RoutingProtocol::SendIgmpReport, this, sgp, interface);
    }

    void
    RoutingProtocol::RemoveClient (SourceGroupPair sgp, uint32_t interface)
    {
      NS_LOG_FUNCTION (this << sgp << interface);
      NS_ASSERT (m_role == ROUTER);
      NS_ASSERT(m_igmpGroups.find (sgp) != m_igmpGroups.end ());
      NS_ASSERT(!m_igmpGroups.find (sgp)->second.igmpRemove.find (interface)->second.IsRunning());
      m_igmpGroups.find(sgp)->second.igmpEvent.find(interface)->second.Cancel();
      m_igmpGroups.find(sgp)->second.igmpRemove.find(interface)->second.Cancel();
      m_igmpGroups.erase(sgp);
      NS_LOG_INFO ("Node " << GetLocalAddress (interface) << " removes group " << sgp);
      if (!m_unregistration.IsNull())
        m_unregistration (sgp.sourceMulticastAddr, sgp.groupMulticastAddr, interface);

    }

    void
    RoutingProtocol::RecvIGMPX (Ptr<Socket> socket)
    {
      NS_LOG_FUNCTION (this);
      Ptr<Packet> receivedPacket;
      Address sourceAddress;
      receivedPacket = socket->RecvFrom (sourceAddress);
      InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
      Ipv4Address senderIfaceAddr = inetSourceAddr.GetIpv4 ();
      int32_t interface = m_ipv4->GetInterfaceForDevice (socket->GetBoundNetDevice ());
      NS_ASSERT (interface);
      Ipv4Address receiverIfaceAddr = m_ipv4->GetAddress (interface, 0).GetLocal ();
      NS_ASSERT (receiverIfaceAddr != Ipv4Address ());
      m_rxControlPacketTrace (receivedPacket);
      Ipv4Header ipv4header;
      receivedPacket->RemoveHeader (ipv4header);
      Ipv4Address subnet = ipv4header.GetDestination ();
      Ipv4Address local = GetLocalAddress (interface);
      NS_LOG_DEBUG ("Sender = " << senderIfaceAddr << " Receiver = " << receiverIfaceAddr << ", Subnet = " << subnet);
      IGMPXHeader igmpxPacket;
      receivedPacket->RemoveHeader (igmpxPacket);
      switch (igmpxPacket.GetType ())
        {
        case IGMPX_REPORT:
          {
            SnrTag snrTag;
            receivedPacket->RemovePacketTag (snrTag);
            RecvIgmpReport (igmpxPacket.GetIgmpReportMessage (), senderIfaceAddr, receiverIfaceAddr, interface,
                snrTag.GetSinr ());
            break;
          }
        case IGMPX_ACCEPT:
          {
            if (m_role == ROUTER)
              return;
            SnrTag ptag;
            receivedPacket->RemovePacketTag (ptag);
            RecvIgmpAccept (igmpxPacket.GetIgmpAcceptMessage (), senderIfaceAddr, receiverIfaceAddr, interface,
                ptag.GetSinr ());
            break;
          }
        default:
          {
            NS_LOG_ERROR ("Packet unrecognized.... " << receivedPacket << "Sender " << senderIfaceAddr << ", Destination " << receiverIfaceAddr);
            break;
          }
        }
    }

    void
    RoutingProtocol::Tokenize (const std::string& str, std::vector<std::string>& tokens,
                                    const std::string& delimiters)
    {
      std::string::size_type lastPos = str.find_first_not_of (delimiters, 0);
      std::string::size_type pos = str.find_first_of (delimiters, lastPos);
      while (std::string::npos != pos || std::string::npos != lastPos)
        {
          // Found a token, add it to the vector.
          tokens.push_back (str.substr (lastPos, pos - lastPos));
          NS_LOG_DEBUG ("\"" << str.substr (lastPos, pos - lastPos) << "\"" << " POS " << pos << " LAST " << lastPos);
          // Skip delimiters.
          lastPos = str.find_first_not_of (delimiters, pos);
          // Find next.
          pos = str.find_first_of (delimiters, lastPos);
        }
    }

    Ipv4Header
    RoutingProtocol::BuildHeader (Ipv4Address source, Ipv4Address destination, uint8_t protocol,
                                       uint16_t payloadSize, uint8_t ttl, bool mayFragment)
    {
      Ipv4Header ipv4header;
      ipv4header.SetSource (source);
      ipv4header.SetDestination (destination);
      ipv4header.SetProtocol (protocol);
      ipv4header.SetPayloadSize (payloadSize);
      ipv4header.SetTtl ( (ttl > 0 ? ttl : 1));
      if (mayFragment)
        {
          ipv4header.SetMayFragment ();
          ipv4header.SetIdentification (m_identification);
          m_identification++;
        }
      else
        {
          ipv4header.SetDontFragment ();
          ipv4header.SetIdentification (m_identification);
          m_identification++;
        }
      if (Node::ChecksumEnabled ())
        {
          ipv4header.EnableChecksum ();
        }
      return ipv4header;
    }

    Time
    RoutingProtocol::TransmissionDelay ()
    {
      return TransmissionDelay (0, 1000, Time::MS);
    }

    Time
    RoutingProtocol::TransmissionDelay (double l, double u, enum Time::Unit unit)
    {
      double delay = UniformVariable ().GetValue (l, u);
      Time delayms = Time::FromDouble (delay, unit);
      return delayms;
    }

  } // namespace  igmpx
} // namespace ns3
