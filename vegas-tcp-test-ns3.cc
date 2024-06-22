#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-vegas.h"
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("VegasSimulation");

class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

int 
main (int argc, char *argv[])
{
  std::vector<uint32_t> alphas = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  std::vector<uint32_t> betas = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  std::vector<uint32_t> fileSizes = {5, 10, 100, 1000}; // in MB
  uint32_t packetSize = 1040; // in bytes

  LogComponentEnable ("VegasSimulation", LOG_LEVEL_INFO);

  for (uint32_t alpha : alphas)
  {
    for (uint32_t beta : betas)
    {
      for (uint32_t fileSize : fileSizes)
      {
        uint32_t nPackets = (fileSize * 1024 * 1024) / packetSize;
        
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
        Config::SetDefault ("ns3::TcpVegas::Alpha", UintegerValue (alpha));
        Config::SetDefault ("ns3::TcpVegas::Beta", UintegerValue (beta));

        NodeContainer nodes;
        nodes.Create (2);

        PointToPointHelper pointToPoint;
        pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
        pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

        NetDeviceContainer devices;
        devices = pointToPoint.Install (nodes);

        InternetStackHelper stack;
        stack.Install (nodes);

        Ipv4AddressHelper address;
        address.SetBase ("10.1.1.0", "255.255.255.0");

        Ipv4InterfaceContainer interfaces = address.Assign (devices);

        uint16_t sinkPort = 8080;
        Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));
        PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
        ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
        sinkApps.Start (Seconds (0.));
        sinkApps.Stop (Seconds (20.));

        Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

        Ptr<MyApp> app = CreateObject<MyApp> ();
        app->Setup (ns3TcpSocket, sinkAddress, packetSize, nPackets, DataRate ("1Mbps"));
        nodes.Get (0)->AddApplication (app);
        app->SetStartTime (Seconds (1.));
        app->SetStopTime (Seconds (20.));

        FlowMonitorHelper flowmon;
        Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

        Simulator::Stop (Seconds (20));
        Simulator::Run ();

        // Print throughput and other metrics
        Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
        FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

        double totalThroughput = 0;
        for (auto iter = stats.begin (); iter != stats.end (); ++iter)
        {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);
          NS_LOG_INFO ("Alpha: " << alpha << " Beta: " << beta << " FileSize: " << fileSize << "MB");
          NS_LOG_INFO ("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
          NS_LOG_INFO ("Tx Packets = " << iter->second.txPackets);
          NS_LOG_INFO ("Rx Packets = " << iter->second.rxPackets);
          NS_LOG_INFO ("Duration: " << (iter->second.timeLastRxPacket.GetSeconds () - iter->second.timeFirstTxPacket.GetSeconds ()));
          NS_LOG_INFO ("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds () - iter->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024 << " Mbps");

          totalThroughput += iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds () - iter->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024;
        }

        NS_LOG_INFO ("Total Throughput: " << totalThroughput << " Mbps");

        Simulator::Destroy ();
      }
    }
  }

  return 0;
}
