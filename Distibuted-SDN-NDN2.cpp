


#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/openflow-module.h"
#include "ns3/log.h"

#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"


//using namespace ns3;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Adding OpenFlowCsmaSwitchs to an NDN Senario");

//ns3::PacketMetadata::Enable();

bool verbose = false;
bool use_drop = false;
ns3::Time timeout = ns3::Seconds (0);

bool
SetVerbose (std::string value)
{
  verbose = true;
  return true;
}

bool
SetDrop (std::string value)
{
  use_drop = true;
  return true;
}

bool
SetTimeout (std::string value)
{
  try {
      timeout = ns3::Seconds (atof (value.c_str ()));
      return true;
    }
  catch (...) { return false; }
  return false;
}

int
main (int argc, char *argv[])
{
  
  #ifdef NS3_OPENFLOW
  //
  // Allow the user to override any of the defaults and the above Bind() at
  // run-time, via command-line arguments
  CommandLine cmd;
  cmd.AddValue ("v", "Verbose (turns on logging).", MakeCallback (&SetVerbose));
  cmd.AddValue ("verbose", "Verbose (turns on logging).", MakeCallback (&SetVerbose));
  cmd.AddValue ("d", "Use Drop Controller (Learning if not specified).", MakeCallback (&SetDrop));
  cmd.AddValue ("drop", "Use Drop Controller (Learning if not specified).", MakeCallback (&SetDrop));
  cmd.AddValue ("t", "Learning Controller Timeout (has no effect if drop controller is specified).", MakeCallback ( &SetTimeout));
  cmd.AddValue ("timeout", "Learning Controller Timeout (has no effect if drop controller is specified).", MakeCallback ( &SetTimeout));

  cmd.Parse (argc, argv);
  

  if (verbose)
    {
      LogComponentEnable ("OpenFlowCsmaSwitchExample", LOG_LEVEL_INFO);
      LogComponentEnable ("OpenFlowInterface", LOG_LEVEL_INFO);
      LogComponentEnable ("OpenFlowSwitchNetDevice", LOG_LEVEL_INFO);
    }

  
  // Create the nodes required by the topology:
  
  NS_LOG_INFO ("Create nodes.");
  NodeContainer terminals;
  terminals.Create (13); // 2 Producers & 2 Consumers and 9 NDN Node

  NodeContainer SwitchContainer;
  SwitchContainer.Create (3); //  3 OF Controllers

  NS_LOG_INFO ("Build Topology");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (1000000));
  
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  // the priviose two lines are similar to :
       // setting default parameters for PointToPoint links and channels
       //Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
       //Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));

  // Create the csma links, from each producers to Controller 0
  NetDeviceContainer terminalDevices;
  NetDeviceContainer switchDevices;
  
  AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-tree-Distribute.txt");
  topologyReader.Read();
  
     // Getting containers for the consumer/producer Pairs and the three Distributed controllers
  Ptr<Node> consumer1 = Names::Find<Node>("leaf-1");
  Ptr<Node> consumer2 = Names::Find<Node>("leaf-2");
  Ptr<Node> producer1 = Names::Find<Node>("leaf-3");
  Ptr<Node> producer2 = Names::Find<Node>("leaf-4");
  Ptr<Node> switchNode1 = Names::Find<Node>("Cont0"); // The controller Cont0 node
  Ptr<Node> switchNode2= Names::Find<Node>("Cont1");  // The controller Cont1 node
  Ptr<Node> switchNode3= Names::Find<Node>("Cont2");  // The controller Cont2 node
   
  // Create the switch netdevice, which will do the packet switching (Install OpenFlow )
  
  OpenFlowSwitchHelper swtch1;
  OpenFlowSwitchHelper swtch2;
  OpenFlowSwitchHelper swtch3;

  if (use_drop)
    {
      Ptr<ns3::ofi::DropController> controller1 = CreateObject<ns3::ofi::DropController> ();
      swtch1.Install (switchNode1, switchDevices, controller1);
      
      Ptr<ns3::ofi::DropController> controller2 = CreateObject<ns3::ofi::DropController> ();
      swtch2.Install (switchNode2, switchDevices, controller2);
      
      Ptr<ns3::ofi::DropController> controller3 = CreateObject<ns3::ofi::DropController> ();
      swtch3.Install (switchNode3, switchDevices, controller3);
    }
  else
    {
      Ptr<ns3::ofi::LearningController> controller1 = CreateObject<ns3::ofi::LearningController> ();
      if (!timeout.IsZero ()) controller1->SetAttribute ("ExpirationTime", TimeValue (timeout));
      swtch1.Install (switchNode1, switchDevices, controller1);
      
      Ptr<ns3::ofi::LearningController> controller2 = CreateObject<ns3::ofi::LearningController> ();
      if (!timeout.IsZero ()) controller2->SetAttribute ("ExpirationTime", TimeValue (timeout));
      swtch2.Install (switchNode2, switchDevices, controller2);
      
      Ptr<ns3::ofi::LearningController> controller3 = CreateObject<ns3::ofi::LearningController> ();
      if (!timeout.IsZero ()) controller3->SetAttribute ("ExpirationTime", TimeValue (timeout));
      swtch2.Install (switchNode3, switchDevices, controller3);
    }

  // Add internet stack to the terminals
  //InternetStackHelper internet;
  //internet.Install (terminals);
  
  // Install NDN stack
    ndn::StackHelper ndnHelper;
    ndnHelper.SetDefaultRoutes(true);
    ndnHelper.InstallAll();
    
   // Create NDN applications (The Consumers)

  // on the first consumer node install a Consumer application
  // that will express interests in /example/data1 namespace
  
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute("Frequency", StringValue("100"));    // 100  interests per second
  consumerHelper.SetAttribute("LifeTime", TimeValue(Seconds(100.0)));
  consumerHelper.SetPrefix("/example/data1");
  ApplicationContainer consumerApps1 = consumerHelper.Install(consumer1); // Install Consumer1 on sender1

  // on the second consumer node install a Consumer application
  // that will express interests in /example/data2 namespace
  consumerHelper.SetPrefix("/example/data2");
  ApplicationContainer consumerApps2 = consumerHelper.Install(consumer2); // Install Consumer2 on sender2


