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

#ifndef IGMPX_HELPER_H
#define IGMPX_HELPER_H

#include <ns3/object-factory.h>
#include <ns3/node.h>
#include <ns3/node-container.h>
#include <ns3/ipv4-routing-helper.h>
#include <ns3/igmpx-routing.h>
#include <map>
#include <set>

namespace ns3
{
  /**
   * \brief Helper class for IGMP-like protocol.
   */
  class IgmpxHelper : public Ipv4RoutingHelper
  {
    public:
      /**
       * Create an IgmpxHelper to install the IGMP-like protocol.
       */
      IgmpxHelper ();

      /**
       * \brief Construct an Helper from another previously initialized instance
       * (Copy Constructor).
       */
      IgmpxHelper (const IgmpxHelper &);

      /**
       * \internal
       * \returns pointer to clone of this Helper
       *
       * This method is mainly for internal use by the other helpers;
       * clients are expected to free the dynamic memory allocated by this method
       */
      IgmpxHelper*
      Copy (void) const;

      /**
       * \param node the node for which an exception is to be defined
       * \param interface an interface of node on which is not to be installed
       *
       * This method allows the user to specify an interface on which is not to be installed on
       */
      void
      ExcludeInterface (Ptr<Node> node, uint32_t interface);

      /**
       * \param node the node on which the routing protocol will run
       * \returns a newly-created routing protocol
       *
       */
      virtual Ptr<Ipv4RoutingProtocol>
      Create (Ptr<Node> node) const;

      /**
       * \param name the name of the attribute to set
       * \param value the value of the attribute to set.
       *
       */
      void
      Set (std::string name, const AttributeValue &value);

    private:
      /**
       * \internal
       * \brief Assignment operator declared private and not implemented to disallow
       * assignment and prevent the compiler from happily inserting its own.
       */
      IgmpxHelper &
      operator = (const IgmpxHelper &o);
      ObjectFactory m_agentFactory;

      std::map<Ptr<Node>, std::set<uint32_t> > m_interfaceExclusions;
  };

} // namespace ns3

#endif
