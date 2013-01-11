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

#ifndef NS3_LOG_ENABLE
#define NS3_LOG_ENABLE
#endif

#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <sstream>

#include <ns3/igmpx-helper.h>

#include <ns3/string.h>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/tools-module.h>
#include <ns3/wifi-module.h>
#include <ns3/applications-module.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/yans-error-rate-model.h>
#include <ns3/csma-helper.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("IgmpxTest");

#ifdef IGMPTEST
#undef IGMPTEST
#define IGMPTEST 1
#endif

struct mycontext
{
    uint32_t id;
    std::string callback;
};


class Experiment
{
public:
  Experiment ();
  Experiment (std::string name);//, uint32_t type);
  Gnuplot2dDataset Run (const uint32_t sizeRouter, const uint32_t sizeClient, const WifiHelper &wifi, const YansWifiPhyHelper &wifiPhy,
                        const NqosWifiMacHelper &wifiMac, const YansWifiChannelHelper &wifiChannel,
                        const double range, const double totalTime, const uint32_t mobility);
private:
  struct mycontext GetContextInfo (std::string context);
  void GenericPacketTrace (std::string context, Ptr<const Packet> p);
  Gnuplot2dDataset m_output;
  uint32_t m_type;
  // gnuplot file
  uint64_t m_txSlot;
  double m_tx;
};

Experiment::Experiment ():
    m_type (0), m_txSlot(0), m_tx(0)
{
}

Experiment::Experiment (std::string name):
  m_output (name), m_type (0), m_txSlot(0), m_tx(0)
{
  m_output.SetStyle (Gnuplot2dDataset::LINES);
}


struct mycontext
Experiment::GetContextInfo (std::string context)
{
  struct mycontext mcontext;
  int p2 = context.find("/");
  int p1 = context.find("/", p2 + 1);
  p2 = context.find("/", p1 + 1);
  mcontext.id = atoi(context.substr(p1 + 1, (p2 - p1) - 1).c_str());
  p1 = context.find_last_of("/");
  p2 = context.length() - p2;
  mcontext.callback = context.substr(p1 + 1, p2).c_str();
  return mcontext;
}

void
Experiment::GenericPacketTrace (std::string context, Ptr<const Packet> p)
{
  struct mycontext mc = GetContextInfo(context);
  uint64_t tslot = Simulator::Now().GetSeconds();
  NS_LOG_INFO("Node " << mc.id << " had " << mc.callback << " PacketID=" << p->GetUid() << " PacketSize=" << p->GetSize()<< " Slot "<<tslot);
  if (mc.callback.compare("IgmpxTxControl") == 0)
    {
      while (m_txSlot < tslot)
        {
          m_output.Add(m_txSlot++, m_tx / 1000);
          m_tx = 0.0;
        }
      m_tx += p->GetSize();
    }
  else if (mc.callback.compare("IgmpxRxControl") == 0)
    {
    }
}

