


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
  terminals.Create (4); // 2 Producers & 2 Consumers 

  NodeContainer switchDevices;
  switchDevices.Create (9); //  9 NDN Nodes

  NS_LOG_INFO ("Build Topology");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (1000000));
  
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
 
 
  NetDeviceContainer terminalDevices;
  NetDeviceContainer switchDevices;
  
  AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-tree-Distribute3.txt");
  topologyReader.Read();
  
     // Getting containers for the consumer/producer Pairs and the three Distributed controllers
  Ptr<Node> consumer1 = Names::Find<Node>("leaf-1");
  Ptr<Node> consumer2 = Names::Find<Node>("leaf-2");
  Ptr<Node> producer1 = Names::Find<Node>("leaf-3");
  Ptr<Node> producer2 = Names::Find<Node>("leaf-4");
  Ptr<Node> consumer3 = Names::Find<Node>("leaf-5");
  Ptr<Node> consumer4 = Names::Find<Node>("leaf-6");
  Ptr<Node> producer5 = Names::Find<Node>("leaf-7");
  Ptr<Node> producer6 = Names::Find<Node>("leaf-8");
  Ptr<Node> producer7 = Names::Find<Node>("leaf-9");
  Ptr<Node> producer3 = Names::Find<Node>("leaf-10");
  Ptr<Node> producer4 = Names::Find<Node>("leaf-11");
  Ptr<Node> consumer5 = Names::Find<Node>("leaf-12");
  Ptr<Node> consumer6 = Names::Find<Node>("leaf-13");
  Ptr<Node> consumer7 = Names::Find<Node>("leaf-14");

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
      swtch3.Install (switchNode3, switchDevices, controller3);
    }


  
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

  // on the other consumer nodes install a Consumer applications
  // that will express interests in /example/data 2,3,4,5,6,7 namespaces
  consumerHelper.SetPrefix("/example/data2");
  ApplicationContainer consumerApps2 = consumerHelper.Install(consumer2); // Install Consumer2 on sender2
// consumer3 will ask for /example/data3/1 namespace
  //consumerHelper.SetPrefix("/example/data3/1");  
// consumer3 will ask for /example/data3 namespace
  consumerHelper.SetPrefix("/example/data3");
  ApplicationContainer consumerApps3 = consumerHelper.Install(consumer3);
// consumer4 will also ask for /example/data3/2 namespace from producer3
  //consumerHelper.SetPrefix("/example/data3/2");
  //ApplicationContainer consumerApps4 = consumerHelper.Install(consumer4);
// consumer4 will also ask for /example/data4 namespace from producer4
  consumerHelper.SetPrefix("/example/data4");
  ApplicationContainer consumerApps4 = consumerHelper.Install(consumer4);
// consumer5 will ask for /example/data5 namespace
  consumerHelper.SetPrefix("/example/data5");
  ApplicationContainer consumerApps5 = consumerHelper.Install(consumer5);
// consumer6 will ask for /example/data6 namespace
  consumerHelper.SetPrefix("/example/data6");
  ApplicationContainer consumerApps6 = consumerHelper.Install(consumer6);
// consumer7 will ask for /example/data7 namespace
  consumerHelper.SetPrefix("/example/data7");
  ApplicationContainer consumerApps7 = consumerHelper.Install(consumer7);


// Create NDN applications (The Producers)
    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetAttribute("PayloadSize", StringValue("102400"));    //100 MB payload. the unit here is kB
    

  // Register /example/data1 prefix with global routing controller and
  // install producer that will satisfy Interests in /example/data1 namespace
  ndnGlobalRoutingHelper.AddOrigins("/example/data1", producer1);
  producerHelper.SetPrefix("/example/data1"); 
  producerHelper.Install(producer1); // Install Producer1 on receiver1

// install producer2 that will satisfy Interests in /example/data2 namespace
  ndnGlobalRoutingHelper.AddOrigins("/example/data2", producer2);
  producerHelper.SetPrefix("/example/data2");
  producerHelper.Install(producer2); // Install Producer2 
// install producer3 that will satisfy Interests in /example/data3 or /example/data3/1 and /example/data3/2 namespaces
  //ndnGlobalRoutingHelper.AddOrigins("/example/data3/1", producer3);
  //ndnGlobalRoutingHelper.AddOrigins("/example/data3/2", producer3);
  ndnGlobalRoutingHelper.AddOrigins("/example/data3", producer3);
  //producerHelper.SetPrefix("/example/data3/1");
  //producerHelper.SetPrefix("/example/data3/2");
  producerHelper.SetPrefix("/example/data3");
  producerHelper.Install(producer3); 
// install producer4 that will satisfy Interests in /example/data4 namespace
  ndnGlobalRoutingHelper.AddOrigins("/example/data4", producer4);
  producerHelper.SetPrefix("/example/data4");
  producerHelper.Install(producer4); 
// install producer5 that will satisfy Interests in /example/data5 namespace
  ndnGlobalRoutingHelper.AddOrigins("/example/data5", producer5);
  producerHelper.SetPrefix("/example/data5");
  producerHelper.Install(producer5); 
// install producer6 that will satisfy Interests in /example/data6 namespace
  ndnGlobalRoutingHelper.AddOrigins("/example/data6", producer6);
  producerHelper.SetPrefix("/example/data6");
  producerHelper.Install(producer6); 
// install producer7 that will satisfy Interests in /example/data7 namespace
  ndnGlobalRoutingHelper.AddOrigins("/example/data7", producer7);
  producerHelper.SetPrefix("/example/data7");
  producerHelper.Install(producer7); 



    consumerApps1.Start(Seconds(1.0));
    consumerApps2.Start(Seconds(1.0));
    consumerApps3.Start(Seconds(1.0));
    consumerApps4.Start(Seconds(1.0));
    consumerApps5.Start(Seconds(1.0));
    consumerApps6.Start(Seconds(1.0));
    consumerApps7.Start(Seconds(1.0));
    consumerApps1.Stop(Seconds(100.0));
    consumerApps2.Stop(Seconds(100.0));
    consumerApps3.Stop(Seconds(100.0));
    consumerApps4.Stop(Seconds(100.0));
    consumerApps5.Stop(Seconds(100.0));
    consumerApps6.Stop(Seconds(100.0));
    consumerApps7.Stop(Seconds(100.0));


    // Choosing forwarding strategy
   
    ndn::StrategyChoiceHelper::InstallAll("/example/data", "/localhost/nfd/strategy/multicast");
    
    ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
    ndnGlobalRoutingHelper.InstallAll();
    

// Metrics:
    
    ndn::AppDelayTracer::InstallAll ("Distributed-Delays-trace3.txt"); //Delay
    
    L2RateTracer::InstallAll("Distributed-drop-trace3.txt", Seconds(0.5)); //packet drop rate (overflow)
 
 
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
  NS_LOG_INFO ("NS-3 OpenFlow NDN is not enabled. Cannot run simulation.");
  #endif // NS3_OPENFLOW
  
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}




