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

#include <ns3/test.h>
#include <ns3/igmpx-packet.h>
#include <ns3/packet.h>

namespace ns3
{

  class IGMPXReportTestCase : public TestCase
  {
    public:
      IGMPXReportTestCase ();
      virtual void
      DoRun (void);
  };

  IGMPXReportTestCase::IGMPXReportTestCase () :
      TestCase("Check IGMPXReport messages")
  {
  }
  void
  IGMPXReportTestCase::DoRun (void)
  {
    Packet packet;
      {
        std::cout << "Testing Report In Start..." << "\n";
        igmpx::IGMPXHeader msgIn(igmpx::IGMPX_REPORT);
        igmpx::IGMPXHeader::IgmpReportMessage &report = msgIn.GetIgmpReportMessage();
        report.m_multicastGroupAddr = Ipv4Address("225.1.2.3");
        report.m_sourceAddr = Ipv4Address("10.1.1.2");
        report.m_upstreamAddr = Ipv4Address("10.10.1.1");
        packet.AddHeader(msgIn);
        msgIn.Print(std::cout);
        report.Print(std::cout);
        std::cout << "Testing Report In End." << "\n";
      }
      {
        std::cout << "Testing Report Out Start..." << "\n";
        igmpx::IGMPXHeader msgOut;
        packet.RemoveHeader(msgOut);
        igmpx::IGMPXHeader::IgmpReportMessage &report = msgOut.GetIgmpReportMessage();
        msgOut.Print(std::cout);
        report.Print(std::cout);
        NS_TEST_ASSERT_MSG_EQ(msgOut.GetType(), igmpx::IGMPX_REPORT, "IGMP Type");
        NS_TEST_ASSERT_MSG_EQ(report.m_multicastGroupAddr, Ipv4Address ("225.1.2.3"), "Multicast Addr");
        NS_TEST_ASSERT_MSG_EQ(report.m_sourceAddr, Ipv4Address ("10.1.1.2"), "Source Addr");
        NS_TEST_ASSERT_MSG_EQ(report.m_upstreamAddr, Ipv4Address ("10.10.1.1"), "Upstream Addr");
        std::cout << "Testing Report Out End" << "\n";
      }
  }

  class IGMPXAcceptTestCase : public TestCase
  {
    public:
      IGMPXAcceptTestCase ();
      virtual void
      DoRun (void);
  };

  IGMPXAcceptTestCase::IGMPXAcceptTestCase () :
      TestCase("Check IGMPXAccept messages")
  {
  }
  void
  IGMPXAcceptTestCase::DoRun (void)
  {
    Packet packet;
      {
        std::cout << "Testing Accept In Start..." << "\n";
        igmpx::IGMPXHeader msgIn(igmpx::IGMPX_ACCEPT);
        igmpx::IGMPXHeader::IgmpAcceptMessage &accept = msgIn.GetIgmpAcceptMessage();
        accept.m_multicastGroupAddr = Ipv4Address("226.1.2.3");
        accept.m_sourceAddr = Ipv4Address("10.1.1.3");
        accept.m_downstreamAddr = Ipv4Address("10.10.2.1");
        packet.AddHeader(msgIn);
        msgIn.Print(std::cout);
        std::cout << "Testing Accept In End." << "\n";
      }
      {
        std::cout << "Testing Accept Out Start..." << "\n";
        igmpx::IGMPXHeader msgOut;
        packet.RemoveHeader(msgOut);
        msgOut.Print(std::cout);
        igmpx::IGMPXHeader::IgmpAcceptMessage &accept = msgOut.GetIgmpAcceptMessage();
        accept.Print(std::cout);
        NS_TEST_ASSERT_MSG_EQ(msgOut.GetType(), igmpx::IGMPX_ACCEPT, "IGMP Type");
        NS_TEST_ASSERT_MSG_EQ(accept.m_multicastGroupAddr, Ipv4Address ("226.1.2.3"), "Multicast Addr");
        NS_TEST_ASSERT_MSG_EQ(accept.m_sourceAddr, Ipv4Address ("10.1.1.3"), "Source Addr");
        NS_TEST_ASSERT_MSG_EQ(accept.m_downstreamAddr, Ipv4Address ("10.10.2.1"), "Upstream Addr");
        std::cout << "Testing Accept Out End" << "\n";
      }
  }

  static class IgmpxTestSuite : public TestSuite
  {
    public:
      IgmpxTestSuite ();
  } j_igmpxTestSuite;

  IgmpxTestSuite::IgmpxTestSuite () :
      TestSuite("igmpx-header", UNIT)
  {
    // RUN $ ./test.py -s igmpx-header -v -c unit 1
    AddTestCase(new IGMPXReportTestCase());
    AddTestCase(new IGMPXAcceptTestCase());
  }

} // namespace ns3

