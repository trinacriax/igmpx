/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of Trento, Italy
 * 					  University of California, Los Angeles, U.S.A.
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

#ifndef __IGMPX_ROUTING_H__
#define __IGMPX_ROUTING_H__

#include "igmpx-packet.h"
#include <ns3/uinteger.h>
#include <ns3/random-variable.h>
#include <ns3/object.h>
#include <ns3/packet.h>
#include <ns3/node.h>
#include <ns3/socket.h>
#include <ns3/event-garbage-collector.h>
#include <ns3/timer.h>
#include <ns3/traced-callback.h>
#include <ns3/ipv4.h>
#include <ns3/ipv4-routing-protocol.h>
#include <ns3/ipv4-static-routing.h>
#include <ns3/ipv4-list-routing.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/string.h>
#include <ns3/pimdm-routing.h>
#include <ns3/video-push.h>
#include <ns3/gnuplot.h>
#include <ns3/application-container.h>

#include <fstream>
#include <vector>
#include <set>
#include <algorithm>
#include <string>
#include <iterator>

#ifndef IGMPTEST
#define IGMPTEST 0
#endif
namespace ns3
{
  namespace igmpx
  {
    const uint32_t IGMP_TIME = 10;
    const double IGMP_TIMEOUT = 3 * IGMP_TIME;
    const uint32_t IGMP_SNR_THRESHOLD = 1000;
    const double IGMP_SNR_RATIO = 0.70;

    enum PeerRole
    {
      CLIENT, ROUTER
    };

    struct SourceGroupPair
    {
        SourceGroupPair () :
            sourceMulticastAddr(Ipv4Address::GetAny()), groupMulticastAddr(Ipv4Address::GetAny()),
            nextMulticastAddr(Ipv4Address::GetAny()), snrNext(0.0)
        {
        }
        SourceGroupPair (Ipv4Address s, Ipv4Address g) :
            sourceMulticastAddr(s), groupMulticastAddr(g), nextMulticastAddr(Ipv4Address::GetAny()), snrNext(0.0)
        {
          NS_ASSERT (g.IsMulticast());
          NS_ASSERT (!s.IsMulticast());
        }
        SourceGroupPair (Ipv4Address s, Ipv4Address g, Ipv4Address n) :
            sourceMulticastAddr(s), groupMulticastAddr(g), nextMulticastAddr(n), snrNext(0.0)
        {
          NS_ASSERT (g.IsMulticast());
          NS_ASSERT (!s.IsMulticast());
        }
        /// Multicast Source address.
        Ipv4Address sourceMulticastAddr;
        /// Multicast group address.
        Ipv4Address groupMulticastAddr;
        /// Associated router address.
        Ipv4Address nextMulticastAddr;
        /// Router's SNR value.
        double snrNext;
    };

    static inline bool
    operator == (const SourceGroupPair &a, const SourceGroupPair &b)
    {
      return (a.sourceMulticastAddr == b.sourceMulticastAddr) && (a.groupMulticastAddr == b.groupMulticastAddr);
    }

    static inline bool
    operator < (const SourceGroupPair &a, const SourceGroupPair &b)
    {
      return (a.groupMulticastAddr < b.groupMulticastAddr)
          || ((a.groupMulticastAddr == b.groupMulticastAddr) && (a.sourceMulticastAddr < b.sourceMulticastAddr));
    }

    static inline std::ostream&
    operator << (std::ostream &os, const SourceGroupPair &a)
    {
      os << "SourceGroupPair (SourceAddress = " << a.sourceMulticastAddr << ", GroupAddress = " << a.groupMulticastAddr
          << ")";
      return os;
    }

    struct IgmpState
    {
        SourceGroupPair sourceGroup; /// SourceGroup pair.
        std::map<uint32_t, Timer> igmpMessage; /// <Interface, Timer > map used by clients send the report messages.
        std::map<uint32_t, Timer> igmpRemove; /// <Interface, Timer > map used by routers to clean clients.
        std::map<uint32_t, EventId> igmpEvent; /// <Interface, EventId > map used to track messages.

        IgmpState (SourceGroupPair sgp) :
            sourceGroup(sgp)
        {
          igmpMessage.clear();
          igmpRemove.clear();
          igmpEvent.clear();
        }

        ~IgmpState ()
        {
        }
    };

    static inline bool
    operator == (const IgmpState &a, const IgmpState &b)
    {
      return (a.sourceGroup == b.sourceGroup);
    }

