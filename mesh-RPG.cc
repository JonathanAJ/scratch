/*
 by Carina T. de Oliveira
 Simulations in Mesh Networks
 December, 2017
 */
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"

#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"

#include <ctime>
#include <time.h>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestMeshScript");

class MeshAulaRPG {
  public:
    /// Init test
    MeshAulaRPG ();
    /// Configure test from command line arguments
    void Configure (int argc, char ** argv);
    /// Run test
    int Run ();
    /// Variance
    void LoopForVariance ();
  private:
    int       m_xSize;
    int       m_ySize;
    int       m_numFlows;
    int       m_seed;
    int       m_standardPhy;
    double    m_step;
    double    m_randomStart;
    double    m_totalTime;
    double    m_packetInterval;
    float     m_timeStartFlowSources;
    uint16_t  m_packetSize;
    uint32_t  m_nIfaces;
    bool      m_chan;
    bool      m_pcap;
    std::ofstream myfile;
    std::string m_stack;
    std::string m_root;
    /// List of network nodes
    NodeContainer nodes;
    /// List of all mesh point devices
    NetDeviceContainer meshDevices;
    //Addresses of interfaces:
    Ipv4InterfaceContainer interfaces;
    // MeshHelper. Report is not static methods
    MeshHelper mesh;
    // seed
    int index_seed = 0;
  private:
    /// Create nodes and setup their mobility
    void CreateNodes ();
    void PrintPositionNodes ();
    /// Install internet m_stack on nodes
    void InstallInternetStack ();
    bool ExistSource (int candidateSource,int sizeVector, int vec[]);
    /// Install applications
    void InstallApplication ();
    /// Print mesh devices diagnostics
    void Report ();
};

MeshAulaRPG::MeshAulaRPG () :
  m_xSize (2),
  m_ySize (1),
  m_numFlows(1),
  m_seed (1),
  m_standardPhy(1),
  m_step (120.0),
  m_randomStart (0.1),
  m_totalTime (50.0),
  m_packetInterval (0.1),
  m_timeStartFlowSources(1),
  m_packetSize (512),
  m_nIfaces (1),
  m_chan (true),
  m_pcap (false),
  m_stack ("ns3::Dot11sStack"),
  m_root ("ff:ff:ff:ff:ff:ff")
{
}

void MeshAulaRPG::Configure (int argc, char *argv[]){
  CommandLine cmd;
  cmd.AddValue ("x-size", "Number of nodes in a row grid. [3]", m_xSize);
  cmd.AddValue ("y-size", "Number of rows in a grid. [3]", m_ySize);
  cmd.AddValue ("numFlows", "Number of flows. [1]", m_numFlows);
  cmd.AddValue ("seed", "Seed [1]", m_seed);
  cmd.AddValue ("standardPhy",  "PHY layer Definition. WIFI_PHY_STANDARD_80211n_5GHZ by default", m_standardPhy);
  cmd.AddValue ("step",   "Size of edge in our grid, meters. [100 m]", m_step);
  /*
   * As soon as starting node means that it sends a beacon,
   * simultaneous start is not good.
   */
  cmd.AddValue ("start",  "Maximum random start delay, seconds. [0.1 s]", m_randomStart);
  cmd.AddValue ("time",  "Simulation time, seconds [100 s]", m_totalTime);
  cmd.AddValue ("packet-interval",  "Interval between packets in UDP ping, seconds [0.001 s]", m_packetInterval);
  cmd.AddValue ("timeStartFlowSources",  "Time to start source flows, seconds [1 s]", m_timeStartFlowSources);
  cmd.AddValue ("packet-size",  "Size of packets in UDP ping", m_packetSize);
  cmd.AddValue ("interfaces", "Number of radio interfaces used by each mesh point. [1]", m_nIfaces);
  cmd.AddValue ("channels",   "Use different frequency channels for different interfaces. [0]", m_chan);
  cmd.AddValue ("pcap",   "Enable PCAP traces on interfaces. [0]", m_pcap);
  cmd.AddValue ("stack",  "Type of protocol stack. ns3::Dot11sStack by default", m_stack);
  cmd.AddValue ("root", "Mac address of root mesh point in HWMP", m_root);

  cmd.Parse (argc, argv);
  NS_LOG_DEBUG ("Grid:" << m_xSize << "*" << m_ySize);
  NS_LOG_DEBUG ("Simulation time: " << m_totalTime << " s");
}

