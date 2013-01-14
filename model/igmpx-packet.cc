/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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

#include "igmpx-packet.h"
#include <ns3/assert.h>
#include <ns3/log.h>

namespace ns3
{
  namespace igmpx
  {

    NS_LOG_COMPONENT_DEFINE("IGMPXHeader");

    NS_OBJECT_ENSURE_REGISTERED(IGMPXHeader);

    IGMPXHeader::IGMPXHeader() :
        m_type(IGMPX_REPORT), m_reserved (0), m_checksum (0)
    {
    }

    IGMPXHeader::IGMPXHeader(IGMPXType type) :
        m_type(type), m_reserved (0), m_checksum (0)
    {
    }

    IGMPXHeader::~IGMPXHeader()
    {
    }

    TypeId
    IGMPXHeader::GetTypeId(void)
    {
      static TypeId tid =
          TypeId("ns3::igmpx::IGMPXHeader").SetParent<Header>().AddConstructor<IGMPXHeader>();
      return tid;
    }
    TypeId
    IGMPXHeader::GetInstanceTypeId(void) const
    {
      return GetTypeId();
    }

    IGMPXType
    IGMPXHeader::GetType() const
    {
      return m_type;
    }

    void
    IGMPXHeader::SetType(IGMPXType type)
    {
      m_type = type;
    }

    uint32_t
    IGMPXHeader::GetSerializedSize(void) const
    {
      uint32_t size = 4;
      switch (m_type)
        {
        case IGMPX_REPORT:
          size += m_igmpx_message.igmpReport.GetSerializedSize();
          break;
        case IGMPX_ACCEPT:
          size += m_igmpx_message.igmpAccept.GetSerializedSize();
          break;
        default:
          {
            NS_ASSERT(false);
            break;
          }
        }
      return size;
    }

    void
    IGMPXHeader::Print(std::ostream &os) const
    {
      os << "Type: " << (uint16_t) m_type
         << " Reserved: " << (uint16_t) m_reserved
         << " Checksum: " << m_checksum << "\n";
    }

    void
    IGMPXHeader::Serialize(Buffer::Iterator start) const
    {
      Buffer::Iterator i = start;
      i.WriteHtonU32(0);
      switch (m_type)
        {
        case IGMPX_REPORT:
          m_igmpx_message.igmpReport.Serialize(i);
          break;
        case IGMPX_ACCEPT:
          m_igmpx_message.igmpAccept.Serialize(i);
          break;
        default:
          {
            NS_ASSERT(false);
            break;
          }
        }
      i = start;
      i.Next(4);
      uint16_t checksum = i.CalculateIpChecksum(GetSerializedSize()-4, 0);
      i = start;
      i.WriteU8(m_type);
      i.WriteU8(m_reserved);
      i.WriteHtonU16(checksum);
    }

    uint32_t
    IGMPXHeader::Deserialize(Buffer::Iterator start)
    {
      Buffer::Iterator i = start;
      uint16_t checksum = 0;
      uint32_t size = 0;
      uint32_t message_size = 4;
      m_type = IGMPXType(i.ReadU8());
      m_reserved = i.ReadU8();
      m_checksum = i.ReadNtohU16();
      size += 4;
      NS_ASSERT(m_type >= IGMPX_REPORT && m_type<=IGMPX_ACCEPT);
      switch (m_type)
        {
        case IGMPX_REPORT:
          message_size += m_igmpx_message.igmpReport.GetSerializedSize();
          size += m_igmpx_message.igmpReport.Deserialize(i, message_size - size);
          break;
        case IGMPX_ACCEPT:
          message_size += m_igmpx_message.igmpAccept.GetSerializedSize();
          size += m_igmpx_message.igmpAccept.Deserialize(i, message_size - size);
          break;
        default:
          {
            NS_ASSERT(false);
            break;
          }
        }
      i = start;
      i.Next(4);
      checksum = i.CalculateIpChecksum(GetSerializedSize()-4, 0);
      NS_ASSERT_MSG (checksum == m_checksum, "Checksum error");
      return size;
    }

    uint32_t
    IGMPXHeader::IgmpReportMessage::GetSerializedSize(void) const
    {
      return 12;
    }

    void
    IGMPXHeader::IgmpReportMessage::Print(std::ostream &os) const
    {
      os << " Group = " << m_multicastGroupAddr << " Source = " << m_sourceAddr
          << " Upstream = " << m_upstreamAddr << "\n";
    }

    void
    IGMPXHeader::IgmpReportMessage::Serialize(Buffer::Iterator start) const
    {
      Buffer::Iterator i = start;
      i.WriteHtonU32(m_multicastGroupAddr.Get());
      i.WriteHtonU32(m_sourceAddr.Get());
      i.WriteHtonU32(m_upstreamAddr.Get());
    }

    uint32_t
    IGMPXHeader::IgmpReportMessage::Deserialize(Buffer::Iterator start,
        uint32_t messageSize)
    {
      Buffer::Iterator i = start;
      NS_ASSERT(messageSize == this->GetSerializedSize());
      uint32_t size;
      m_multicastGroupAddr = Ipv4Address(i.ReadNtohU32());
      size = 4;
      m_sourceAddr = Ipv4Address(i.ReadNtohU32());
      size += 4;
      m_upstreamAddr = Ipv4Address(i.ReadNtohU32());
      size += 4;
      return size;
    }

    uint32_t
    IGMPXHeader::IgmpAcceptMessage::GetSerializedSize(void) const
    {
      return 12;
    }

    void
    IGMPXHeader::IgmpAcceptMessage::Print(std::ostream &os) const
    {
      os << " Group = " << m_multicastGroupAddr << " Source = " << m_sourceAddr
          << " Downstream = " << m_downstreamAddr << "\n";
    }

    void
    IGMPXHeader::IgmpAcceptMessage::Serialize(Buffer::Iterator start) const
    {
      Buffer::Iterator i = start;
      i.WriteHtonU32(m_multicastGroupAddr.Get());
      i.WriteHtonU32(m_sourceAddr.Get());
      i.WriteHtonU32(m_downstreamAddr.Get());
    }

    uint32_t
    IGMPXHeader::IgmpAcceptMessage::Deserialize(Buffer::Iterator start,
        uint32_t messageSize)
    {
      Buffer::Iterator i = start;
      NS_ASSERT(messageSize == this->GetSerializedSize());
      uint32_t size;
      m_multicastGroupAddr = Ipv4Address(i.ReadNtohU32());
      size = 4;
      m_sourceAddr = Ipv4Address(i.ReadNtohU32());
      size += 4;
      m_downstreamAddr = Ipv4Address(i.ReadNtohU32());
      size += 4;
      return size;
    }

  } // namespace igmpx
} // namespace ns3
