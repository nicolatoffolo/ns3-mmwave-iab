//the needed modules are included
#include "ns3/buildings-module.h"
#include "ns3/mmwave-helper.h"
#include "ns3/lte-module.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"

/**
 * Application that simulates the DOWNLINK TCP data over mmWave links
 * with the mmWave devices and the LTE EPC.
 * Taken from: mmwave-tcp-building-example.cc
 */
NS_LOG_COMPONENT_DEFINE("mmWaveTCPExample");

class MyAppTag : public Tag
{
public:
  MyAppTag()
  {
  }

  MyAppTag(Time sendTs) : m_sendTs(sendTs)
  {
  }

  static TypeId GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::MyAppTag")
                            .SetParent<Tag>()
                            .AddConstructor<MyAppTag>();
    return tid;
  }

  virtual TypeId GetInstanceTypeId(void) const
  {
    return GetTypeId();
  }

  virtual void Serialize(TagBuffer i) const
  {
    i.WriteU64(m_sendTs.GetNanoSeconds());
  }

  virtual void Deserialize(TagBuffer i)
  {
    m_sendTs = NanoSeconds(i.ReadU64());
  }

  virtual uint32_t GetSerializedSize() const
  {
    return sizeof(m_sendTs);
  }

  virtual void Print(std::ostream &os) const
  {
    std::cout << m_sendTs;
  }

  Time m_sendTs;
};

class MyApp : public Application
{
public:
  MyApp();
  virtual ~MyApp();
  void ChangeDataRate(DataRate rate);
  void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication(void);
  virtual void StopApplication(void);

  void ScheduleTx(void);
  void SendPacket(void);

  Ptr<Socket> m_socket;
  Address m_peer;
  uint32_t m_packetSize;
  uint32_t m_nPackets;
  DataRate m_dataRate;
  EventId m_sendEvent;
  bool m_running;
  uint32_t m_packetsSent;
};

MyApp::MyApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_nPackets(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void MyApp::ChangeDataRate(DataRate rate)
{
  m_dataRate = rate;
}

void MyApp::StartApplication(void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind();
  m_socket->Connect(m_peer);
  SendPacket();
}

void MyApp::StopApplication(void)
{
  m_running = false;

  if (m_sendEvent.IsRunning())
  {
    Simulator::Cancel(m_sendEvent);
  }

  if (m_socket)
  {
    m_socket->Close();
  }
}

void MyApp::SendPacket(void)
{
  Ptr<Packet> packet = Create<Packet>(m_packetSize);
  MyAppTag tag(Simulator::Now());

  m_socket->Send(packet);
  if (++m_packetsSent < m_nPackets)
  {
    ScheduleTx();
  }
}

void MyApp::ScheduleTx(void)
{
  if (m_running)
  {
    Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
    m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
  }
}

/*
static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}
*/

static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
}



static void Rx (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet, const Address &from)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize()<< std::endl;
}

/*static void Sstresh (Ptr<OutputStreamWrapper> stream, uint32_t oldSstresh, uint32_t newSstresh)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldSstresh << "\t" << newSstresh << std::endl;
}*/

void ChangeSpeed(Ptr<Node> n, Vector speed)
{
  n->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(speed);
}

