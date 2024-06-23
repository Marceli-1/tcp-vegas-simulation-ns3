#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-vegas.h"
#include <vector>
#include <fstream>
#include <iomanip>

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
  std::vector<uint64_t> fileSizes = {1, 100, 1000, 10000, 100000, 1000000}; // in MB
  uint32_t packetSize = 1040; // in bytes
  uint32_t runs = 10000; // Reduced number of runs to 10,000

  std::ofstream resultsFile;
  resultsFile.open("results.txt");

  LogComponentEnable ("VegasSimulation", LOG_LEVEL_INFO);

  for (uint32_t alpha : alphas)
  {
    for (uint32_t beta : betas)
    {
      for (uint64_t fileSize : fileSizes)
      {
        double totalThroughputSum = 0;
        uint32_t nPackets = (fileSize * 1024 * 1024) / packetSize;

        for (uint32_t run = 0; run < runs; ++run)
        {
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

          // Adding packet loss rate to the channel
          Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
          em->SetAttribute ("ErrorRate", DoubleValue (0.01)); // 1% packet loss
          devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

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
            totalThroughput += iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds () - iter->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024;
          }

          totalThroughputSum += totalThroughput;

          Simulator::Destroy ();
        }

        double averageThroughput = totalThroughputSum / runs;
        NS_LOG_INFO ("Alpha: " << alpha << " Beta: " << beta << " FileSize: " << fileSize << "MB Average Throughput: " << averageThroughput << " Mbps");
        resultsFile << std::fixed << std::setprecision(6)
                    << "Alpha: " << alpha << " Beta: " << beta << " FileSize: " << fileSize << "MB "
                    << "Average Throughput: " << averageThroughput << " Mbps" << std::endl;
      }
    }
  }

  resultsFile.close();

  return 0;
}
