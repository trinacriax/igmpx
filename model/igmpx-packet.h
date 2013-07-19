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

#ifndef __IGMPX_PACKET_H__
#define __IGMPX_PACKET_H__

#include <iostream>
#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <ns3/header.h>
#include <ns3/ipv4-address.h>
#include <ns3/enum.h>
#include <ns3/nstime.h>

namespace ns3
{
  namespace igmpx
  {
    const uint32_t IPV4_ADDRESS_SIZE = 4;
    const uint32_t IGMPX_IP_PROTOCOL_NUM = 104;
    const uint32_t IGMPX_PORT_NUM = 905;
    const std::string ALL_IGMPX_NODES("224.2.2.1");

    enum IGMPXType
    {
      IGMPX_REPORT = 11, IGMPX_ACCEPT = 12
    };

    /**
     * \brief IGMP-like packet format.
     *
     * This file contains the IGMP-like protocol packet format.
     *
     */

//	0               1               2               3
//	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	| Type 		|    Reserved   |           Checksum          |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
    class IGMPXHeader : public Header
    {
    public:
      IGMPXHeader();
      IGMPXHeader(IGMPXType type);

      virtual
      ~IGMPXHeader();

      /**
       * \returns The type of the igmp message.
       *
       * The type of the corresponding IGMP messages.
       */

      IGMPXType
      GetType() const;

      /**
       * \param type The type of the igmp message.
       *
       * Set the type of the igmp message.
       */
      void
      SetType(IGMPXType type);

    private:

      /// Type. Types for specific IGMP messages.
      IGMPXType m_type;
      uint8_t m_reserved;
      uint16_t m_checksum;

    public:
      ///\name Header serialization/deserialization
      //\{
      static TypeId GetTypeId(void);
      virtual TypeId GetInstanceTypeId(void) const;
      virtual void Print(std::ostream &os) const;
      virtual uint32_t GetSerializedSize(void) const;
      virtual void Serialize(Buffer::Iterator start) const;
      virtual uint32_t Deserialize(Buffer::Iterator start);
      //\}

//	0               1               2               3
//	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|               Multicast group address                         |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|                Unicast source address                         |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|                Upstream node address                          |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//

      struct IgmpReportMessage
      {
        Ipv4Address m_multicastGroupAddr;
        Ipv4Address m_sourceAddr;
        Ipv4Address m_upstreamAddr;

        void Print(std::ostream &os) const;
        uint32_t GetSerializedSize(void) const;
        void Serialize(Buffer::Iterator start) const;
        uint32_t Deserialize(Buffer::Iterator start, uint32_t messageSize);
      };

//	0               1               2               3
//	0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|               Multicast group address                         |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|                Unicast source address                         |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|              Downstream node address                          |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//

      struct IgmpAcceptMessage
      {
        Ipv4Address m_multicastGroupAddr;
        Ipv4Address m_sourceAddr;
        Ipv4Address m_downstreamAddr;

        void Print(std::ostream &os) const;
        uint32_t GetSerializedSize(void) const;
        void Serialize(Buffer::Iterator start) const;
        uint32_t Deserialize(Buffer::Iterator start, uint32_t messageSize);
      };

    private:
      struct
      {
        IgmpReportMessage igmpReport;
        IgmpAcceptMessage igmpAccept;
      } m_igmpx_message;

    public:

      IgmpReportMessage&
      GetIgmpReportMessage()
      {
        if (m_type == 0)
          {
            m_type = IGMPX_REPORT;
          }
        else
          {
            NS_ASSERT(m_type == IGMPX_REPORT);
          }
        return m_igmpx_message.igmpReport;
      }

      IgmpAcceptMessage&
      GetIgmpAcceptMessage()
      {
        if (m_type == 0)
          {
            m_type = IGMPX_ACCEPT;
          }
        else
          {
            NS_ASSERT(m_type == IGMPX_ACCEPT);
          }
        return m_igmpx_message.igmpAccept;
      }
    };

    static inline std::ostream&
    operator<<(std::ostream& os, const IGMPXHeader & packet)
    {
      packet.Print(os);
      return os;
    }

  }  //end namespace igmpx
}  //end namespace ns3

#endif