int main(int argc, char *argv[])
{

  //enabling logging
  //LogComponentEnable("LteRlc", LOG_LEVEL_LOGIC);
  //LogComponentEnable("LteRlcAm", LOG_LEVEL_LOGIC);
  //LogComponentEnable("LtePdcp", LOG_LEVEL_LOGIC);
  LogComponentEnableAll(LOG_PREFIX_TIME); //simulation time
  LogComponentEnableAll(LOG_PREFIX_FUNC); //calling function
  LogComponentEnableAll(LOG_PREFIX_NODE); //node ID
  //LogComponentEnableAll(LOG_LEVEL_LOGIC);
  //LogComponentEnable("EpcIabApplication", LOG_LEVEL_LOGIC);
  //LogComponentEnable("EpcEnbApplication", LOG_LEVEL_LOGIC);
  //LogComponentEnable("EpcSgwPgwApplication", LOG_LEVEL_LOGIC);
  //LogComponentEnable("EpcMmeApplication", LOG_LEVEL_LOGIC);
  //LogComponentEnable("EpcUeNas", LOG_LEVEL_LOGIC);
  //LogComponentEnable("LteEnbRrc", LOG_LEVEL_INFO);
  //LogComponentEnable("LteUeRrc", LOG_LEVEL_INFO);
  //LogComponentEnable("MmWaveHelper", LOG_LEVEL_LOGIC);
  //LogComponentEnable("MmWavePointToPointEpcHelper", LOG_LEVEL_LOGIC);
  //LogComponentEnable("EpcS1ap", LOG_LEVEL_LOGIC);

  //command line params
  CommandLine cmd;
  unsigned run = 2;
  bool rlc_am = true;
  bool UDP_DL = true;
  bool UDP_UL = false;
  bool TCP_DL = false;
  bool TCP_UL = false;
  uint32_t rlc_buf_size = 10;
  uint32_t inter_pck = 200;

  cmd.AddValue("run", "run for RNG", run);
  cmd.AddValue("am", "RLC in AM if true", rlc_am);
  cmd.AddValue("UDP_DL", "if true, UDP downlink traffic will be instantiated", UDP_DL);
  cmd.AddValue("UDP_UL", "if true, UDP uplink traffic will be instantiated", UDP_UL);
  cmd.AddValue("TCP_DL", "if true, TCP downlink traffic will be instantiated", TCP_DL);
  cmd.AddValue("TCP_UL", "if true, TCP uplink traffic will be instantiated", TCP_UL);
  cmd.AddValue("rlc_buf_size", "size of RLC buffer [MB]", rlc_buf_size);
  cmd.AddValue("inter_pck", "inter pck interval [us]", inter_pck);
  cmd.Parse(argc, argv);

  //default parameters for the simulation
  Config::SetDefault("ns3::MmWavePhyMacCommon::UlSchedDelay", UintegerValue(1));
  Config::SetDefault("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue(rlc_buf_size * 1024 * 1024));
  Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(rlc_buf_size * 1024 * 1024));

  //TODO: The ns-3 time constructors accept an int as input. If you need to use non-integers,
  //      use the smallest time unit which makes the desired value an integer (the smallest option is ns).
  //      Float values cause problems
  Config::SetDefault("ns3::LteRlcAm::PollRetransmitTimer", TimeValue(MilliSeconds(1)));
  Config::SetDefault("ns3::LteRlcAm::ReorderingTimer", TimeValue(MilliSeconds(2)));
  Config::SetDefault("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(1)));
  Config::SetDefault("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MilliSeconds(5)));
  //Config::SetDefault("ns3::LteRlcUm::ReportBufferStatusTimer", TimeValue(MilliSeconds(0.5)));

  //TODO: These lines configure the patched scheduler.
  Config::SetDefault("ns3::MmWaveHelper::Scheduler", StringValue("ns3::MmWaveRrIabMacScheduler"));
  Config::SetDefault("ns3::MmWaveIabController::CentralAllocationEnabled", BooleanValue(false));
  Config::SetDefault("ns3::MmWaveRrIabMacScheduler::HarqEnabled", BooleanValue(true));

  Config::SetDefault("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlc_am));
  Config::SetDefault("ns3::MmWaveRrIabMacScheduler::CqiTimerThreshold", UintegerValue(100));

  RngSeedManager::SetSeed(2);
  RngSeedManager::SetRun(run);

  //Config::SetDefault("ns3::MmWavePhyMacCommon::SymbolsPerSubframe", UintegerValue(240));
  //Config::SetDefault("ns3::MmWavePhyMacCommon::SubframePeriod", DoubleValue(1000));
  //Config::SetDefault("ns3::MmWavePhyMacCommon::SymbolPeriod", DoubleValue(1000 / 240));

  //mmWave module initialization and connection to the core
  Ptr<MmWaveHelper> mm_wave_helper = CreateObject<MmWaveHelper>();
  mm_wave_helper->SetChannelModelType("ns3::MmWave3gppChannel");
  mm_wave_helper->SetPathlossModelType("ns3::MmWave3gppBuildingsPropagationLossModel");
  Ptr<MmWavePointToPointEpcHelper> epc_helper = CreateObject<MmWavePointToPointEpcHelper>(); //conditions deterministically
  mm_wave_helper->SetEpcHelper(epc_helper);
  mm_wave_helper->Initialize();

  //remote servers (only 1 in this case) def and topology connecting them to the core
  Ptr<Node> pgw = epc_helper->GetPgwNode();

  NodeContainer remote_host_container;
  remote_host_container.Create(1);
  Ptr<Node> remote_host = remote_host_container.Get(0);
  InternetStackHelper internet; //declared once
  internet.Install(remote_host_container);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
  p2p.SetDeviceAttribute("Mtu", UintegerValue(1500));
  p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));
  NetDeviceContainer internet_devices = p2p.Install(pgw, remote_host);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer ip_interfaces = ipv4.Assign(internet_devices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remote_host_addr = ip_interfaces.GetAddress(1);

  Ipv4StaticRoutingHelper ipv4_routing_helper;
  Ptr<Ipv4StaticRouting> remote_host_static_routing = ipv4_routing_helper.GetStaticRouting(remote_host->GetObject<Ipv4>());
  remote_host_static_routing->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

  //positioning the building
  /*

              IAB


           ///////////
           ///////////
  gNB      ///////////      UE
           ///////////
           ///////////


  */

  Ptr<Building> building;
  building = Create<Building>();
  building->SetBoundaries(Box(20.0, 40.0,
                              0.0, 20.0,
                              0.0, 50.0));
  building->SetBuildingType(Building::Residential);
  building->SetExtWallsType(Building::ConcreteWithWindows);
  building->SetNFloors(1);
  building->SetNRoomsX(1);
  building->SetNRoomsY(1);

  //also the gNB, the IAB nodes and the UE nodes are created
  NodeContainer ue_nodes;
  NodeContainer gnb_nodes;
  NodeContainer iab_nodes;

  gnb_nodes.Create(1);
  iab_nodes.Create(1);
  ue_nodes.Create(1);

  //mobility Model (gNB)
  Ptr<ListPositionAllocator> gnb_pos_alloc = CreateObject<ListPositionAllocator>();
  gnb_pos_alloc->Add(Vector(0.0, 10.0, 25.0));
  MobilityHelper gnb_mobility;
  gnb_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  gnb_mobility.SetPositionAllocator(gnb_pos_alloc);
  gnb_mobility.Install(gnb_nodes);

  //mobility Model (IAB)
  Ptr<ListPositionAllocator> iab_pos_alloc = CreateObject<ListPositionAllocator>();
  iab_pos_alloc->Add(Vector(30.0, 30.0, 10.0));
  MobilityHelper iab_mobility;
  iab_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  iab_mobility.SetPositionAllocator(iab_pos_alloc);
  iab_mobility.Install(iab_nodes);

  //mobility Model (single UE)
  Ptr<ListPositionAllocator> ue_pos_alloc = CreateObject<ListPositionAllocator>();
  ue_pos_alloc->Add(Vector(60.0, 10.0, 1.75));
  MobilityHelper ue_mobility;
  ue_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  ue_mobility.SetPositionAllocator(ue_pos_alloc);
  ue_mobility.Install(ue_nodes);

  //let the buildings be aware of the nodes' position
  BuildingsHelper::Install(gnb_nodes);
  BuildingsHelper::Install(iab_nodes);
  BuildingsHelper::Install(ue_nodes);
  BuildingsHelper::MakeMobilityModelConsistent();

  //MmWaveHelper creates relevant NetDevice instances for the different nodes
  NetDeviceContainer gnb_mm_wave_devices = mm_wave_helper->InstallEnbDevice(gnb_nodes);
  NetDeviceContainer iab_mm_wave_devices = mm_wave_helper->InstallIabDevice(iab_nodes);
  NetDeviceContainer ue_mm_wave_devices = mm_wave_helper->InstallUeDevice(ue_nodes);

  //create InternetStack for Ue devices and determination of default gw
  internet.Install(ue_nodes);
  Ipv4InterfaceContainer ue_ip_interfaces;
  ue_ip_interfaces = epc_helper->AssignUeIpv4Address(NetDeviceContainer(ue_mm_wave_devices));

  for (uint32_t u = 0; u < ue_nodes.GetN(); ++u)
  {
    Ptr<Node> ue_node = ue_nodes.Get(u);
    Ptr<Ipv4StaticRouting> ue_static_routing = ipv4_routing_helper.GetStaticRouting(ue_node->GetObject<Ipv4>());
    ue_static_routing->SetDefaultRoute(epc_helper->GetUeDefaultGatewayAddress(), 1);
  }

  NetDeviceContainer possibleBaseStations(gnb_mm_wave_devices, iab_mm_wave_devices);
  mm_wave_helper->ChannelInitialization(ue_mm_wave_devices, gnb_mm_wave_devices, iab_mm_wave_devices);
  mm_wave_helper->AttachIabToClosestEnb(iab_mm_wave_devices, gnb_mm_wave_devices);
  mm_wave_helper->AttachToClosestEnbWithDelay(ue_mm_wave_devices, possibleBaseStations, MilliSeconds(300));

  //TODO:  This line is needed to setup the patched scheduler. Keep here (just after the devices attachment)
  mm_wave_helper->ActivateDonorControllerIabSetup(gnb_mm_wave_devices, iab_mm_wave_devices,
                                                  gnb_nodes, iab_nodes);

                                                  
  //TCP applications setup in UE and remote hosts (downlink & uplink traffic)
  if (TCP_DL || TCP_UL) {

    uint16_t sinkPort1 = 20000;
    uint16_t sinkPort2 = 30000;

    Address sinkAddress1(InetSocketAddress(ue_ip_interfaces.GetAddress(0), sinkPort1));
    Address sinkAddress2(InetSocketAddress(remote_host_addr, sinkPort2));

    PacketSinkHelper packetSinkHelper1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort1));
    PacketSinkHelper packetSinkHelper2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort2));

    ApplicationContainer sinkApps;

    if (TCP_DL && TCP_UL) {

      sinkApps.Add (packetSinkHelper1.Install (ue_nodes.Get (0)));
      sinkApps.Add (packetSinkHelper2.Install (remote_host));
      sinkApps.Start (MilliSeconds (0.));
	    sinkApps.Stop (MilliSeconds (1200));
      
      Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (remote_host_container.Get (0), TcpSocketFactory::GetTypeId ());

      Ptr<MyApp> app1 = CreateObject<MyApp>();
      app1->Setup (ns3TcpSocket1, sinkAddress1, 1400, 5000000, DataRate ("1000Mb/s"));

      remote_host_container.Get (0)->AddApplication (app1);

      AsciiTraceHelper asciiTraceHelper1;
      Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper1.CreateFileStream ("mmWave-tcp-data-am-DL.txt");
	    sinkApps.Get(0)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream1));

      app1->SetStartTime(MilliSeconds(500));
      app1->SetStopTime(MilliSeconds(1200));

      Ptr<Socket> ns3TcpSocket2 = Socket::CreateSocket (ue_nodes.Get (0), TcpSocketFactory::GetTypeId ());

      Ptr<MyApp> app2 = CreateObject<MyApp>();
      app2->Setup (ns3TcpSocket2, sinkAddress2, 1400, 5000000, DataRate ("1000Mb/s"));

      ue_nodes.Get (0)->AddApplication (app2);
      
      AsciiTraceHelper asciiTraceHelper2;
      Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper2.CreateFileStream ("mmWave-tcp-data-am-UL.txt");
	    sinkApps.Get(1)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream2));

      app2->SetStartTime(MilliSeconds(500));
      app2->SetStopTime(MilliSeconds(1200));

    }
    else if (TCP_UL) {
      
      sinkApps.Add (packetSinkHelper2.Install (remote_host));
      sinkApps.Start (MilliSeconds (0.));
	    sinkApps.Stop (MilliSeconds (1200));

      Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (ue_nodes.Get (0), TcpSocketFactory::GetTypeId ());

      Ptr<MyApp> app = CreateObject<MyApp>();
      app->Setup (ns3TcpSocket, sinkAddress2, 1400, 5000000, DataRate ("1000Mb/s"));

      ue_nodes.Get (0)->AddApplication (app);
      
      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-data-am-UL.txt");
	    sinkApps.Get(0)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream1));

      app->SetStartTime(MilliSeconds(500));
      app->SetStopTime(MilliSeconds(1200));

    }
    else {
      
      sinkApps.Add (packetSinkHelper1.Install (ue_nodes.Get (0)));
      sinkApps.Start (MilliSeconds (0.));
	    sinkApps.Stop (MilliSeconds (1200));

      Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (remote_host_container.Get (0), TcpSocketFactory::GetTypeId ());

      Ptr<MyApp> app = CreateObject<MyApp>();
      app->Setup (ns3TcpSocket, sinkAddress1, 1400, 5000000, DataRate ("1000Mb/s"));

      remote_host_container.Get (0)->AddApplication (app);

      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-data-am-DL.txt");
	    sinkApps.Get(0)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream1));

      Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-RTT-am-DL.txt");
	    ns3TcpSocket->TraceConnectWithoutContext("RTT", MakeBoundCallback (&RttChange, stream2));


      app->SetStartTime(MilliSeconds(500));
      app->SetStopTime(MilliSeconds(1200));

    }
  }
  
  // UDP applications setup in UE and remote hosts (downlink & uplink traffic)
  else
  {

    uint16_t sinkPort1 = 20000;
    uint16_t sinkPort2 = 30000;

    Address sinkAddress1(InetSocketAddress(ue_ip_interfaces.GetAddress(0), sinkPort1));
    Address sinkAddress2(InetSocketAddress(remote_host_addr, sinkPort2));

    PacketSinkHelper packetSinkHelper1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort1));
    PacketSinkHelper packetSinkHelper2("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort2));

    ApplicationContainer sinkApps;

    if (UDP_DL && UDP_UL)
    {
      sinkApps.Add (packetSinkHelper1.Install (ue_nodes.Get (0)));
      sinkApps.Add (packetSinkHelper2.Install (remote_host));
      sinkApps.Start (MilliSeconds (0.));
	    sinkApps.Stop (MilliSeconds (1200));
      
      Ptr<Socket> ns3UdpSocket1 = Socket::CreateSocket (remote_host_container.Get (0), UdpSocketFactory::GetTypeId ());

      Ptr<MyApp> app1 = CreateObject<MyApp>();
      app1->Setup (ns3UdpSocket1, sinkAddress1, 1400, 5000000, DataRate ("1000Mb/s"));

      remote_host_container.Get (0)->AddApplication (app1);

      AsciiTraceHelper asciiTraceHelper1;
      Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper1.CreateFileStream ("mmWave-udp-data-am-DL.txt");
	    sinkApps.Get(0)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream1));

      app1->SetStartTime(MilliSeconds(500));
      app1->SetStopTime(MilliSeconds(1200));

      Ptr<Socket> ns3UdpSocket2 = Socket::CreateSocket (ue_nodes.Get (0), UdpSocketFactory::GetTypeId ());

      Ptr<MyApp> app2 = CreateObject<MyApp>();
      app2->Setup (ns3UdpSocket2, sinkAddress2, 1400, 5000000, DataRate ("1000Mb/s"));

      ue_nodes.Get (0)->AddApplication (app2);
      
      AsciiTraceHelper asciiTraceHelper2;
      Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper2.CreateFileStream ("mmWave-udp-data-am-UL.txt");
	    sinkApps.Get(1)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream2));

      app2->SetStartTime(MilliSeconds(500));
      app2->SetStopTime(MilliSeconds(1200));

    }
    else if (UDP_UL)
    { 
      sinkApps.Add (packetSinkHelper2.Install (remote_host));
      sinkApps.Start (MilliSeconds (0.));
	    sinkApps.Stop (MilliSeconds (1200));

      Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (ue_nodes.Get (0), UdpSocketFactory::GetTypeId ());

      Ptr<MyApp> app = CreateObject<MyApp>();
      app->Setup (ns3UdpSocket, sinkAddress2, 1400, 5000000, DataRate ("1000Mb/s"));

      ue_nodes.Get (0)->AddApplication (app);
      
      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("mmWave-udp-data-am-UL.txt");
	    sinkApps.Get(0)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream1));

      app->SetStartTime(MilliSeconds(500));
      app->SetStopTime(MilliSeconds(1200));

    }
    else
    { //default: UDL_DL only
      sinkApps.Add (packetSinkHelper1.Install (ue_nodes.Get (0)));
      sinkApps.Start (MilliSeconds (0.));
	    sinkApps.Stop (MilliSeconds (1200));

      Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (remote_host_container.Get (0), UdpSocketFactory::GetTypeId ());

      Ptr<MyApp> app = CreateObject<MyApp>();
      app->Setup (ns3UdpSocket, sinkAddress1, 1400, 5000000, DataRate ("1000Mb/s"));

      remote_host_container.Get (0)->AddApplication (app);

      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("mmWave-udp-data-am-DL.txt");
	    sinkApps.Get(0)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream1));
      
      app->SetStartTime(MilliSeconds(500));
      app->SetStopTime(MilliSeconds(1200));

    }
  }
  
  mm_wave_helper->EnableTraces();

  //p2p.EnablePcapAll("basic_scen");

  Simulator::Stop(MilliSeconds(1200));
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