    /**
     * \brief Define the IGMP-like protocol.
     *
     * Define the IGMP-like protocol as an Ipv4RoutingProtocol.
     * Such a protocol runs on all the interfaces,
     * the exclusion of several interface is not yet implemented.
     *
     * The clients have a set of source-group tuple
     * registered on each interface, meaning that they are
     * willing to receive multicast traffic belonging to
     * that source-group pair from that interface.
     *
     * The protocol works as follow:
     * - The CLIENT sends a Report for a given
     *   Source-Group pair on a given interface in broadcast,
     *   providing the Ip address of the upstream router,
     *   set to ANY if no routers are available.
     *
     * - The ROUTER receive the message:
     *   - if ANY, replies with the accept for the Source-Group pair
     *     on that interface in broadcast for that client;
     *   - if its address, Add the Source-Group pair and
     *     the interface, then set a timeout to clean the clients list,
     *     finally, sends the accept message for that client;
     *   - if another address, SKIP.
     *
     * - A CLIENT receives the accept message:
     *   - the message is from the associated router for this client OR another client:
     *     - update router's lifetime and SNR;
     *     - check SNR level, if too low, the clients looks for a new router to associate.
     *     - update the renew timer to send the report to the router.
     *
     *   - the message is NOT from the associated router:
     *     - if the SNR ratio between the associated router and this router is over the threshold
     *       - change the associated router to the new one, sending a report message for this router
     *
     */

    class RoutingProtocol : public Ipv4RoutingProtocol
    {
      private:
        int32_t m_mainInterface; ///< Node main interface. Right now it runs on all interfaces
        Ipv4Address m_mainAddress; ///< Main address on the main interface.
        std::map<SourceGroupPair, IgmpState> m_igmpGroups; ///< Map of SGP->State
        bool m_stopTx;
        Ptr<Ipv4> m_ipv4; ///< Node IP Protocol.
        uint32_t m_identification; ///< Identification counter for IPv4 header.
        ///< Raw socket per each IP interface, map socket -> iface address (IP + mask)
        std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketAddresses;
        /// Pointer to socket.
        Ptr<Ipv4RoutingProtocol>* m_routingProtocol;
        /// Loopback device used to defer RREQ until packet will be fully formed
        Ptr<Ipv4StaticRouting> m_RoutingTable;
        Time m_startTime; ///< Node start time.
        PeerRole m_role; ///< Node role.
//        Ptr<Ipv4RoutingProtocol> m_multicastProtocol;
        TracedCallback<Ptr<const Packet> > m_rxControlPacketTrace;
        TracedCallback<Ptr<const Packet> > m_txControlPacketTrace;
        Callback<void, Ipv4Address, Ipv4Address, uint32_t > m_registration;
        Callback<void, Ipv4Address, Ipv4Address, uint32_t > m_unregistration;

      protected:
        virtual void
        DoStart (void);

      public:
        static TypeId
        GetTypeId (void);

        RoutingProtocol ();

        virtual ~RoutingProtocol ();

        /**
         * \returns Main IGMP interface.
         *
         * Get the main interface running IGMP-like protocol.
         *
         */
        int32_t
        GetMainInterface ();

        /**
         *
         * @param exceptions Not IGMP interface.
         *
         * Define the interface that are not IGMP-enabled.
         * TODO: To implement.
         *
         */
        void SetInterfaceExclusions (std::set<uint32_t> exceptions);

        /**
         *
         * \param str String to parse.
         * \param tokens Vector to store tokens.
         * \param delimiters toke delimiters in the string.
         *
         * Parse a string for Source-Group-Interface tuple.
         *
         */
        void
        Tokenize (const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = ",");

        /**
         *
         * \param SGI Source-Group-Interface tuple string.
         *
         * Register a new Source-Group-Interface tuple string.
         *
         */
        void
        RegisterInterfaceString (std::string SGI);

        /**
         *
         * \param source Multicast Source to register.
         * \param group Multicast Group to register.
         * \param interface Interface to register.
         *
         * Register a new Source-Group-Interface tuple string.
         *
         */
        void
        RegisterInterface (Ipv4Address source, Ipv4Address group, uint32_t interface);

        /**
         *
         * \param SGI Source-Group-Interface tuple string.
         *
         * Deregister a new Source-Group-Interface tuplestring .
         *
         */
        void
        UnregisterInterfaceString (std::string SGI);

        /**
         *
         * \param source Multicast Source to register.
         * \param group Multicast Group to register.
         * \param interface Interface to register.
         *
         * Deregister a new Source-Group-Interface tuple string.
         *
         */
        void
        UnregisterInterface (Ipv4Address source, Ipv4Address group, uint32_t interface);

        /**
         *
         * @param callaback Callback to invoke whenever a source-group-interface has clients.
         *
         * Invoke a given callback whenever a the router has clients for a source-group-interface tuple.
         */
        void RegisterCallback (Callback<void, Ipv4Address, Ipv4Address, uint32_t > callback);

        /**
         *
         * @param callaback Callback to invoke whenever a source-group-interface has NO clients.
         *
         * Invoke a given callback whenever a the router has NO clients for a source-group-interface tuple.
         */
        void UnregisterCallback (Callback<void, Ipv4Address, Ipv4Address, uint32_t > callback);

      private:
        void
        Clear ();

        /**
         *
         * \param interface Interface.
         * \returns Ipv4Address associated to the interface.
         *
         * Get the Ipv4address address associated to the interface.
         *
         */
        Ipv4Address
        GetLocalAddress (int32_t interface);

        // From Ipv4RoutingProtocol
        virtual Ptr<Ipv4Route>
        RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

