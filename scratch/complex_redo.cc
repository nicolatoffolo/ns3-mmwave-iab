# include "../scratch/simulation-config/simulation-config.h"

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
  int num_ue = 4;
  double start_time = 0.5; //applications start time
  double stop_time = 0.6; //applications stop time

  cmd.AddValue("stop_time", "duration of the simulation", stop_time);
  cmd.AddValue("run", "run for RNG", run);
  cmd.AddValue("am", "RLC in AM if true", rlc_am);
  cmd.AddValue("UDP_DL", "if true, UDP downlink traffic will be instantiated", UDP_DL);
  cmd.AddValue("UDP_UL", "if true, UDP uplink traffic will be instantiated", UDP_UL);
  cmd.AddValue("TCP_DL", "if true, TCP downlink traffic will be instantiated", TCP_DL);
  cmd.AddValue("TCP_UL", "if true, TCP uplink traffic will be instantiated", TCP_UL);
  cmd.AddValue("rlc_buf_size", "size of RLC buffer [MB]", rlc_buf_size);
  cmd.AddValue("inter_pck", "inter pck interval [us]", inter_pck);
  cmd.AddValue("num_ue", "number of users to be deployed in the scenario", num_ue);
  cmd.Parse(argc, argv);

  //default parameters for the simulation
  Config::SetDefault("ns3::MmWavePhyMacCommon::UlSchedDelay", UintegerValue(1));
  Config::SetDefault("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue(rlc_buf_size * 1024 * 1024));
  Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(rlc_buf_size * 1024 * 1024));

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

  Config::SetDefault("ns3::MmWave3gppPropagationLossModel::ScenarioEnbEnb", StringValue("UMi-StreetCanyon"));
  Config::SetDefault("ns3::MmWave3gppPropagationLossModel::ScenarioEnbUe", StringValue("UMi-StreetCanyon"));
  Config::SetDefault("ns3::MmWave3gppPropagationLossModel::ScenarioEnbIab", StringValue("UMi-StreetCanyon"));
  Config::SetDefault("ns3::MmWave3gppPropagationLossModel::ScenarioIabIab", StringValue("UMi-StreetCanyon"));
  Config::SetDefault("ns3::MmWave3gppPropagationLossModel::ScenarioIabUe", StringValue("UMi-StreetCanyon"));
  Config::SetDefault("ns3::MmWave3gppPropagationLossModel::ScenarioUeUe", StringValue("UMi-StreetCanyon"));

  RngSeedManager::SetSeed(15);
  RngSeedManager::SetRun(run);

  //mmWave module initialization and connection to the core
  Ptr<MmWaveHelper> mm_wave_helper = CreateObject<MmWaveHelper>();
  mm_wave_helper->SetChannelModelType("ns3::MmWave3gppChannel");
  mm_wave_helper->SetPathlossModelType("ns3::MmWave3gppBuildingsPropagationLossModel");
  Ptr<MmWavePointToPointEpcHelper> epc_helper = CreateObject<MmWavePointToPointEpcHelper>(); //conditions deterministically
  mm_wave_helper->SetEpcHelper(epc_helper);
  mm_wave_helper->Initialize();

  //remote server def and topology connecting them to the core
  std::pair<Ptr<Node>, Ipv4Address> rhPair = SimulationConfig::CreateInternet(epc_helper);
  Ptr<Node> remoteHost = rhPair.first;
  //Ipv4Address remoteHostAddr = rhPair.second;


  //positioning the building in a grid with streets separing them
  
  double street_width = 10;    //meters
  double building_widthX = 50; //meters
  double building_widthY = 50; //meters
  double building_height = 10; //meters
  int num_buildings_row = 4;
  int num_buildings_column = 4;
  std::vector<Ptr<Building>> building_vector;

  for (int row_index = 0; row_index < num_buildings_row; ++row_index)
  {
    double min_y_building = row_index * (building_widthY + street_width);
    for (int col_index = 0; col_index < num_buildings_column; ++col_index)
    {
      double min_x_building = col_index * (building_widthX + street_width);
      Ptr<Building> building;
      building = Create<Building>();
      building->SetBoundaries(Box(min_x_building, min_x_building + building_widthX, min_y_building, min_y_building + building_widthY, 0.0, building_height));
      building->SetNRoomsX(1);
      building->SetNRoomsY(1);
      building->SetNFloors(1);
      building_vector.push_back(building);
      //Box building_box_for_log = building->GetBoundaries();

      //NS_LOG_INFO("Created building between coordinates("<< building_box_for_log.xMin <<", "<<building_box_for_log.yMin <<"), ("<< building_box_for_log.xMax <<", "<<building_box_for_log.yMin <<"), ("<< building_box_for_log.xMin <<", "<<building_box_for_log.yMax <<"), ("<< building_box_for_log.xMax <<", "<<building_box_for_log.yMax <<") "<<"with height "<< building_box_for_log.zMax - building_box_for_log.zMin <<" meters");
    }
  }

  //also the gNB, the IAB nodes and the UE nodes are created
  NodeContainer ue_nodes;
  NodeContainer gnb_nodes;
  NodeContainer iab_nodes;

  ue_nodes.Create(num_ue);
  iab_nodes.Create(1); //only 1 IAB
  gnb_nodes.Create(1); //only 1 gNB

  //preliminary quantities
  double x_max = num_buildings_row * (building_widthX + street_width) - street_width;
  double y_max = num_buildings_column * (building_widthY + street_width) - street_width;
  //double area = x_max * y_max;
  double antenna_height = building_height / 2; //for now, antenna is half a building tall
  double ue_height = 1.75;  //meters

  //mobility Model (gNB)
  Ptr<ListPositionAllocator> gnb_pos_alloc = CreateObject<ListPositionAllocator>();
  gnb_pos_alloc->Add(Vector(x_max/2, y_max/2, antenna_height));
  MobilityHelper gnb_mobility;
  gnb_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  gnb_mobility.SetPositionAllocator(gnb_pos_alloc);
  gnb_mobility.Install(gnb_nodes);

  //mobility Model (IAB)
  Ptr<ListPositionAllocator> iab_pos_alloc = CreateObject<ListPositionAllocator>();
  //iab_pos_alloc->Add(Vector(x_max*0.25, y_max*0.5, antenna_height));
  //iab_pos_alloc->Add(Vector(x_max*0.75, y_max*0.5, antenna_height));
  //iab_pos_alloc->Add(Vector(x_max*0.5, y_max*0.25, antenna_height));
  iab_pos_alloc->Add(Vector(x_max*0.5, y_max*0.75, antenna_height));
  MobilityHelper iab_mobility;
  iab_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  iab_mobility.SetPositionAllocator(iab_pos_alloc);
  iab_mobility.Install(iab_nodes);

  //mobility Model (single UE)
  Ptr<ListPositionAllocator> ue_pos_alloc = CreateObject<ListPositionAllocator>();
  for (int n = 0; n < num_ue; ++n)
  {
    Ptr<OutdoorPositionAllocator> ue_pos= CreateObject<OutdoorPositionAllocator>();
    Ptr<UniformRandomVariable> x_pos = CreateObject<UniformRandomVariable>();
    x_pos->SetAttribute ("Min", DoubleValue (0.0));
    x_pos->SetAttribute ("Max", DoubleValue (x_max));
    Ptr<UniformRandomVariable> y_pos = CreateObject<UniformRandomVariable>();
    y_pos->SetAttribute ("Min", DoubleValue (y_max*0.625));
    y_pos->SetAttribute ("Max", DoubleValue (y_max));
    Ptr<UniformRandomVariable> z_pos = CreateObject<UniformRandomVariable>();
    z_pos->SetAttribute ("Min", DoubleValue (ue_height));
    z_pos->SetAttribute ("Max", DoubleValue (ue_height));
    ue_pos->SetX(PointerValue (x_pos));
    ue_pos->SetY(PointerValue (y_pos));
    ue_pos->SetZ(PointerValue (z_pos));
    ue_pos_alloc->Add(ue_pos->GetNext ());
  }
  MobilityHelper ue_mobility;
  ue_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  ue_mobility.SetPositionAllocator(ue_pos_alloc);
  ue_mobility.Install(ue_nodes);

  //let the buildings be aware of the nodes' position
  BuildingsHelper::Install(iab_nodes);
  BuildingsHelper::Install(ue_nodes);
  BuildingsHelper::Install(gnb_nodes);
  BuildingsHelper::MakeMobilityModelConsistent();

  //MmWaveHelper creates relevant NetDevice instances for the different nodes
  NetDeviceContainer gnb_mm_wave_devices = mm_wave_helper->InstallEnbDevice(gnb_nodes);
  NetDeviceContainer iab_mm_wave_devices = mm_wave_helper->InstallIabDevice(iab_nodes);
  NetDeviceContainer ue_mm_wave_devices = mm_wave_helper->InstallUeDevice(ue_nodes);

  //create InternetStack for Ue devices and determination of default gw
  Ipv4InterfaceContainer ue_ip_interfaces = SimulationConfig::InstallUeInternet (epc_helper, ue_nodes, ue_mm_wave_devices);

  NetDeviceContainer possibleBaseStations(gnb_mm_wave_devices, iab_mm_wave_devices);
  mm_wave_helper->ChannelInitialization(ue_mm_wave_devices, gnb_mm_wave_devices, iab_mm_wave_devices);
  mm_wave_helper->AttachIabToClosestEnb(iab_mm_wave_devices, gnb_mm_wave_devices);
  mm_wave_helper->AttachToClosestEnbWithDelay(ue_mm_wave_devices, possibleBaseStations, MilliSeconds(50));

  mm_wave_helper->ActivateDonorControllerIabSetup(gnb_mm_wave_devices, iab_mm_wave_devices,
                                                  gnb_nodes, iab_nodes);

  //applications setup in UEs and remote host
  
  uint16_t dl_port = 1234;

  for (int u = 0; u < num_ue; ++u)
  {
  	++dl_port;
    AsciiTraceHelper ascii_trace_helper;
    Ptr<OutputStreamWrapper> stream = ascii_trace_helper.CreateFileStream ("mmWave-udp-data-am-DL" + std::to_string(u) +".txt");
    SimulationConfig::SetupUdpPacketSink (ue_nodes.Get(u), dl_port, start_time, stop_time, stream);
  	
  	SimulationConfig::SetupUdpApplication (remoteHost, ue_ip_interfaces.GetAddress (u), dl_port, inter_pck, start_time, stop_time);

  }

  mm_wave_helper->EnableTraces();

  PrintHelper::PrintGnuplottableBuildingListToFile ("buildings.txt");
  PrintHelper::PrintGnuplottableNodeListToFile ("all_nodes.txt");

  //p2p.EnablePcapAll("complex_scen");

  Simulator::Stop(Seconds(stop_time));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}