enum WifiPhyStandard getEnumFromString(int phy){
    //ps: src/wifi/model/wifi-phy-standard.h
    switch(phy)
    {   case 1:
            return WIFI_PHY_STANDARD_80211a; //5 GHz
        case 2:
            return WIFI_PHY_STANDARD_80211b;//2.4 GHz
        case 3:
            return WIFI_PHY_STANDARD_80211g;//2.4 GHz
        case 4:
            return WIFI_PHY_STANDARD_80211_10MHZ;
        case 5:
            return WIFI_PHY_STANDARD_80211_5MHZ;
        //case 6:
          //  return WIFI_PHY_STANDARD_holland;
        default:
            return WIFI_PHY_STANDARD_80211n_2_4GHZ;
    }
}

void MeshAulaRPG::CreateNodes (){
  /*
   * Create m_ySize*m_xSize stations to form a grid topology
   */
  nodes.Create (m_ySize*m_xSize);
  // Configure YansWifiChannel
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  /*
   * Create mesh helper and set stack installer to it
   * Stack installer creates all needed protocols and install them to
   * mesh point device
   */
  mesh = MeshHelper::Default ();
  if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
    {
      mesh.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
    }
  else
    {
      //If root is not set, we do not use "Root" attribute, because it
      //is specified only for 11s
      mesh.SetStackInstaller (m_stack);
    }
  if (m_chan)
    {
      mesh.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
    }
  else
    {
      mesh.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
    }
    
  mesh.SetStandard(getEnumFromString(m_standardPhy));
    
  mesh.SetMacType ("RandomStart", TimeValue (Seconds (m_randomStart)));
  // Set number of interfaces - default is single-interface mesh point
  mesh.SetNumberOfInterfaces (m_nIfaces);
  // Install protocols and return container if MeshPointDevices
  meshDevices = mesh.Install (wifiPhy, nodes);
  // Setup mobility - static grid topology
  MobilityHelper mobility;
  SeedManager::SetSeed (m_seed);
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (m_step),
                                 "DeltaY", DoubleValue (m_step),
                                 "GridWidth", UintegerValue (m_xSize),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
  if (m_pcap)
    wifiPhy.EnablePcapAll (std::string ("mp-"));

  PrintPositionNodes();
}

void MeshAulaRPG::PrintPositionNodes (){

    // NS_LOG_DEBUG ("\nPosition of nodes: ");

    // NodeContainer const & n = NodeContainer::GetGlobal ();

    // for (NodeContainer::Iterator i = n.Begin (); i != n.End (); ++i)
    // {
    //     Ptr<Node> node = *i;
    //     Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();

    //     if (! mob) continue; // Strange -- node has no mobility model installed. Skip.
    //     Vector pos = mob->GetPosition ();

    //     std::cout << "Node is at (" << pos.x << ", " << pos.y << ")\n";
    // }
}

void MeshAulaRPG::InstallInternetStack (){
  InternetStackHelper internetStack;
  internetStack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  interfaces = address.Assign (meshDevices);
}

bool MeshAulaRPG::ExistSource (int candidateSource,int sizeVector, int vec[]){
    for(int i=0; i < sizeVector; i++){
        if (candidateSource == vec[i])
            return true;
    }
    return false;
}

void MeshAulaRPG::InstallApplication (){
  int sources[m_numFlows];
  int candidateSource;
  int limit = ((m_xSize*m_ySize)-1);

  UdpServerHelper server (9);
  //node 0 is always the server
  ApplicationContainer serverApps = server.Install (nodes.Get (0));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (m_totalTime));

  UdpClientHelper client (interfaces.GetAddress (0), 9);
  client.SetAttribute ("MaxPackets", UintegerValue ((uint32_t)(m_totalTime*(1/m_packetInterval))));
  client.SetAttribute ("Interval", TimeValue (Seconds (m_packetInterval)));
  client.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  ApplicationContainer clientApps;

  srand(time(NULL));
  for (int index=1; index <= m_numFlows; ++index) {
      candidateSource = 1 + (rand() % (limit));
      while(ExistSource(candidateSource,index,sources)){
          candidateSource = 1 + (rand() % (limit));
      }
      sources[index] = candidateSource;
      clientApps = client.Install (nodes.Get (candidateSource));
      clientApps.Start (Seconds (m_timeStartFlowSources*(0.01*candidateSource)));
  }
  clientApps.Stop (Seconds (Seconds (m_totalTime-1)));
}

