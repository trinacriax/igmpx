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

#include <ns3/aodv-helper.h>
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

// gnuplot file
Gnuplot2dDataset g_controlTx("IgmpxTxControl");
Gnuplot2dDataset g_controlRx("IgmpxRxControl");
uint64_t txSlot = 0;
uint64_t rxSlot = 0;
double tx = 0.0;
double rx = 0.0;

#ifdef IGMPTEST
#undef IGMPTEST
#define IGMPTEST 1
#endif

struct mycontext
{
    uint32_t id;
    std::string callback;
};

struct mycontext
GetContextInfo (std::string context)
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
GenericPacketTrace (std::string context, Ptr<const Packet> p)
{
  struct mycontext mc = GetContextInfo(context);
  uint64_t tslot = Simulator::Now().GetSeconds();
//  NS_LOG_INFO("Node " << mc.id << " had " << mc.callback << " PacketID=" << p->GetUid() << " PacketSize=" << p->GetSize()<< " Slot "<<tslot);
  if (mc.callback.compare("IgmpxTxControl") == 0)
    {
      while (txSlot < tslot)
        {
          g_controlTx.Add(txSlot++, tx / 1000);
          tx = 0.0;
        }
      tx += p->GetSize();
    }
  else if (mc.callback.compare("IgmpxRxControl") == 0)
    {
      while (rxSlot < tslot)
        {
          g_controlRx.Add(rxSlot++, rx / 1000);
          rx = 0.0;
        }
      rx += p->GetSize();
    }
}

