/*
In this scenario we deploy a number of n users, k IABs and m gNBs.
In particular, a number numBuild of buildings will be placed in the interested area.
*/

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
//#include "ns3/gtk-config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/random-variable-stream.h>
#include <ns3/lte-ue-net-device.h>

#include <vector>
#include <iostream>
#include <ctime>
#include <stdlib.h>
#include <list>

using namespace ns3;

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

void PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      std::string msg = "Can't open file " + filename;
      NS_LOG_ERROR (msg);
      return;
    }
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
    {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index
              << " rect from " << box.xMin  << "," << box.yMin
              << " to "   << box.xMax  << "," << box.yMax
              << " front fs empty "
              << std::endl;
    }
}

void PrintGnuplottableUeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<MmWaveUeNetDevice> mmuedev = node->GetDevice (j)->GetObject <MmWaveUeNetDevice> ();
          Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
          outFile << "set label \"" << mmuedev->GetImsi ()
                  << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"black\" offset 0,0"
                  << std::endl;
        }
    }
}

void PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<MmWaveEnbNetDevice> mmdev = node->GetDevice (j)->GetObject <MmWaveEnbNetDevice> ();
          Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
          outFile << "set label \"" << mmdev->GetCellId ()
                  << "\" at "<< pos.x << "," << pos.y
                  << " left font \"Helvetica,8\" textcolor rgb \"red\" front  point pt 4 ps 0.3 lc rgb \"red\" offset 0,0"
                  << std::endl;
        }
    }
}

void PrintGnuplottableIabListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<MmWaveIabNetDevice> mmdev = node->GetDevice (j)->GetObject <MmWaveIabNetDevice> ();
          Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
          outFile << "set label \"" << mmdev->GetCellId ()
                  << "\" at "<< pos.x << "," << pos.y
                  << " left font \"Helvetica,8\" textcolor rgb \"red\" front  point pt 4 ps 0.3 lc rgb \"red\" offset 0,0"
                  << std::endl;
        }
    }
}

void 
PrintLostUdpPackets(Ptr<UdpServer> app, std::string fileName)
{
  std::ofstream logFile(fileName.c_str(), std::ofstream::app);
  logFile << Simulator::Now().GetSeconds() << " " << app->GetLost() << std::endl;
  logFile.close();
  Simulator::Schedule(MilliSeconds(20), &PrintLostUdpPackets, app, fileName);
}


void
ChangeSpeed(Ptr<Node>  n, Vector speed)
{
	n->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (speed);
}