Gnuplot2dDataset
Experiment::Run (const uint32_t sizeRouter, const uint32_t sizeClient, const WifiHelper &wifi,
                 const YansWifiPhyHelper &wifiPhy, const NqosWifiMacHelper &wifiMac,
                 const YansWifiChannelHelper &wifiChannel, const double range, const double totalTime, const uint32_t mobility)
{
  NS_LOG_INFO ("Create nodes Router "<<sizeRouter<< " Clients "<<sizeClient<<" Source 1 ");

  NodeContainer routers;
  routers.Create(sizeRouter);

  NodeContainer clients;
  clients.Create(sizeClient);

  NS_LOG_INFO("Installing WIFI router and clients.");
  NetDeviceContainer routersNetDev = wifi.Install(wifiPhy, wifiMac, routers);
  NetDeviceContainer clientsNetDev = wifi.Install(wifiPhy, wifiMac, clients);

  NS_LOG_INFO("Installing Internet Stack.");
  IgmpxHelper igmpxStack;

  /* ROUTERS */
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper listRouters;
  listRouters.Add(staticRouting, 0);
  listRouters.Add(igmpxStack, 1);

  InternetStackHelper internetRouters;
  internetRouters.SetRoutingHelper(listRouters);
  internetRouters.Install(routers);

  Ipv4ListRoutingHelper listSource;
  listSource.Add(staticRouting, 11);

  /* CLIENTS */
  Ipv4ListRoutingHelper listClients;
  listClients.Add(staticRouting, 0);
  listClients.Add(igmpxStack, 1);

  InternetStackHelper internetClients;
  internetClients.SetRoutingHelper(listClients);
  internetClients.Install(clients);

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  Ipv4Address base = Ipv4Address("10.0.0.0");
  Ipv4Mask mask = Ipv4Mask("255.0.0.0");
  ipv4.SetBase(base, mask);
  Ipv4InterfaceContainer ipRouter = ipv4.Assign(routersNetDev);
  Ipv4InterfaceContainer ipClient = ipv4.Assign(clientsNetDev);

  NS_LOG_INFO ("Configure multicasting.");
  Ipv4Address multicastGroup("225.1.2.4");
  Ipv4Address multicastSource("10.0.1.1");

  std::stringstream ss;
  /*
   * Define source, group, interface
   * */
  //ROUTERS
  ss << multicastSource << "," << multicastGroup;
  for (uint32_t n = 0; n < routers.GetN(); n++)
    {
      std::stringstream command;
      command << "NodeList/" << routers.Get(n)->GetId() << "/$ns3::igmpx::RoutingProtocol/PeerRole";
      Config::Set(command.str(), EnumValue(igmpx::ROUTER));
    }
  // CLIENTS
  ss.str("");
  ss << multicastSource << "," << multicastGroup << "," << "1";
  for (uint32_t n = 0; n < clients.GetN(); n++)
    { //Clients are RN nodes
      std::stringstream command;
      command << "/NodeList/" << clients.Get(n)->GetId() << "/$ns3::igmpx::RoutingProtocol/PeerRole";
      Config::Set(command.str(), EnumValue(igmpx::CLIENT));
      command.str("");
      command << "/NodeList/" << clients.Get(n)->GetId() << "/$ns3::igmpx::RoutingProtocol/RegisterAsMember";
      Config::Set(command.str(), StringValue(ss.str()));
    }

  Config::Connect("/NodeList/*/$ns3::igmpx::RoutingProtocol/IgmpxTxControl", MakeCallback(&Experiment::GenericPacketTrace, this));
//  Config::Connect("/NodeList/*/$ns3::igmpx::RoutingProtocol/IgmpxRxControl",  MakeCallback(&Experiment::GenericPacketTrace, this));

  NS_LOG_INFO("Installing Position and Mobility.");
  uint32_t cols = (uint16_t) ceil(sqrt(sizeRouter));
  cols = (cols == 0 ? 1 : cols);

  MobilityHelper mobilityR;
  mobilityR.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityR.SetPositionAllocator("ns3::GridPositionAllocator", "MinX", DoubleValue(0.0), "MinY", DoubleValue(0.0),
      "DeltaX", DoubleValue(range), "DeltaY", DoubleValue(range), "GridWidth", UintegerValue(cols), "LayoutType",
      StringValue("RowFirst"));
  mobilityR.Install(routers);

  Vector rPos[sizeRouter];

  for (uint32_t i = 0; i < sizeRouter; i++)
    {
      Ptr<MobilityModel> mobility = routers.Get(i)->GetObject<MobilityModel>();
      rPos[i] = mobility->GetPosition(); // Get position
      NS_LOG_DEBUG ("Position Router ["<<i<<"] = ("<< rPos[i].x << ", " << rPos[i].y<<", "<<rPos[i].z<<")");
    }

  if (mobility == 0) // Fixed Nodes.
    {
      NS_LOG_DEBUG ("Position Router ["<<0<<"] = ("<< rPos[0].x << ", " << rPos[0].y<<", "<<rPos[0].z<<")");
      MobilityHelper mobilityC;
      mobilityC.SetMobilityModel("ns3::ConstantPositionMobilityModel");
      Ptr<ConstantRandomVariable> xval = CreateObject<ConstantRandomVariable> ();
      xval->SetAttribute("Constant",DoubleValue(rPos[0].x));
      Ptr<ConstantRandomVariable> yval = CreateObject<ConstantRandomVariable>();
      yval->SetAttribute("Constant", DoubleValue(rPos[0].y+10));
      Ptr<ConstantRandomVariable> zval = CreateObject<ConstantRandomVariable>();
      zval->SetAttribute("Constant", DoubleValue(rPos[0].z));
      mobilityC.SetPositionAllocator("ns3::RandomBoxPositionAllocator",
                                "X",PointerValue(xval),
                                "Y",PointerValue(yval),
                                "Z",PointerValue(zval));
      mobilityC.Install(clients);
    }
  else // Roaming clients. Move from the first to the second, the third and fourth, finally back to the first.
    {
      uint32_t groupPoints = 4;
      Ptr<ListPositionAllocator> positionAllocG[sizeClient];
      for (int g = 0; g < sizeClient; g++)
        {
          std::stringstream poi;
          positionAllocG[g] = CreateObject<ListPositionAllocator>();
          poi << "Group[" << g << "] POI: ";
          for (int k = 0; k < groupPoints; k++)
            {
              int d = (g + (k + 1)) % sizeRouter;
              positionAllocG[g]->Add(Vector(rPos[d].x, rPos[d].y + 10, rPos[d].z));
              poi << "(" << rPos[d].x << "," << rPos[d].y << "," << rPos[d].z << ") ";
            }
            NS_LOG_INFO (poi.str());
        }

      NodeContainer groups[sizeClient];
      for (uint32_t c = 0; c < sizeClient; c++)
        {
          groups[c].Add(clients.Get(c));
        }

      double vpause = 20; // Pause time
      double vspeed = 3.0; // Client's speed
      Ptr<ConstantRandomVariable> pause = CreateObject<ConstantRandomVariable>();
      pause->SetAttribute("Constant", DoubleValue(vpause));
      Ptr<ConstantRandomVariable> speed = CreateObject<ConstantRandomVariable>();
      speed->SetAttribute("Constant", DoubleValue(vspeed));

      for (int g = 0; g < sizeClient; g++)
        {
          Ptr<ListPositionAllocator> positionAllocC = CreateObject<ListPositionAllocator>();
          positionAllocC->Add(Vector(rPos[g % sizeRouter].x, rPos[g % sizeRouter].y + 10, rPos[g % sizeRouter].z));
          MobilityHelper mobilityG;
          mobilityG.SetPositionAllocator(positionAllocC);
          NS_LOG_DEBUG ("Node "<< g << " Pause " << vpause << "s, Speed " << vspeed << "m/s");
          mobilityG.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                                     "Pause", PointerValue(pause),
                                     "Speed", PointerValue(speed),
                                     "PositionAllocator", PointerValue(positionAllocG[g]));
          mobilityG.Install(groups[g]);
        }
    }

    for (uint32_t i = 0; i < sizeClient; i++)
    {
      Ptr<MobilityModel> mobility = clients.Get(i)->GetObject<MobilityModel>();
      Vector pos = mobility->GetPosition(); // Get position
      NS_LOG_INFO("Position Client ["<<i<<"] = ("<< pos.x << ", " << pos.y<<", "<<pos.z<<")");
    }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(totalTime));
  Simulator::Run();
  NS_LOG_INFO ("Done.");
  uint64_t tslot = Simulator::Now().GetSeconds();
  while (m_txSlot < tslot)
    {
      m_output.Add(m_txSlot++, m_tx / 1000);
      m_tx = 0.0;
    }
  m_output.Add(m_txSlot, m_tx / 1000);
  Simulator::Destroy();
