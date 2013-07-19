/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
*/

/*
//
// Network topology
//
//           7
//           |
//           0
//          / \
//         /   \
//  4 --- 1     3 --- 6
//         \   /
//          \ /
//           2
//           |
//           5
*/

#ifndef NS3_LOG_ENABLE
	#define NS3_LOG_ENABLE
#endif


#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <sstream>

#include "ns3/aodv-helper.h"
#include "ns3/pimdm-helper.h"
#include "ns3/igmpx-helper.h"

#include "ns3/video-helper.h"
#include "ns3/video-push-module.h"

#include "ns3/string.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/tools-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/yans-error-rate-model.h"
#include "ns3/csma-helper.h"

#include <iostream>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("IgmpxTestPim");

static bool g_verbose = false;
//static ofstream controls;


struct mycontext{
	uint32_t id;
	std::string callback;
};

struct mycontext GetContextInfo (std::string context){
	struct mycontext mcontext;
	int p2 = context.find("/");
	int p1 = context.find("/",p2+1);
	p2 = context.find("/",p1+1);
	mcontext.id = atoi(context.substr(p1+1, (p2-p1)-1).c_str());
//	std::cout<<"P1 = "<<p1<< " P2 = "<< p2 << " ID: " <<mcontext.id;
	p1 = context.find_last_of("/");
	p2 = context.length() - p2;
	mcontext.callback = context.substr(p1+1, p2).c_str();
//	std::cout<<"; P1 = "<<p1<< " P2 = "<< p2<< " CALL: "<< mcontext.callback<<"\n";
	return mcontext;
}

static void GenericPacketTrace (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
//	controls
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " " << p->GetSize()  <<  std::endl;
}
/*
static void AppTx (Ptr<const Packet> p)
{
std::cout << Simulator::Now() <<" Sending Packet "<< p->GetSize() << " bytes " << std::endl;
}
*/
static void PhyTxDrop (Ptr<const Packet> p)
{
std::cout << Simulator::Now() <<" Phy Drop Packet "<< p->GetSize() << " bytes " << std::endl;
}

static void TableChanged (std::string context, uint32_t size)
{
	struct mycontext mc = GetContextInfo (context);
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<< size  << " entries " << std::endl;
}

void
DevTxTrace (std::string context, Ptr<const Packet> p)
{
 if (g_verbose)
   {
		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<< p->GetUid() << std::endl;
   }
}
void
DevRxTrace (std::string context, Ptr<const Packet> p)
{
 if (g_verbose)
   {
		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " " << p->GetUid() << std::endl;
   }
}
void
PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double
snr, WifiMode mode, enum WifiPreamble preamble)
{
 if (g_verbose)
   {

		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " PHYRXOK mode=" << mode << " snr=" << snr << " " <<
*packet << std::endl;
   }
}
void
PhyRxErrorTrace (std::string context, Ptr<const Packet> packet, double
snr)
{
 if (g_verbose)
   {
		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " PHYRXERROR snr=" << snr << " " << *packet <<
std::endl;
   }
}
void
PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode
mode, WifiPreamble preamble, uint8_t txPower)
{
 if (g_verbose)
   {
		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " PHYTX mode=" << mode << " " << *packet <<
std::endl;
   }
}

void
PhyStateTrace (std::string context, Time start, Time duration, enum
WifiPhy::State state)
{
 if (g_verbose)
   {
		struct mycontext mc = GetContextInfo (context);
		std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<<  " state=" << state << " start=" << start << " duration=" << duration << std::endl;
   }
}

static void ArpDiscard (std::string context, Ptr<const Packet> p)
{
	struct mycontext mc = GetContextInfo (context);
	std::cout << Simulator::Now() << " Node "<< mc.id << " "<< mc.callback << " "<< " Arp discards packet "<< p->GetUid() << " of "<<p->GetSize() << " bytes " << std::endl;
}