        // From Ipv4RoutingProtocol
        virtual bool
        RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                    UnicastForwardCallback ucb, MulticastForwardCallback mcb, LocalDeliverCallback lcb,
                    ErrorCallback ecb);

        // From Ipv4RoutingProtocol
        virtual void
        NotifyInterfaceUp (uint32_t interface);
        // From Ipv4RoutingProtocol
        virtual void
        NotifyInterfaceDown (uint32_t interface);
        // From Ipv4RoutingProtocol
        virtual void
        NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
        // From Ipv4RoutingProtocol
        virtual void
        NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
        // From Ipv4RoutingProtocol
        virtual void
        SetIpv4 (Ptr<Ipv4> ipv4);
        // From Ipv4RoutingProtocol
        virtual void
        PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;

        void
        DoDispose ();

        /**
         *
         * \param a Ipv4 address.
         * \returns True, if associate to some interface, false otherwise.
         *
         * Check whether the Ipv4Address is associated to any interface.
         *
         */
        bool
        IsMyOwnAddress (const Ipv4Address & a) const;

        /**
         *
         * \param source
         * \param destination
         * \param protocol
         * \param payloadSize
         * \param ttl
         * \param mayFragment
         * \returns Ipv4 header.
         *
         * Build the corresponding IPv4Header.
         *
         */

        Ipv4Header
        BuildHeader (Ipv4Address source, Ipv4Address destination, uint8_t protocol, uint16_t payloadSize, uint8_t ttl,
                     bool mayFragment);

        /**
         *
         * \param socket
         *
         * Parse a IGMP message, following the protocol rules.
         *
         */
        void
        RecvIGMPX (Ptr<Socket> socket);

        /**
         *
         * \param packet Packet to send.
         * \param message Message to add to the packet.
         * \param interface Output interface.
         *
         * Add the message to a packet which is sent on the specific interface in broadcast.
         *
         */
        void
        SendPacketIGMPXBroadcast (Ptr<Packet> packet, const IGMPXHeader &message, int32_t interface);

        /**
         *
         * \param packet Packet to send.
         * \param message Message to add to the packet.
         * \param interface Output interface.
         * \param destination Target node.
         *
         * Add the message to a packet which is sent on the specific interface in unicast.
         *
         */
        void
        SendPacketIGMPXUnicast (Ptr<Packet> packet, const IGMPXHeader &message, int32_t interface,
                                Ipv4Address destination);

        /**
         *
         * \param sgp Target Source-Group pair.
         * \param interface Target interface.
         *
         * Send an IgmpReport message for the specific sgp and interface.
         *
         */
        void
        IgmpReportTimerExpire (SourceGroupPair sgp, uint32_t interface);

        /**
         *
         * \param sgp Target Source-Group pair.
         * \param interface Target interface.
         *
         * Send an IgmpReport for a specific sgp - interface pair in broadcast.
         *
         */
        void
        SendIgmpReport (SourceGroupPair sgp, uint32_t interface);

        /**
         *
         * \param sgp Target Source-Group pair.
         * \param interface Target interface.
         *
         * Remove all the clients associated to a specific sgp - interface pair.
         *
         */

        void
        RemoveClient (SourceGroupPair sgp, uint32_t interface);

        /**
         *
         * \param report Igmp message.
         * \param sender Sender Address.
         * \param receiver Receiver Address.
         * \param interface Inbound interface.
         * \param snr Message SNR.
         *
         * Parse and compute a Report message.
         *
         */
        void
        RecvIgmpReport (IGMPXHeader::IgmpReportMessage &report, Ipv4Address sender, Ipv4Address receiver,
                        uint32_t interface, double snr);

        /**
         *
         * \param sgp Target Source-Group pair.
         * \param interface Target interface.
         * \param destination Target node.
         *
         * Send an IGMP accept message to a given destination.
         *
         */
        void
        SendIgmpAccept (SourceGroupPair sgp, uint32_t interface, Ipv4Address destination);

        /**
         *
         * \param accept Igmp Message.
         * \param sender Sender Address.
         * \param receiver Received Address.
         * \param interface Inbound interface.
         * \param snr Message SNR.
         *
         * Parse and compute the accept message.
         *
         */
        void
        RecvIgmpAccept (IGMPXHeader::IgmpAcceptMessage &accept, Ipv4Address sender, Ipv4Address receiver,
                        uint32_t interface, double snr);

        /**
         *
         * \param sgp Target Source-Group pair.
         * \param interface Target interface.
         *
         * The client removes the router associated to the SourceGroup-Interface pair.
         *
         */
        void
        RemoveRouter (SourceGroupPair sgp, uint32_t interface);

        /**
         *
         * \param l Minimum delay value.
         * \param u Maximum delay value.
         * \param unit Time unit.
         * \returns Random delay.
         *
         * Provide a random delay between a minimum and a maximum with a given time unit.
         *
         */
        Time
        TransmissionDelay (double l, double u, enum Time::Unit unit);

        /**
         *
         * \returns Random delay.
         *
         * Provide a random delay between zero a one second in milliseconds.
         *
         */
        Time
        TransmissionDelay ();

    };
  } // namespace  igmpx
} // namespace  ns3
#endif