void
ChangePosIfNeeded(Ptr<Node> n, double xMin, double xMax, double yMin, double yMax, bool fixed)
{
  Vector actual_pos = n->GetObject<ConstantVelocityMobilityModel> ()->DoGetPosition ();
  Vector new_speed;
  if (fixed) {
    if (actual_pos.y > yMax){
      new_speed = Vector(0, -2, 0);
    }
    else if (actual_pos.y < yMin){
      new_speed = Vector(0, 2, 0);
    }
    else {
      return;
    }
  }
  else {
    if (actual_pos.x > xMax){
      new_speed = Vector(-2, 0, 0);
    }
    else if (actual_pos.x < xMin){
      new_speed = Vector(2, 0, 0);
    }
    else {
      return;
    }
  ChangeSpeed(n, new_speed);
}

int main(int argc, char *argv[]) { 
  
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
  int stop_time = 2;
  unsigned run = 2;
  bool rlc_am = true;
  bool UDP_DL = true;
  bool UDP_UL = false;
  bool TCP_DL = false;
  bool TCP_UL = false;
  uint32_t rlc_buf_size = 10;
  uint32_t inter_pck = 200;
  uint32_t num_ue = 1;
  uint32_t num_iab = 1;
  uint32_t num_gnb = 1;
  uint32_t num_buildings_row = 4;
  uint32_t num_buildings_column = 4; 

  cmd.AddValue("stop_time", "duration of the simulation", stop_time);
	cmd.AddValue("run", "run for RNG", run);
  cmd.AddValue("am", "RLC in AM if true", rlc_am);
  cmd.AddValue("UDP_DL", "if true, UDP downlink traffic will be instantiated", UDP_DL);
  cmd.AddValue("UDP_UL", "if true, UDP uplink traffic will be instantiated", UDP_UL);
  cmd.AddValue("TCP_DL", "if true, TCP downlink traffic will be instantiated", TCP_DL);
  cmd.AddValue("TCP_UL", "if true, TCP uplink traffic will be instantiated", TCP_UL);
  cmd.AddValue("rlc_buf_size", "size of RLC buffer [MB]", rlc_buf_size);
  cmd.AddValue("inter_pck", "inter pck interval [us]", inter_pck);
  cmd.AddValue("num_ue", "number of user equipments", num_ue);
  cmd.AddValue("num_iab", "number of IABs", num_iab);
  cmd.AddValue("num_gnb", "number of gNBs", num_gnb);
  cmd.AddValue("num_buildings_row", "number of buildings in a row of the grid", num_buildings_row);
  cmd.AddValue("num_buildings_column", "number of buildings in a column of the grid", num_buildings_column);
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

  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ScenarioEnbEnb", StringValue ("UMi-StreetCanyon"));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ScenarioEnbUe", StringValue ("UMi-StreetCanyon"));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ScenarioEnbIab", StringValue ("UMi-StreetCanyon"));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ScenarioIabIab", StringValue ("UMi-StreetCanyon"));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ScenarioIabUe", StringValue ("UMi-StreetCanyon"));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ScenarioUeUe", StringValue ("UMi-StreetCanyon"));

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
  Ptr<Node> pgw = epc_helper->GetPgwNode ();

  NodeContainer remote_host_container;
  remote_host_container.Create (1);
  Ptr<Node> remote_host = remote_host_container.Get (0);
  InternetStackHelper internet; //declared once
  internet.Install (remote_host_container);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  NetDeviceContainer internet_devices = p2p.Install (pgw, remote_host);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer ip_interfaces = ipv4.Assign (internet_devices); //0 is localhost, 1 is p2p device

  Ipv4StaticRoutingHelper ipv4_routing_helper;
  Ptr<Ipv4StaticRouting> remote_host_static_routing = ipv4_routing_helper.GetStaticRouting (remote_host->GetObject<Ipv4> ());
  remote_host_static_routing->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  //positioning the building in a grid with streets separing them
  /*

  //////////     //////////     
  //      //     //      //
  //      //     //      //
  //      //     //      //
  //////////     //////////
        

  //////////     //////////
  //      //     //      //
  //      //     //      //
  //      //     //      //
  //////////     //////////

  */
  double street_width = 10;  //meters
  double building_widthX = 50;  //meters
  double building_widthY = 50;  //meters
  double building_height = 10;  //meters
  std::vector<Ptr<Building>> building_vector;

  for(int row_index = 0; row_index < num_buildings_row; ++row_index){
    double min_y_building = row_index * (building_widthY + street_width);
    for(int col_index = 0; col_index < num_buildings_column; ++col_index){
      double min_x_building = col_index * (building_widthX + street_width);
      Ptr <Building> building;
      building = Create<Building> ();
      building->SetBoundaries (Box (min_x_building, min_x_building + building_widthX, min_y_building, min_y_building + building_widthY, 0.0, building_height));
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

  ue_nodes.Create (num_ue);
  iab_nodes.Create (num_iab);
  gnb_nodes.Create (num_gnb);

  //preliminary quantities
  double x_max = num_buildings_row * (building_widthX + street_width) - street_width;
  double y_max = num_buildings_column * (building_widthY + street_width) - street_width;
  double area = x_max * y_max;
  double gnb_height = building_height/2; //for now, the gnb is taller as half a building
  double ue_height = 1.75; //meters
  std::vector<Vector> pos_gnb;
  std::vector<Vector> pos_iab;
  std::vector<Vector> pos_ue;

  Ptr<UniformRandomVariable> coin = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> x_disc = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> x_cont = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> y_disc = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> y_cont = CreateObject<UniformRandomVariable> ();
  x_disc->SetAttribute ("Min", DoubleValue (0.0));
  x_disc->SetAttribute ("Max", DoubleValue (num_buildings_column - 1));
  x_cont->SetAttribute ("Min", DoubleValue (0.0));
  x_cont->SetAttribute ("Max", DoubleValue (x_max));
  y_disc->SetAttribute ("Min", DoubleValue (0.0));
  y_disc->SetAttribute ("Max", DoubleValue (num_buildings_row - 1));
  y_cont->SetAttribute ("Min", DoubleValue (0.0));
  y_cont->SetAttribute ("Max", DoubleValue (y_max));

  for (int m = 0; m < num_gnb; ++m) {

    if (coin->GetValue() < 0.5) { //we choose x in discrete set and y in continuous set
      double temp = ceil(x_disc->GetValue());
      double x_gnb = (temp * (building_widthX + street_width)) - street_width/2;
      double y_gnb = y_cont->GetValue();
    }
    else {  //we choose y in discrete set and x in continuous set
      double temp = ceil(y_disc->GetValue());
      double x_gnb = x_cont->GetValue ();
      double y_gnb = (temp * (building_widthY + street_width)) - street_width/2;
    }

    Vector pos = Vector(x_gnb, y_gnb, gnb_height);
    pos_iab.push_back (pos);
  } 
  
  for (int k = 0; k < num_iab; ++k) {

    if (coin->GetValue() < 0.5) { //we choose x in discrete set and y in continuous set
      double temp = ceil(x_disc->GetValue());
      double x_iab = (temp * (building_widthX + street_width)) - street_width/2;
      double y_iab = y_cont->GetValue();
    }
    else {  //we choose y in discrete set and x in continuous set
      double temp = ceil(y_disc->GetValue());
      double x_iab = x_cont->GetValue ();
      double y_iab = (temp * (building_widthY + street_width)) - street_width/2;
    }

    Vector pos = Vector(x_iab, y_iab, gnb_height);
    pos_iab.push_back (pos);
  }

  std::vector<bool> fix_dir; //if =true, the user is fixed in x, else fixed in y

  for (int n = 0; n < num_ue; ++n) {

    if (coin->GetValue() < 0.5) { //we choose x in discrete set and y in continuous set
      double temp = ceil(x_disc->GetValue());
      double x_ue = (temp * (building_widthX + street_width)) - street_width/2;
      double y_ue = y_cont->GetValue();
      fix_dir.push_back (true);
    }
    else {  //we choose y in discrete set and x in continuous set
      double temp = ceil(y_disc->GetValue());
      double x_ue = x_cont->GetValue ();
      double y_ue = (temp * (building_widthY + street_width)) - street_width/2;
      fix_dir.push_back (false);
    }

    Vector pos = Vector(x_ue, y_ue, ue_height);
    pos_ue.push_back (pos);
  }

  //NS_LOG_UNCOND("gNB "<< pos_gnb <<"\niab "<< pos_iab <<"\nue "<< pos_ue <<"\ntotalArea "<< area);

  //mobility Model (gNB)
  Ptr<ListPositionAllocator> gnb_pos_alloc = CreateObject<ListPositionAllocator> ();
  for (int m = 0; m < num_gnb; ++m) {
    gnb_pos_alloc->Add (pos_gnb[m]);
  }
  MobilityHelper gnb_mobility;
  gnb_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  gnb_mobility.SetPositionAllocator (gnb_pos_alloc);
  gnb_mobility.Install (gnb_nodes);

  //mobility Model (IAB)
  Ptr<ListPositionAllocator> iab_pos_alloc = CreateObject<ListPositionAllocator> ();
  for (int k = 0; k < num_iab; ++k) {
    iab_pos_alloc->Add (pos_iab[k]);
  }
  MobilityHelper iab_mobility;
  iab_mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  iab_mobility.SetPositionAllocator (iab_pos_alloc);
  iab_mobility.Install (iab_nodes);
  
  //mobility Model (single UE)
  Ptr<ListPositionAllocator> ue_pos_alloc = CreateObject<ListPositionAllocator> ();
  for (int n = 0; n < num_ue; ++n) {
    ue_pos_alloc->Add (pos_ue[n]);
  }
  MobilityHelper ue_mobility;
  ue_mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  ue_mobility.SetPositionAllocator (ue_pos_alloc);
  ue_mobility.Install (ue_nodes);

  for (int n = 0; n < num_ue; ++n) { //the users will start to move at time 0.5 seconds
    Vector new_speed;

    if (fix_dir[n]) {
      new_speed = Vector(0, 2, 0);
    }
    else {
      new_speed = Vector(2, 0, 0)
    }
    
    Simulator::Schedule (MilliSeconds (500), &ChangeSpeed, ue_nodes.Get (n), new_speed);
  }

  for (int s = 1; s < stop_time/0.5; ++s) {
    for (int n = 0; n < num_ue; ++n) { //the users will start to move at time 0.5 seconds
    Simulator::Schedule (MilliSeconds (s*500), &ChangePosIfNeeded, ue_nodes.Get (n), 0, x_max, 0, y_max, fix_dir[n]);
    }
  }

  //let the buildings be aware of the nodes' position
  BuildingsHelper::Install ((iab_nodes);
  BuildingsHelper::Install (ue_nodes);gnb_nodes);
  BuildingsHelper::Install 
  BuildingsHelper::MakeMobilityModelConsistent ();

  //MmWaveHelper creates relevant NetDevice instances for the different nodes
  NetDeviceContainer gnb_mm_wave_devices = mm_wave_helper->InstallEnbDevice (gnb_nodes);
  NetDeviceContainer iab_mm_wave_devices;
  iab_mm_wave_devices = mm_wave_helper->InstallIabDevice (iab_nodes);
  NetDeviceContainer ue_mm_wave_devices = mm_wave_helper->InstallUeDevice (ue_nodes);

  //create InternetStack for Ue devices and determination of default gw
  internet.Install (ue_nodes);
  Ipv4InterfaceContainer ue_ip_interfaces;
  ue_ip_interfaces = epc_helper->AssignUeIpv4Address (NetDeviceContainer (ue_mm_wave_devices));

  for (uint32_t u = 0; u < num_ue; ++u) {
    Ptr<Node> ue_node = ue_nodes.Get (u);
    Ptr<Ipv4StaticRouting> ue_static_routing = ipv4_routing_helper.GetStaticRouting (ue_node->GetObject<Ipv4> ());
    ue_static_routing->SetDefaultRoute (epc_helper->GetUeDefaultGatewayAddress (), 1);
  }

  NetDeviceContainer possibleBaseStations(gnb_mm_wave_devices, iab_mm_wave_devices);
  mm_wave_helper->ChannelInitialization (ue_mm_wave_devices, gnb_mm_wave_devices, iab_mm_wave_devices);
  mm_wave_helper->AttachIabToClosestEnb (iab_mm_wave_devices, gnb_mm_wave_devices);
  mm_wave_helper->AttachToClosestEnbWithDelay (ue_mm_wave_devices, possibleBaseStations, MilliSeconds (300));

  mm_wave_helper->ActivateDonorControllerIabSetup(gnb_mm_wave_devices, iab_mm_wave_devices,
                                                  gnb_nodes, iab_nodes);

  //applications setup in UEs and remote host
  //TODO
  
  mm_wave_helper->EnableTraces();

  //p2p.EnablePcapAll("complex_scen");

  Simulator::Stop(Seconds(stop_time));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