// Create NDN applications (The Producers)
    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetAttribute("PayloadSize", StringValue("102400"));    //100 MB payload. the unit here is kB
    

  // Register /example/data1 prefix with global routing controller and
  // install producer that will satisfy Interests in /example/data1 namespace
  ndnGlobalRoutingHelper.AddOrigins("/example/data1", producer1);
  producerHelper.SetPrefix("/example/data1"); 
  producerHelper.Install(producer1); // Install Producer1 on receiver1

  // Register /example/data2 prefix with global routing controller and
  // install producer that will satisfy Interests in /example/data2 namespace
  ndnGlobalRoutingHelper.AddOrigins("/example/data2", producer2);
  producerHelper.SetPrefix("/example/data2");
  producerHelper.Install(producer2); // Install Producer2 on receiver2


    apps1.Stop(Seconds(100.0)); 

    // Choosing forwarding strategy
   
    ndn::StrategyChoiceHelper::InstallAll("/example/data", "/localhost/nfd/strategy/multicast");
    
    ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
    ndnGlobalRoutingHelper.InstallAll();
    

// Metrics:
    
    ndn::AppDelayTracer::InstallAll ("Distributed-Delays-trace.txt"); //Delay
    
    L2RateTracer::InstallAll("Distributed-drop-trace.txt", Seconds(0.5)); //packet drop rate (overflow)
  //
  // Also configure some tcpdump traces; each interface will be traced.
  // The output files will be named:
  //     openflow-switch-<nodeId>-<interfaceId>.pcap
  // and can be read by the "tcpdump -r" command (use "-tt" option to
  // display timestamps correctly)
  //
  csma.EnablePcapAll ("openflow-switch", false);

  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(100.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  #else
  NS_LOG_INFO ("NS-3 OpenFlow is not enabled. Cannot run simulation.");
  #endif // NS3_OPENFLOW
  
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}




