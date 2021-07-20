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
  unsigned run = 67;
  bool rlc_am = true;
  uint32_t rlc_buf_size = 10;
  uint32_t inter_pck = 200;

  cmd.AddValue("run", "run for RNG", run);
  cmd.AddValue("am", "RLC in AM if true", rlc_am);
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
  Config::SetDefault ("ns3::MmWaveHelper::Scheduler", StringValue("ns3::MmWaveRrIabMacScheduler"));
  Config::SetDefault ("ns3::MmWaveIabController::CentralAllocationEnabled", BooleanValue(false));
  Config::SetDefault ("ns3::MmWaveRrIabMacScheduler::HarqEnabled", BooleanValue(true));

  Config::SetDefault("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlc_am));
  Config::SetDefault("ns3::MmWaveRrIabMacScheduler::CqiTimerThreshold", UintegerValue(100));

  RngSeedManager::SetSeed(2);
  RngSeedManager::SetRun(run);

  Config::SetDefault("ns3::MmWavePhyMacCommon::SymbolsPerSubframe", UintegerValue(240));
  Config::SetDefault("ns3::MmWavePhyMacCommon::SubframePeriod", DoubleValue(1000));
  Config::SetDefault("ns3::MmWavePhyMacCommon::SymbolPeriod", DoubleValue(1000 / 240));

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
  mm_wave_helper->ActivateDonorControllerIabSetup (gnb_mm_wave_devices, iab_mm_wave_devices, 
                                                   gnb_nodes, iab_nodes);

  //applications setup in UE and remote hosts (downlink traffic)
  uint16_t port = 8080;
  ApplicationContainer client_apps;
  ApplicationContainer server_apps;
  
  UdpServerHelper packet_sink_helper(port);
  server_apps.Add(packet_sink_helper.Install(ue_nodes.Get(0)));
  UdpClientHelper client(ue_ip_interfaces.GetAddress(0), port);
  client.SetAttribute("Interval", TimeValue(MicroSeconds(inter_pck)));
  client.SetAttribute("PacketSize", UintegerValue(1400));
  client.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
  client_apps.Add(client.Install(remote_host));

  server_apps.Start(MilliSeconds(490));
  //server_apps.Stop(Seconds(1.2));
  client_apps.Stop(MilliSeconds(1200));
  client_apps.Start(MilliSeconds(500));
  mm_wave_helper->EnableTraces();

  //TODO: Usually we do not use PCAPs. Instead, we use the traces enabled in the previous line
  //      and then we parse the results (usually with a Python script).
  //p2p.EnablePcapAll("basic-scenario");

  Simulator::Stop(MilliSeconds(1200));
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