int
main (int argc, char *argv[])
{
  LogComponentEnable("IgmpxTest", LogLevel(LOG_LEVEL_ALL| LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
  LogComponentEnable("IGMPXRoutingProtocol",
      LogLevel(LOG_LEVEL_INFO | LOG_LEVEL_DEBUG | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("AodvRoutingProtocol",
//      LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("OlsrRoutingProtocol",
//      LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("UdpL4Protocol", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("Ipv4ListRouting", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("UdpSocketImpl", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("Ipv4L3Protocol", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("Ipv4RawSocketImpl",
//      LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("Ipv4EndPointDemux",
//      LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("Socket", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("Ipv4Interface", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("MacLow", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("MacRxMiddle", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("YansWifiPhy", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("InterferenceHelper",
//      LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("ArpL3Protocol", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("ArpCache", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("YansWifiChannel", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("Node", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("Packet", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
//  LogComponentEnable("DefaultSimulatorImpl",
//      LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));

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
  //Step in the grid X
  double range = 30;
  // Simulation time, seconds
  double totalTime = 100;
  // grid columns
  uint16_t cols;
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
  cmd.AddValue("cols", "Grid width ", cols);
  cmd.AddValue("time", "Simulation time, s.", totalTime);
  cmd.AddValue("PLref", "Reference path loss dB.", PLref);
  cmd.AddValue("PLexp", "Path loss exponent.", PLexp);
  cmd.AddValue("TxStart", "Transmission power start dBm.", TxStart);
  cmd.AddValue("TxEnd", "Transmission power end dBm.", TxEnd);
  cmd.AddValue("TxLevels", "Transmission power levels.", TxLevels);
  cmd.AddValue("EnergyDet", "Energy detection threshold dBm.", EnergyDet);
  cmd.AddValue("CCAMode1", "CCA mode 1 threshold dBm.", CCAMode1);

  cmd.Parse(argc, argv);
  cols = (uint16_t) ceil(sqrt(sizeRouter));
  cols = (cols == 0 ? 1 : cols);

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
  NS_LOG_INFO ("Create nodes Router "<<sizeRouter<< " Clients "<<sizeClient<<" Source 1 ");

  NodeContainer sources;
  sources.Create(1);

  NodeContainer routers;
  routers.Create(sizeRouter);

  NodeContainer clients;
  clients.Create(sizeClient);

  NodeContainer allNodes;
  allNodes.Add(sources);
  allNodes.Add(routers);
  allNodes.Add(clients);

  Gnuplot gnuplot = Gnuplot("IgmpControlMessages.png");
  gnuplot.SetLegend("Time [sec]", "ControlRate [kbps]");

  g_controlTx.SetStyle(Gnuplot2dDataset::LINES);
  g_controlRx.SetStyle(Gnuplot2dDataset::LINES);

  gnuplot.AddDataset(g_controlTx);
  gnuplot.AddDataset(g_controlRx);

  for (uint32_t i = 0; i < allNodes.GetN(); ++i)
    { // Name nodes
      std::ostringstream os;
      os << "node-" << i;
      Names::Add(os.str(), allNodes.Get(i));
    }

  NS_LOG_INFO("Installing WiFi.");
  WifiHelper wifi = WifiHelper::Default();
  wifi.SetStandard(WIFI_PHY_STANDARD_80211g);
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
  phy.SetChannel(wifiChannel.Create());
  Ptr<ErrorRateModel> error = CreateObject<YansErrorRateModel>();
  phy.SetErrorRateModel("ns3::NistErrorRateModel");
  NqosWifiMacHelper mac = NqosWifiMacHelper::Default();
  mac.SetType("ns3::AdhocWifiMac");
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("ErpOfdmRate54Mbps"),
      "ControlMode", StringValue("ErpOfdmRate54Mbps"), "NonUnicastMode", StringValue("ErpOfdmRate54Mbps"));

  NS_LOG_INFO("Installing CSMA link source-router.");
  CsmaHelper csma; //Wired
  csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(10000000)));
  csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

  NodeContainer s0r0;
  s0r0.Add(sources.Get(0));
  s0r0.Add(routers.Get(0));
  NetDeviceContainer ds0dr0 = csma.Install(s0r0);

  NetDeviceContainer routersNetDev = wifi.Install(phy, mac, routers);
  NetDeviceContainer clientsNetDev = wifi.Install(phy, mac, clients);

  NS_LOG_INFO("Installing Internet Stack.");
  AodvHelper aodvStack;
  IgmpxHelper igmpxStack;
  NS_LOG_INFO ("Enabling Routing.");

  /* ROUTERS */
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper listRouters;
  listRouters.Add(staticRouting, 0);
  listRouters.Add(igmpxStack, 1);
  listRouters.Add(aodvStack, 10);

  InternetStackHelper internetRouters;
  internetRouters.SetRoutingHelper(listRouters);
  internetRouters.Install(routers);

  Ipv4ListRoutingHelper listSource;
  listSource.Add(staticRouting, 11);

  InternetStackHelper internetSource;
  internetSource.SetRoutingHelper(listSource);
  internetSource.Install(sources);

  /* CLIENTS */
  Ipv4ListRoutingHelper listClients;
  listClients.Add(staticRouting, 0);
  listClients.Add(igmpxStack, 1);

  InternetStackHelper internetClients;
  internetClients.SetRoutingHelper(listClients);
  internetClients.Install(clients);

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  Ipv4Address base = Ipv4Address("10.1.2.0");
  Ipv4Mask mask = Ipv4Mask("255.255.255.0");
  ipv4.SetBase(base, mask);
  Ipv4InterfaceContainer ipRouter = ipv4.Assign(routersNetDev);
  Ipv4InterfaceContainer ipClient = ipv4.Assign(clientsNetDev);

  base = Ipv4Address("10.1.1.0");
  ipv4.SetBase(base, mask);
  Ipv4InterfaceContainer ipSource = ipv4.Assign(ds0dr0);

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
      command << "NodeList/" << routers.Get(n)->GetId() << "/$ns3::igmpx::IGMPXRoutingProtocol/PeerRole";
      Config::Set(command.str(), EnumValue(igmpx::ROUTER));
    }
  // CLIENTS
  ss.str("");
  ss << multicastSource << "," << multicastGroup << "," << "1";
  for (uint32_t n = 0; n < clients.GetN(); n++)
    { //Clients are RN nodes
      std::stringstream command;
      command << "/NodeList/" << clients.Get(n)->GetId() << "/$ns3::igmpx::IGMPXRoutingProtocol/PeerRole";
      Config::Set(command.str(), EnumValue(igmpx::CLIENT));
      command.str("");
      command << "/NodeList/" << clients.Get(n)->GetId() << "/$ns3::igmpx::IGMPXRoutingProtocol/RegisterAsMember";
      Config::Set(command.str(), StringValue(ss.str()));
    }

  Config::Connect("/NodeList/*/$ns3::igmpx::IGMPXRoutingProtocol/IgmpxTxControl", MakeCallback(&GenericPacketTrace));
//  Config::Connect("/NodeList/*/$ns3::igmpx::IGMPXRoutingProtocol/IgmpxRxControl", MakeCallback(&GenericPacketTrace));

  NS_LOG_INFO("Installing Position and Mobility.");
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

  double rangino = range * 0.5;
  double deltaXmin, deltaXmax, deltaYmin, deltaYmax;
  deltaYmin = deltaXmin = 0 - rangino;
  deltaXmax = floor(range * (cols - 1)) + rangino;
  int rows = (int) floor(sizeRouter / cols);
  deltaYmax = range * (rows - 1) + rangino;

  NS_LOG_DEBUG ("Arranging clients between ["<<deltaXmin<<","<< deltaYmin<<"] - [" <<deltaXmax<<","<<deltaYmax<<"]");
  Ptr<UniformRandomVariable> X = CreateObject<UniformRandomVariable>();
  X->SetAttribute("Min", DoubleValue(deltaXmin));
  X->SetAttribute("Max", DoubleValue(deltaXmax));
  Ptr<UniformRandomVariable> Y = CreateObject<UniformRandomVariable>();
  Y->SetAttribute("Min", DoubleValue(deltaYmin));
  Y->SetAttribute("Max", DoubleValue(deltaYmax));
  Ptr<ConstantRandomVariable> Z = CreateObject<ConstantRandomVariable>();
  Z->SetAttribute("Constant", DoubleValue(0));

  if (mobility == 0) // Fixed Nodes.
    {
      Ptr<ListPositionAllocator> positionAllocC = CreateObject<ListPositionAllocator>();
      positionAllocC->Add(Vector(rPos[0].x, rPos[0].y + 10, rPos[0].z));
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
      for (uint32_t c = 0; c < clients.GetN(); c++)
        {
          groups[c % sizeClient].Add(clients.Get(c));
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

  Ptr<ListPositionAllocator> positionAllocS = CreateObject<ListPositionAllocator>();
  positionAllocS->Add(Vector(-10.0, 0.0, 0.0)); // Source
  MobilityHelper mobilityS;
  mobilityS.SetPositionAllocator(positionAllocS);
  mobilityS.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityS.Install(sources);

  for (uint32_t i = 0; i < clients.GetN(); i++)
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
  while (txSlot < tslot)
    {
      g_controlTx.Add(txSlot++, tx / 1000);
      tx = 0.0;
    }
  while (rxSlot < tslot)
    {
      g_controlRx.Add(rxSlot++, rx / 1000);
      rx = 0.0;
    }
  g_controlTx.Add(txSlot, tx / 1000);
  g_controlRx.Add(rxSlot, rx / 1000);
  gnuplot.GenerateOutput(std::cout);

  Simulator::Destroy();
  return 0;
}