return  m_output;
}

int
main (int argc, char *argv[])
{
  LogComponentEnable("IgmpxTest", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
  LogComponentEnable("IgmpxRoutingProtocol",
      LogLevel(LOG_LEVEL_INFO | LOG_LEVEL_DEBUG | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
  LogComponentEnable("YansWifiPhy", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
  LogComponentEnable("InterferenceHelper",
      LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
  LogComponentEnable("PropagationLossModel",
      LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
  LogComponentEnable("YansWifiChannel", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));

  // Number of router nodes
  uint32_t sizeRouter = 4;
// Number of client nodes
  uint32_t sizeClient = 1;
  //Routing protocol 1) OLSR, 2) AODV, 3) MBN-AODV
  int32_t routing = 1;
  //Seed for random numbers
  uint32_t seed = 190569531;
  //Seed Run -> 10 runs!
  uint32_t run = 1;
  // reference loss
  double PLref = 30.0;
  // loss exponent
  double PLexp = 3.5;
  // Tx power start
  double TxStart = 16.0;
  // Tx power end
  double TxEnd = 16.0;
  // Tx power levels
  uint32_t TxLevels = 1;
  // Energy detection threshold
  double EnergyDet = -95.0;
  // CCA mode 1
  double CCAMode1 = -62.0;
  //Step in the grid X
  double range = 30;
  // Simulation time, seconds
  double totalTime = 100;
  // mobility scenario
  uint32_t mobility = 0;


  CommandLine cmd;
  cmd.AddValue("seed", "Seed Random.", seed);
  cmd.AddValue("run", "Seed Run.", run);
  cmd.AddValue("sizeRouter", "Number of router nodes.", sizeRouter);
  cmd.AddValue("sizeClient", "Number of Clients.", sizeClient);
  cmd.AddValue("routing", "Routing protocol used.", routing);
  cmd.AddValue("mobility", "Mobility 0)no 1)yes.", mobility);
  cmd.AddValue("range", "Cover range in meters.", range);
  cmd.AddValue("time", "Simulation time, s.", totalTime);
  cmd.AddValue("PLref", "Reference path loss dB.", PLref);
  cmd.AddValue("PLexp", "Path loss exponent.", PLexp);
  cmd.AddValue("TxStart", "Transmission power start dBm.", TxStart);
  cmd.AddValue("TxEnd", "Transmission power end dBm.", TxEnd);
  cmd.AddValue("TxLevels", "Transmission power levels.", TxLevels);
  cmd.AddValue("EnergyDet", "Energy detection threshold dBm.", EnergyDet);
  cmd.AddValue("CCAMode1", "CCA mode 1 threshold dBm.", CCAMode1);

  cmd.Parse(argc, argv);

  Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
  Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
  Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue(PLref));
  Config::SetDefault("ns3::LogDistancePropagationLossModel::Exponent", DoubleValue(PLexp));
  Config::SetDefault("ns3::YansWifiPhy::TxGain", DoubleValue(0.0));
  Config::SetDefault("ns3::YansWifiPhy::RxGain", DoubleValue(0.0));
  Config::SetDefault("ns3::YansWifiPhy::TxPowerStart", DoubleValue(TxStart));
  Config::SetDefault("ns3::YansWifiPhy::TxPowerEnd", DoubleValue(TxEnd));
  Config::SetDefault("ns3::YansWifiPhy::TxPowerLevels", UintegerValue(TxLevels));
  Config::SetDefault("ns3::YansWifiPhy::EnergyDetectionThreshold", DoubleValue(EnergyDet));
  Config::SetDefault("ns3::YansWifiPhy::CcaMode1Threshold", DoubleValue(CCAMode1));

  SeedManager::SetSeed(seed);
  SeedManager::SetRun(run);

  Gnuplot gnuplot = Gnuplot("IgmpxTest.png");
  gnuplot.SetLegend("Time [sec]","IgmpxTxControl [kbps]");
  gnuplot.SetExtra("set yrange [0:1.00]");

  Experiment experiment;
  uint32_t clientz[] = {1, 100};
  uint32_t clen = 2;
  for (uint32_t m = 0 ; m<=1 ; m++)
  {
      for (uint32_t c = 0; c < clen; c++)
        {
          NS_LOG_INFO("Installing WiFi.");
          WifiHelper wifi = WifiHelper::Default();
          wifi.SetStandard(WIFI_PHY_STANDARD_80211g);
          wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                        "DataMode", StringValue ("ErpOfdmRate54Mbps"),
                                        "NonUnicastMode", StringValue ("ErpOfdmRate54Mbps")
                                        );
          YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();

          YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
          phy.SetErrorRateModel("ns3::NistErrorRateModel");
          phy.SetChannel (wifiChannel.Create ());

          NqosWifiMacHelper mac = NqosWifiMacHelper::Default();
          mac.SetType("ns3::AdhocWifiMac");

          sizeClient = clientz[c];
          mobility = m;

          std::stringstream sr;
          sr << "N" << sizeClient << "R" << sizeRouter << "M" << mobility;
          NS_LOG_DEBUG ("Running experiment " << sr.str());

          experiment = Experiment(sr.str());
          gnuplot.AddDataset(experiment.Run(sizeRouter, sizeClient, wifi, phy, mac, wifiChannel, range, totalTime, mobility));
        }
    }

  gnuplot.GenerateOutput(std::cout);

  return 0;
}