int
main (int argc, char *argv[])
 	{
// Users may find it convenient to turn on explicit debugging
// for selected modules; the below lines suggest how to do this
#if 1
//	LogComponentEnable ("SimpleGlobalRoutingExample", LOG_LEVEL_INFO);
	LogComponentEnable ("IgmpxTestPim", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("VideoPushApplication", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("ChunkBuffer", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("PacketSink", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("PIMDMMulticastRouting", LogLevel(LOG_LEVEL_ALL| LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
	LogComponentEnable ("IGMPXRoutingProtocol", LogLevel(LOG_LEVEL_ALL| LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("AodvRoutingProtocol", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("OlsrRoutingProtocol", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("MbnAodvRoutingProtocol", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("MbnAodvNeighbors", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("MbnRoutingTable", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("UdpL4Protocol", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("Ipv4ListRouting", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("UdpSocketImpl", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("Ipv4L3Protocol", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("Ipv4RawSocketImpl", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("Ipv4EndPointDemux", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("Socket", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("Ipv4Interface", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("MacLow", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("MacRxMiddle", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("YansWifiPhy", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("InterferenceHelper", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("ArpL3Protocol", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("ArpCache", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("YansWifiChannel", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("Node", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("Packet", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
//	LogComponentEnable ("DefaultSimulatorImpl", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC));
#endif
	/// Number of router nodes
	uint32_t sizeRouter = 12;
	/// Number of client nodes
	uint32_t sizeClient = 4;
	/// Number of source nodes
	uint32_t sizeSource = 1;
	//Routing protocol 1) OLSR, 2) AODV, 3) MBN-AODV
//	int32_t routing = 2;
	//Seed for random numbers
	uint32_t seed = 4001254589;
	//Seed Run -> 10 runs!
	uint32_t run = 1;
	//Source streaming rate
	uint64_t stream = 1000000;
	//Step in the grid X
	double range = 15;
	/// Simulation time, seconds
	double totalTime = 100;
	/// grid cols number
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
	double EnergyDet= -95.0;
	// CCA mode 1
	double CCAMode1 = -62.0;

	CommandLine cmd;
	cmd.AddValue ("seed", "Seed Random.", seed);
	cmd.AddValue ("run", "Seed Run.", run);
	cmd.AddValue ("sizeRouter", "Number of router nodes.", sizeRouter);
	cmd.AddValue ("sizeClient", "Number of clients.", sizeClient);
	cmd.AddValue ("sizeSource", "Number of sources.", sizeSource);
//	cmd.AddValue ("routing", "Routing protocol used.", routing);
	cmd.AddValue ("range", "Cover range in meters.", range);
	cmd.AddValue ("cols", "Grid width ", cols);
	cmd.AddValue ("time", "Simulation time, s.", totalTime);
	cmd.AddValue ("PLref", "Reference path loss dB.", PLref);
	cmd.AddValue ("PLexp", "Path loss exponent.", PLexp);
	cmd.AddValue ("TxStart", "Transmission power start dBm.", TxStart);
	cmd.AddValue ("TxEnd", "Transmission power end dBm.", TxEnd);
	cmd.AddValue ("TxLevels", "Transmission power levels.", TxLevels);
	cmd.AddValue ("EnergyDet", "Energy detection threshold dBm.", EnergyDet);
	cmd.AddValue ("CCAMode1", "CCA mode 1 threshold dBm.", CCAMode1);
	cmd.AddValue ("stream", "Source streaming rate in bps", stream);

	cmd.Parse(argc, argv);

	cols = (uint16_t)ceil(sqrt(sizeRouter));
	cols = (cols==0?1:cols);

	NS_LOG_DEBUG("Seed " << seed << " run "<< run << " sizeRouter "<< sizeRouter << " sizeClient " << sizeClient <<
			" sizeSource " << sizeSource <<// " routing " << routing <<
			" range " << range << " cols " << cols <<
			" time " << totalTime << " PLref " << PLref << " PLexp " << PLexp << " TxStart " << TxStart <<
			" TxEnd " << TxEnd << " TxLevels " << TxLevels << " EnergyDet " << EnergyDet << " CCAMode1 "<< CCAMode1 <<
			" Stream "<< stream << "\n");

	Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
	Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
	Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue(PLref));
	Config::SetDefault("ns3::LogDistancePropagationLossModel::Exponent", DoubleValue(PLexp));
//	Config::SetDefault("ns3::NakagamiPropagationLossModel::Distance1", DoubleValue(30));
//	Config::SetDefault("ns3::NakagamiPropagationLossModel::Distance2", DoubleValue(50));
//	Config::SetDefault("ns3::NakagamiPropagationLossModel::m0", DoubleValue(1));
//	Config::SetDefault("ns3::NakagamiPropagationLossModel::m1", DoubleValue(1));
//	Config::SetDefault("ns3::NakagamiPropagationLossModel::m2", DoubleValue(.75));
	Config::SetDefault("ns3::YansWifiPhy::TxGain",DoubleValue(0.0));
	Config::SetDefault("ns3::YansWifiPhy::RxGain",DoubleValue(0.0));
	Config::SetDefault("ns3::YansWifiPhy::TxPowerStart",DoubleValue(TxStart));
	Config::SetDefault("ns3::YansWifiPhy::TxPowerEnd",DoubleValue(TxEnd));
	Config::SetDefault("ns3::YansWifiPhy::TxPowerLevels",UintegerValue(TxLevels));
	Config::SetDefault("ns3::YansWifiPhy::EnergyDetectionThreshold",DoubleValue(EnergyDet));///17.3.10.1 Receiver minimum input sensitivity
	Config::SetDefault("ns3::YansWifiPhy::CcaMode1Threshold",DoubleValue(CCAMode1));///17.3.10.5 CCA sensitivity
	Config::SetDefault("ns3::MatrixPropagationLossModel::DefaultLoss", DoubleValue(0));

	// Here, we will explicitly create four nodes.  In more sophisticated
	// topologies, we could configure a node factory.
	double sourceStart = ceil(totalTime*.25);//after 25% of simulation
	/// Video stop
	double sourceStop = ceil(totalTime*.90);//after 90% of simulation
	/// Client start
	double clientStart = ceil(totalTime*.23);;
	/// Client stop
	double clientStop = ceil(totalTime*.99);

//	controls.open ("controls.txt", ios::out | ios::app);

	SeedManager::SetSeed(seed);
	SeedManager::SetRun(run);
	NS_LOG_INFO ("Create nodes Router "<<sizeRouter<< " Clients "<<sizeClient<<" Source " << sizeSource<<" ");

	NodeContainer source;
	source.Create(sizeSource);

	NodeContainer routers;
	routers.Create (sizeRouter);

	NodeContainer clients;
	clients.Create (sizeClient);

	NodeContainer allNodes;
	allNodes.Add(source);
	allNodes.Add(routers);
	allNodes.Add(clients);

	for (uint32_t i = 0; i < allNodes.GetN(); ++i) { // Name nodes
		std::ostringstream os;
		os << "node-" << i;
		Names::Add(os.str(), allNodes.Get(i));
	}
	NS_LOG_INFO("Build Topology.");
	/* WIFI STANDARD */

	WifiHelper wifi = WifiHelper::Default ();
	wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
//	wifi.EnableLogComponents();
	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
//	wifiChannel.AddPropagationLoss("ns3::MatrixPropagationLossModel");

	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.SetChannel (wifiChannel.Create());
	Ptr<ErrorRateModel> error = CreateObject<YansErrorRateModel> ();
	phy.SetErrorRateModel("ns3::NistErrorRateModel");

	 // Add a non-QoS upper mac, and disable rate control
	NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

	// Set it to adhoc mode
	mac.SetType ("ns3::AdhocWifiMac");

	//802.11g
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
		"DataMode", StringValue ("ErpOfdmRate54Mbps")
		,"ControlMode", StringValue ("ErpOfdmRate54Mbps")
		,"NonUnicastMode", StringValue ("ErpOfdmRate54Mbps")
		);


	/* Source Node to Gateway */
	CsmaHelper csma; //Wired
	csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (10000000)));
	csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

	NetDeviceContainer routersNetDev = wifi.Install(phy, mac, routers);
	NetDeviceContainer clientsNetDev = wifi.Install(phy, mac, clients);

	NodeContainer s0r0;
	s0r0.Add(source.Get(0));
	s0r0.Add(routers.Get(0));
	NetDeviceContainer ds0dr0 = csma.Install (s0r0);

	// INSTALL INTERNET STACK
	AodvHelper aodvStack;
	PimDmHelper pimdmStack;
	IgmpxHelper igmpxStack;
	NS_LOG_INFO ("Enabling Routing.");

	/* ROUTERS */
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper listRouters;
	listRouters.Add (staticRouting, 0);
	listRouters.Add (igmpxStack, 1);
	listRouters.Add (aodvStack, 10);
	listRouters.Add (pimdmStack, 11);

	InternetStackHelper internetRouters;
	internetRouters.SetRoutingHelper (listRouters);
	internetRouters.Install (routers);

	/* CLIENTS */
	Ipv4ListRoutingHelper listClients;
	listClients.Add (staticRouting, 0);
	listClients.Add (igmpxStack, 1);

	InternetStackHelper internetClients;
	internetClients.SetRoutingHelper (listClients);
	internetClients.Install (clients);

	Ipv4ListRoutingHelper listSource;
	listSource.Add (aodvStack, 10);
	listSource.Add (staticRouting, 11);

	InternetStackHelper internetSource;
	internetSource.SetRoutingHelper (listSource);
	internetSource.Install (source);

	// Later, we add IP addresses.
	NS_LOG_INFO ("Assign IP Addresses.");
	Ipv4AddressHelper ipv4;
	Ipv4Address base = Ipv4Address("10.1.2.0");
	Ipv4Mask mask = Ipv4Mask("255.255.255.0");
	ipv4.SetBase (base, mask);
	Ipv4InterfaceContainer ipRouter = ipv4.Assign (routersNetDev);
	Ipv4InterfaceContainer ipClient = ipv4.Assign (clientsNetDev);

	base = Ipv4Address("10.1.1.0");
	ipv4.SetBase (base, mask);
	Ipv4InterfaceContainer ipSource = ipv4.Assign (ds0dr0);
	Ipv4Address multicastSource = ipSource.GetAddress(0);

  	NS_LOG_INFO ("Configure multicasting.");

	Ipv4Address multicastGroup ("225.1.2.4");

	/* 1) Configure a (static) multicast route on ASNGW (multicastRouter) */
	Ptr<Node> multicastRouter = routers.Get (0); // The node in question
	Ptr<NetDevice> inputIf = routersNetDev.Get (0); // The input NetDevice

	Ipv4StaticRoutingHelper multicast;
	multicast.AddMulticastRoute (multicastRouter, multicastSource, multicastGroup, inputIf, routersNetDev.Get(0));

	/* 2) Set up a default multicast route on the sender n0 */
	Ptr<Node> sender = source.Get (0);
//	Ptr<NetDevice> senderIf = sourceNetDev.Get (0);
	Ptr<NetDevice> senderIf = ds0dr0.Get (0);
	multicast.SetDefaultMulticastRoute (sender, senderIf);

	std::stringstream ss;
	/* source, group, interface*/
	//ROUTERS
	ss<< multicastSource<< "," << multicastGroup;
	for (uint32_t n = 0;  n < routers.GetN() ; n++){
		std::stringstream command;//create a stringstream
		command<< "NodeList/" << routers.Get(n)->GetId() << "/$ns3::pimdm::MulticastRoutingProtocol/RegisterSG";
		Config::Set(command.str(), StringValue(ss.str()));
		command.str("");
		command<< "NodeList/" << routers.Get(n)->GetId() << "/$ns3::igmpx::IGMPXRoutingProtocol/PeerRole";
		Config::Set(command.str(), EnumValue(igmpx::ROUTER));
	}
	// CLIENTS
	ss.str("");
	ss<< multicastSource<< "," << multicastGroup << "," << "1";
	for (uint32_t n = 0;  n < clients.GetN() ; n++){//Clients are RN nodes
		std::stringstream command;
		command<< "/NodeList/" << clients.Get(n)->GetId()<<"/$ns3::igmpx::IGMPXRoutingProtocol/PeerRole";
		Config::Set(command.str(), EnumValue(igmpx::CLIENT));
		command.str("");
		command<< "/NodeList/" << clients.Get(n)->GetId()<<"/$ns3::igmpx::IGMPXRoutingProtocol/RegisterAsMember";
		Config::Set(command.str(), StringValue(ss.str()));
	}

	if(g_verbose){
		Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/PhyTxDrop",MakeCallback (&PhyTxDrop));
		Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacTx", MakeCallback (&DevTxTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacRx",	MakeCallback (&DevRxTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxOk",	MakeCallback (&PhyRxOkTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxError",	MakeCallback (&PhyRxErrorTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/Tx",	MakeCallback (&PhyTxTrace));
		Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/State", MakeCallback (&PhyStateTrace));
		Config::Connect ("/NodeList/*/$ns3::ArpL3Protocol/Drop", MakeCallback (&ArpDiscard));
		Config::Connect ("/NodeList/*/$ns3::olsr::RoutingProtocol/RoutingTableChanged",MakeCallback (&TableChanged));
	}
	Config::Connect ("/NodeList/*/$ns3::igmpx::IGMPXRoutingProtocol/RxIgmpxControl",MakeCallback (&GenericPacketTrace));
	Config::Connect ("/NodeList/*/$ns3::igmpx::IGMPXRoutingProtocol/TxIgmpxControl",MakeCallback (&GenericPacketTrace));
	Config::Connect ("/NodeList/*/$ns3::pimdm::MulticastRoutingProtocol/TxPimData",MakeCallback (&GenericPacketTrace));
	Config::Connect ("/NodeList/*/$ns3::pimdm::MulticastRoutingProtocol/RxPimData",MakeCallback (&GenericPacketTrace));
	Config::Connect ("/NodeList/*/$ns3::pimdm::MulticastRoutingProtocol/TxPimControl", MakeCallback (&GenericPacketTrace));
	Config::Connect ("/NodeList/*/$ns3::pimdm::MulticastRoutingProtocol/RxPimControl", MakeCallback (&GenericPacketTrace));
	Config::Connect ("/NodeList/*/$ns3::ArpL3Protocol/Drop", MakeCallback (&GenericPacketTrace));

	NS_LOG_INFO ("Create Source");
	InetSocketAddress dst = InetSocketAddress (multicastGroup, PUSH_PORT);
	Config::SetDefault ("ns3::UdpSocket::IpMulticastTtl", UintegerValue (1));
	VideoHelper video = VideoHelper ("ns3::UdpSocketFactory", dst);
	video.SetAttribute ("DataRate", DataRateValue (DataRate (stream)));
	video.SetAttribute ("PacketSize", UintegerValue (1200));
	video.SetAttribute ("PeerType", EnumValue (SOURCE));
	video.SetAttribute ("Local", AddressValue (ipSource.GetAddress(0)));
	video.SetAttribute ("PeerPolicy", EnumValue (PS_RANDOM));
	video.SetAttribute ("ChunkPolicy", EnumValue (CS_LATEST));


	ApplicationContainer apps = video.Install (source.Get (0));
	apps.Start (Seconds (sourceStart));
	apps.Stop (Seconds (sourceStop));

	for(uint32_t n = 0; n < clients.GetN() ; n++){
		InetSocketAddress dstC = InetSocketAddress (multicastGroup, PUSH_PORT);
		Config::SetDefault ("ns3::UdpSocket::IpMulticastTtl", UintegerValue (1));
		VideoHelper videoC = VideoHelper ("ns3::UdpSocketFactory", dstC);
		videoC.SetAttribute ("PeerType", EnumValue (PEER));
		videoC.SetAttribute ("LocalPort", UintegerValue (PUSH_PORT));
		videoC.SetAttribute ("Local", AddressValue(ipClient.GetAddress(n)));
		videoC.SetAttribute ("PeerPolicy", EnumValue (PS_RANDOM));
		videoC.SetAttribute ("ChunkPolicy", EnumValue (CS_LATEST));

		ApplicationContainer appC = videoC.Install (clients.Get(n));
		appC.Start (Seconds (clientStart));
		appC.Stop (Seconds (clientStop));
	}

	double rangino = range*0.5;
	double deltaXmin, deltaXmax, deltaYmin, deltaYmax;
	deltaYmin = deltaXmin = 0-rangino;
	deltaXmax = floor (range * (cols-1)) + rangino;
	int rows = (int)floor (sizeRouter / cols);
	deltaYmax = range * (rows - 1) + rangino;

	NS_LOG_INFO ("Arranging clients between ["<<deltaXmin<<","<< deltaYmin<<"] - [" <<deltaXmax<<","<<deltaYmax<<"]");

	Ptr<ListPositionAllocator> positionAlloc[4] = CreateObject<ListPositionAllocator> ();
	positionAlloc[0]->Add(Vector(120.0, 0.0, 0.0));// 1
	positionAlloc[1]->Add(Vector(190.0, 80.0, 0.0));// 2
	positionAlloc[2]->Add(Vector(120.0, 160.0, 0.0));// 3
	positionAlloc[3]->Add(Vector(50.0, 80.0, 0.0));// 4

	MobilityHelper mobility[4];
	Vector velocity[4];
	velocity[0] = Vector(0.0, 1.0, 0.0);
	velocity[1] = Vector(-1.0, 0.0, 0.0);
	velocity[2] = Vector(0.0, -1.0, 0.0);
	velocity[3] = Vector(1.0, 0.0, 0.0);

	mobility[0].SetPositionAllocator(positionAlloc[0]);
	mobility[0].SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
	mobility[1].SetPositionAllocator(positionAlloc[1]);
	mobility[1].SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
	mobility[2].SetPositionAllocator(positionAlloc[2]);
	mobility[2].SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
	mobility[3].SetPositionAllocator(positionAlloc[3]);
	mobility[3].SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
//	mobility[0].SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X",RandomVariableValue(UniformVariable(100,130)),
//		"Y",RandomVariableValue(UniformVariable(0,-20)), "Z",RandomVariableValue(ConstantVariable(0)));
//	mobility[1].SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X",RandomVariableValue(UniformVariable(170,200)),
//			"Y",RandomVariableValue(UniformVariable(70,100)), "Z",RandomVariableValue(ConstantVariable(0)));
//	mobility[2].SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X",RandomVariableValue(UniformVariable(100,130)),
//			"Y",RandomVariableValue(UniformVariable(140,200)), "Z",RandomVariableValue(ConstantVariable(0)));
//	mobility[3].SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X",RandomVariableValue(UniformVariable(60,0)),
//			"Y",RandomVariableValue(UniformVariable(70,100)), "Z",RandomVariableValue(ConstantVariable(0)));
//
//
//	mobility[0].SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
//			"Bounds", StringValue ("100|130|-20|220"), "Speed", StringValue ("Constant:1.2527"),//4.510km/h
//			"Pause", StringValue ("Constant:1") //, "Time", StringValue ("10s")
//            );
//	mobility[0].SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X",RandomVariableValue(UniformVariable(100,130)),
//		"Y",RandomVariableValue(UniformVariable(0,-20)), "Z",RandomVariableValue(ConstantVariable(0)));
//
//	mobility[1].SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
//			"Bounds", StringValue ("100|130|-20|220"), "Speed", StringValue ("Constant:1.2527"),//4.510km/h
//			"Pause", StringValue ("Constant:1") //, "Time", StringValue ("10s")
//            );
//	mobility[1].SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X",RandomVariableValue(UniformVariable(100,130)),
//		"Y",RandomVariableValue(UniformVariable(0,-20)), "Z",RandomVariableValue(ConstantVariable(0)));
//
//
//	mobility[2].SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
//			"Bounds", StringValue ("100|130|-20|220"), "Speed", StringValue ("Constant:1.2527"),//4.510km/h
//			"Pause", StringValue ("Constant:1") //, "Time", StringValue ("10s")
//            );
//	mobility[2].SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X",RandomVariableValue(UniformVariable(100,130)),
//		"Y",RandomVariableValue(UniformVariable(0,-20)), "Z",RandomVariableValue(ConstantVariable(0)));
//
//	mobility[3].SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
//			"Bounds", StringValue ("100|130|-20|220"), "Speed", StringValue ("Constant:1.2527"),//4.510km/h
//			"Pause", StringValue ("Constant:1") //, "Time", StringValue ("10s")
//            );
//	mobility[3].SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X",RandomVariableValue(UniformVariable(100,130)),
//		"Y",RandomVariableValue(UniformVariable(0,-20)), "Z",RandomVariableValue(ConstantVariable(0)));

	NodeContainer clientGroup[4];
	for (uint32_t c = 0; c < clients.GetN();c++){
		clientGroup[c%4].Add(clients.Get(c));
	}
	for (int c = 0; c < 4;c++){
		mobility[c].Install(clientGroup[c]);
	}

	for (uint32_t c = 0; c < clients.GetN();c++){
		Ptr<ConstantVelocityMobilityModel> cvm = clients.Get(c)->GetObject<ConstantVelocityMobilityModel> ();
		cvm->SetVelocity(velocity[c%4]);
	}
//	mobilityC1.Install(clients);

	Ptr<ListPositionAllocator> positionAllocS = CreateObject<ListPositionAllocator> ();
	positionAllocS->Add(Vector(90.0, 60.0, 0.0));// Source
	MobilityHelper mobilityS;
	mobilityS.SetPositionAllocator(positionAllocS);
	mobilityS.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityS.Install(source);

	/*
        int lossMatrix[12][12] = {
			{0,1,1,1,1,0,0,0,0,0,0,1},
			{1,0,1,1,0,1,1,0,0,0,0,0},
			{1,1,0,1,0,0,0,0,0,1,1,0},
			{1,1,1,0,0,0,0,1,1,0,0,0},//4
			{1,0,0,0,0,1,0,0,0,0,0,0},
			{0,1,0,0,1,0,0,0,0,0,0,0},//6
			{0,1,0,0,0,0,0,1,0,0,0,0},//7
			{0,0,0,1,0,0,1,0,0,0,0,0},//8
			{0,0,0,1,0,0,0,0,1,0,0,0},//9
			{0,0,1,0,0,0,0,0,1,0,0,0},//10
			{0,0,1,0,0,0,0,0,0,0,0,1},//11
			{1,0,0,0,0,0,0,0,0,0,1,0}//12
	};
	*/
	Ptr<ListPositionAllocator> positionAllocR = CreateObject<ListPositionAllocator> ();
	positionAllocR->Add(Vector(100.0, 70.0, 0.0));// 1
	positionAllocR->Add(Vector(130.0, 70.0, 0.0));// 2
	positionAllocR->Add(Vector(100.0, 100.0, 0.0));// 3
	positionAllocR->Add(Vector(130.0, 100.0, 0.0));// 4
	positionAllocR->Add(Vector(100.0, 30.0, 0.0));// 5
	positionAllocR->Add(Vector(130.0, 30.0, 0.0));// 6
	positionAllocR->Add(Vector(170.0, 70.0, 0.0));// 7
	positionAllocR->Add(Vector(170.0, 100.0, 0.0));// 8
	positionAllocR->Add(Vector(130.0, 140.0, 0.0));// 9
	positionAllocR->Add(Vector(100.0, 140.0, 0.0));// 10
	positionAllocR->Add(Vector(60.0, 70.0, 0.0));// 11
	positionAllocR->Add(Vector(60.0, 100.0, 0.0));// 12

	MobilityHelper mobilityR;
	mobilityR.SetPositionAllocator(positionAllocR);
	mobilityR.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobilityR.Install(routers);

	for(uint32_t i = 0; i < allNodes.GetN(); i++){
		  Ptr<MobilityModel> mobility = allNodes.Get(i)->GetObject<MobilityModel> ();
	      Vector pos = mobility->GetPosition (); // Get position
	      NS_LOG_INFO("Position Node ["<<i<<"] = ("<< pos.x << ", " << pos.y<<", "<<pos.z<<")");
	}

//	for (int r = 0 ; r < sizeRouter; r++){
//		Ptr<Node> n = routers.Get(r);
//		Ptr<PropagationLossModel> plossModel = n->GetObject<PropagationLossModel> ();
//		Ptr<MatrixPropagationLossModel> lossModel = DynamicCast<MatrixPropagationLossModel> (plossModel);
//		for(int v = r+1 ; v <sizeRouter; v++){
//		  double value = (lossMatrix[r][v]==0)?300:0;
//		  lossModel->SetLoss(routers.Get(r)->GetObject<MobilityModel>(), routers.Get(v)->GetObject<MobilityModel>(), value, true);
//		}
//	}

	NS_LOG_INFO ("Run Simulation.");
	Simulator::Stop (Seconds (totalTime));
	Simulator::Run ();
	NS_LOG_INFO ("Done.");

	Simulator::Destroy ();
//	controls.close();
	return 0;
}