int MeshAulaRPG::Run() {

  CreateNodes();
  InstallInternetStack();
  InstallApplication();

  int counterNbFlow = 0;

  FlowMonitorHelper flowmon;

  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  std::vector< ns3::Ptr<ns3::FlowProbe> > flowProbes = monitor->GetAllProbes ();

  Simulator::Schedule (Seconds (m_totalTime), &MeshAulaRPG::Report, this);

  Simulator::Stop (Seconds (m_totalTime));
  Simulator::Run ();

  monitor->SerializeToXmlFile("results.xml", true, true);
  monitor->CheckForLostPackets ();

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  {
        if (i->second.rxBytes > 0){
            
            std::string MY_FILE = "/content/a/ns-allinone-3.28/ns-3.28/scratch/data.csv";

            if(std::fstream{MY_FILE}){
              std::ofstream myfile(MY_FILE, std::ios::app);
              if (myfile.is_open()){

                  myfile << m_xSize << ";";
                  myfile << m_ySize << ";";
                  myfile << m_numFlows << ";";
                  myfile << m_seed << ";";
                  myfile << m_standardPhy << ";";
                  myfile << m_step << ";";
                  myfile << m_packetSize << ";";
                  myfile << m_packetInterval << ";";

                  myfile << (i->second.rxPackets * 100.0)/ (i->second.txPackets)  << ";";
                  myfile << (i->second.rxBytes * 8.0 / 10.0 / 1024 / 1024)  << ";";
                  myfile << (i->second.delaySum.GetSeconds()) / (i->second.rxPackets) << ";";
                  myfile << (i->second.jitterSum.GetSeconds()) / (i->second.rxPackets -1) << ";";

                  myfile << i->second.txBytes << ";";
                  myfile << i->second.rxBytes << ";";
                  myfile << i->second.txPackets << ";";
                  myfile << i->second.rxPackets << ";";
                  myfile << (i->second.lostPackets) << ";";
                  myfile << i->second.delaySum.GetSeconds() << ";";
                  myfile << i->second.jitterSum.GetSeconds() << ";";

                  myfile << (i->second.txBytes) / (i->second.txPackets) << ";";
                  myfile << ((i->second.txBytes) * 8.0) / ((i->second.timeLastTxPacket.GetSeconds())-(i->second.timeFirstTxPacket.GetSeconds())) << ";";
                  myfile << (i->second.timesForwarded) / (i->second.rxPackets) + 1 << ";";
                  myfile << (i->second.lostPackets) / ((i->second.rxPackets)+(i->second.lostPackets)) << ";";
                  myfile << (i->second.rxBytes) / (i->second.rxPackets) << ";";
                  myfile << ((i->second.rxBytes)* 8.0) / ((i->second.timeLastRxPacket.GetSeconds())-(i->second.timeFirstRxPacket.GetSeconds())) << ";";

                  myfile << "\n";

                  myfile.close();
                  std::cout << "CSV: data.csv nova linha adicionada.\n";
              }
            }else{
              std::ofstream myfile(MY_FILE, std::ios::app);

              myfile << "xSize;";
              myfile << "ySize;";
              myfile << "NumberFlows;";
              myfile << "Seed;";
              myfile << "StandardPHY;";
              myfile << "Step;";
              myfile << "PacketSize;";
              myfile << "PacketInterval;";

              myfile << "DeliveryRate;";
              myfile << "Throughput;";
              myfile << "DelayMean;";
              myfile << "JitterMean;";

              myfile << "TxBytes;";
              myfile << "RxBytes;";
              myfile << "TxPackets;";
              myfile << "RxPackets;";
              myfile << "LostPackets;";
              myfile << "DelaySum;";
              myfile << "JitterSum;";

              myfile << "MeanTransmittedPacketSize;";
              myfile << "MeanTransmittedBitrate;";
              myfile << "MeanHopCount;";
              myfile << "PacketLossRatio;";
              myfile << "MeanReceivedPacketSize;";
              myfile << "MeanReceivedBitrate;";
              
              myfile << "\n";

              myfile.close();
              std::cout << "CSV: data.csv criado com sucesso.\n";
            }

           counterNbFlow++;
        }
    }

  // NS_LOG_DEBUG ("\nNumber of started flows = "<< counterNbFlow);

  Simulator::Destroy();
  return 0;
}

void MeshAulaRPG::Report(){
  unsigned n (0);
  for (NetDeviceContainer::Iterator i = meshDevices.Begin (); i != meshDevices.End (); ++i, ++n)
    {
      std::ostringstream os;
      os << "mp-report-" << n << ".xml";
      // std::cerr << "Printing mesh point device #" << n << " diagnostics to " << os.str () << "\n";
      std::ofstream of;
      of.open (os.str ().c_str ());
      if (!of.is_open ())
        {
          std::cerr << "Error: Can't open file " << os.str () << "\n";
          return;
        }
      mesh.Report (*i, of);
      of.close ();
    }
}

void MeshAulaRPG::LoopForVariance(){
  for(int i = index_seed; i <10; i++){
    Run();
    m_seed++;
    index_seed++;
  }
}

int main (int argc, char *argv[]){
  MeshAulaRPG t;
  t.Configure (argc, argv);
  // t.LoopForVariance();
  t.Run();

  return 0;
}
