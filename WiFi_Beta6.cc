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
 */
 
 //Information
//==============================================================================
//Author: Christos Kyprianou
//Subject: Wireless Networking
//Note: This project is based on NS3 tutorial3
//==============================================================================
//==============================================================================
// Network Information:
//---------------------
// Multiple AP´s, not connected together. 
// AP Nodes:
// ----Test AP: 1-3 nodes (given by user)
// ----Rest: 1-5 nodes (given by user: all the same or random)
// Data link:
// ----Test AP: 2,5,10,56 Mbps (Choise by user)
// ----Rest: 2,5,10,56 Mbps (all same or random) 
// Channel Frequencies:
// ----Test AP: 13 channels on 2Ghz band and 1 channel in 5 Ghz (choise by user)
// ----Rest: 13 channels on 2Ghz band (set by user - gausian distribution)
//==============================================================================
// Simple test - Only close neiburghs test (3x3 grid)
// 1 test AP + 9 other, each 5m away from their neiburhg (test AP middle)
//==============================================================================
//    (-1,1)  /----5m----/ (0,1)    /----5m----/ (1,1)    
// AP0--(1-5 nodes) |   AP1--(1-5 nodes)  |   AP2--(1-5 nodes)
//       ---                ---                  ---
//        |                  |                    |
//        5m                5m                   5m
//        |                  |                    |
//       ---                ---                  ---
//     (-1,0) /----5m----/ (0,0)  /----5m----/  (1,0)    
// AP3--(1-5 nodes) |AP4(Test)--(1-3 nodes) |   AP5--(1-5 nodes)
//       ---                ---                  ---
//        |                  |                    |
//        5m                5m                   5m
//        |                  |                    |
//       ---                ---                  ---                
//    (-1,-1) /----5m----/ (0,-1)  /----5m----/ (1,-1)              
// AP6--(1-5 nodes) |  AP7--(1-3 nodes)  |  AP8--(1-5 nodes)      
//==============================================================================
// Extended Version (flexible model) - MxN grid
// M = raws
// N = netSize/raws
// 1 test AP + netSize other, each 5m away from their neiburhg                         
// AP can take several possitions
//==============================================================================

//Include libraries
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/random-variable-stream.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <math.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Beta6");

/*
********************************************************************************
void CourseChange (std::string context, Ptr<const MobilityModel> model)
{
  Vector position = model->GetPosition ();
  NS_LOG_UNCOND (context <<
    " x = " << position.x << ", y = " << position.y);
}
********************************************************************************
*/

//Main starts here
int main (int argc, char *argv[])
{
  //Variables:
  //============================================================================
  //Logs and tracing
  bool verbose = true; //Enable log
  bool tracing = true; //Enable tracing
  //Device stuff
  uint32_t nWifi = 5;  //number of wifi devices in non-test APs
  uint8_t nTest = 3;   //Number of wifi devices for tested Ap
  uint8_t testAp = 4;  //Number of tested AP
  int netSize = 9;     //Number of total APs
  int rows = sqrt(netSize);        //Number of APs per row (5m difference)
  //Channel
  uint32_t channels[] =
     {1,1,1,1,13,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}; //Channels for each AP
  //Packet transmission
  int maxPackets = 5;
  int PackSize = 1024;
  double PackInterval = 0.2;
  
  //============================================================================
  
  //User inputs
  //============================================================================
  CommandLine cmd;
  cmd.AddValue ("netSize", "Number of APs in system", netSize);
  cmd.AddValue ("nWifi", "Number of wifi STA devices per AP", nWifi);
  cmd.AddValue ("nTest", "Number of wifi STA devices on tested AP", nTest);
  cmd.AddValue ("testAP", "Select test AP (0-8)", testAp);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.Parse (argc,argv);
  //============================================================================
  
  //Start program
  std::cout<<"Beta 6 init.."<<std::endl;
  // No more than 250 wifi nodes!
  if (nWifi > 250|| nTest > 250)
    {
      std::cout << "Too many wifi nodes, no more than 250 each." << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
    
  //Create nodes:
  //============================================================================
  NodeContainer wifiStaNodes[netSize];
  NodeContainer wifiApNode[netSize];
  for(int i = 0; i<netSize; i++)
    {
      if (i!=testAp)
        wifiStaNodes[i].Create (nWifi);
      else
        wifiStaNodes[i].Create (nTest);
      wifiApNode[i].Create(1);
    }
  std::cout<<"Nodes created.."<<std::endl;
  //============================================================================
  
  //WiFi & PHY channel
  //============================================================================
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());
  //wifi stations
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
  //SSid
  Ssid ssid;
  std::string txt = "Beta_";
  std::string SSIDtxt;
  //Devics
  NetDeviceContainer staDevices[netSize];
  NetDeviceContainer apDevices[netSize];
  for(int i = 0; i<netSize; i++)
    {
      //Set channel frequency
      phy.Set ("ChannelNumber",UintegerValue(channels[i]));
      //Set station nodes in WiFi  (each station has a different SSID)
      std::stringstream sstm;
      sstm << txt << i;
      SSIDtxt = sstm.str();
      ssid = Ssid (SSIDtxt);
      //Set STA wifi
      mac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));
      staDevices[i] = wifi.Install (phy, mac, wifiStaNodes[i]);
      //Set AP wifi
      mac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
      apDevices[i] = wifi.Install (phy, mac, wifiApNode[i]);
    }
  std::cout<<"WiFi stations set.."<<std::endl;
  //============================================================================
  
  //Placing devices AP-AP 5m dist, STA-STA 1m dist
  //============================================================================
  MobilityHelper mobility;
  for(int i = 0; i<netSize; i++)
    {
      int x = 5*(i%rows);
      int y = 5*(int(i/rows));
      mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (x),
                                     "MinY", DoubleValue (y),
                                     "DeltaX", DoubleValue (1.0),
                                     "DeltaY", DoubleValue (1.0),
                                     "GridWidth", UintegerValue (5),
                                   "LayoutType", StringValue ("RowFirst"));
      //Place AP node
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (wifiApNode[i]);
      //Place STA nodes
      mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                    "Bounds", RectangleValue (Rectangle (x-5, x+5, y-5, y+5)));
      mobility.Install (wifiStaNodes[i]);
      std::cout<<"Station "<<i<<" placed at: "<<x<<","<<y <<std::endl;
    }
  std::cout<<"All devices are in place.."<<std::endl;
  //============================================================================
  
  //Stack protocol & IP assign
  //============================================================================
  InternetStackHelper stack;
  for(int i = 0; i<netSize; i++)
    {
      stack.Install (wifiApNode[i]);
      stack.Install (wifiStaNodes[i]);
    }
  //Assign IP
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer wifiAPint[netSize];
  address.SetBase ("10.1.1.0", "255.255.255.0");
  for(int i = 0; i<netSize; i++)
    { 
      wifiAPint[i] = address.Assign (apDevices[i]);
      address.Assign (staDevices[i]);
      address.NewNetwork ();
    }
  std::cout<<"All devices are given IP.."<<std::endl;
  //============================================================================
  
  //UDP echo
  //============================================================================
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps;
  ApplicationContainer clientApps;
  for(int i = 0; i<netSize; i++)
    {
      //Set server
      serverApps.Add( echoServer.Install (wifiApNode[i].Get(0)));
      serverApps.Start (Seconds (1.0));
      serverApps.Stop (Seconds (20.0));
      //Packet info
      UdpEchoClientHelper echoClient (wifiAPint[i].GetAddress (0), 9);
      echoClient.SetAttribute ("MaxPackets", UintegerValue (maxPackets));
      echoClient.SetAttribute ("Interval", TimeValue (Seconds (PackInterval)));
      echoClient.SetAttribute ("PacketSize", UintegerValue (PackSize));
      //Set client (all devices)
      clientApps.Add( echoClient.Install (wifiStaNodes[i]));//.Get (nWifi - 1));
      clientApps.Start (Seconds (2.0));
      clientApps.Stop (Seconds (20.0));
    }
  //============================================================================
  
  //Simulation
  //============================================================================
  //Set Stop time
  Simulator::Stop (Seconds (20.0));
  //Enable tracing
  if (tracing == true)
    {
      AsciiTraceHelper ascii;
      phy.EnableAsciiAll (ascii.CreateFileStream ("Beta6.tr"));
      phy.EnablePcap ("Beta6", apDevices[testAp].Get (0));
    }
  /*
  ******************************************************************************
  //Print possitions
  std::ostringstream oss;
  oss <<
    "/NodeList/" << wifiStaNodes.Get (nWifi - 1)->GetId () <<
    "/$ns3::MobilityModel/CourseChange";
  Config::Connect (oss.str (), MakeCallback (&CourseChange));
  ******************************************************************************
  */
 	//Set flow monitor (part modified from RizqiWifi project)
	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  //Start simulation
  std::cout<<"--------------------"<<std::endl; 
  std::cout<<"Starting Simulator:"<<std::endl; 
  std::cout<<"--------------------\n\n"<<std::endl; 
  Simulator::Run ();
  Simulator::Destroy ();
  //Calculate througput and Delay
  //============================================================================
  //Save logs into file:
  std::ofstream myfile;
  myfile.open ("BetaLog.txt");
  //Threshold counters
  double th = 0;    //Current throughput
  double maxTh = 0; //Max recorded throughput
  double minTh = 9999; //Min recorded throughput
  double avThz = 0;  //Average throughput (including zero)
  double avThnz = 0;  //Average throughput (non zero)
  //Packet counters
  int countz = 0;    //Flow counter
  int countnz = 0;    //Flow counter
  int lostPack, total_lostPack; //lost packages
  double av_lostPack = 0; //Average lost packages
  //Delay counters
  Time del;
  Time maxDel = Seconds(0.0);  //Max recorded delay
  Time minDel = Seconds(100); //Min recorded delay
  Time avDel = Seconds(0.0);   //Average delay
  //Tested AP Counters
  Time tmaxDel = Seconds(0.0);  //Max recorded delay 
  Time tminDel = Seconds(100); //Min recorded delay
  Time tavDel = Seconds(0.0);   //Average delay
  int count_test = 0;   //Counter,
  int test_lostPack;    // Lost packages
  double tmaxTh = 0;   //Max recorded throughput
  double tminTh = 9999; //Min recorded throughput
  double tavThz = 0;    //Average throughput (including zero)

  //Begin
  monitor->CheckForLostPackets ();
	Ptr<Ipv4FlowClassifier> classifier =
	  DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  //Average throughput and delay per flow:
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
	         stats.begin (); i != stats.end (); ++i)
	  {
	    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
	    //Print flow number/info
	    myfile << "Flow " << i->first << " (" << t.sourceAddress << " -> "
	       << t.destinationAddress << ")\n";
	    //print packet status
	    myfile << "  Tx Bytes: " << i->second.txBytes << "\n";
	    myfile << "  Rx Bytes: " << i->second.rxBytes << "\n";
	    th = i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() 
	      - i->second.timeFirstTxPacket.GetSeconds())/1024/nWifi;
	    myfile << "  Average Throughput: " << th<< "kbps\n";
	    //Sum up total average througput calculate max and min
	    avThz = avThz + th;
      //Calculate delay (check if packet was sent first!)
      lostPack = maxPackets - i->second.rxPackets;
      total_lostPack = total_lostPack + lostPack;
      myfile << "  Lost Packets: " << lostPack << std::endl; 
	    if (i->second.rxPackets != 0)
	      {
	        //Delay measuring
	        del = i->second.delaySum / i->second.rxPackets;
	        avDel = avDel + del;
    	    if (maxDel < del)
    	      maxDel = del;
     	    if (minDel > del)
    	      minDel = del;
	        myfile << "  Delay : " << del << "\n";
          //Threshold measuring
    	    if (maxTh < th)
    	      maxTh = th;
     	    if (minTh > th)
    	      minTh = th;
    	    avThnz = avThnz + th;
    	    countnz++;
        }
	    else
	      myfile << "  Delay : N/A" << std::endl; 
	    countz++;
	    //Check for test AP status
	    if (t.sourceAddress == "10.1.5.1" || t.destinationAddress == "10.1.5.1")
	      {
	       myfile << "\nFOUND: \n----------" << count_test<< std::endl;
	        count_test++; //update counter
	        test_lostPack = test_lostPack + lostPack;
	        if (lostPack != maxPackets)
	          {   
	            tavDel = tavDel + del;
        	    if (tmaxDel < del)
        	      tmaxDel = del;
         	    if (tminDel > del)
        	      tminDel = del;
        	    if (tmaxTh < th)
        	      tmaxTh = th;
         	    if (tminTh > th)
        	      tminTh = th;
        	    tavThz = tavThz + th;
        	  }
	      } 
	  }
	//General data  
  myfile << "\nGeneral: \n----------" << std::endl; 
	myfile << "\nPackets: \n----------" << std::endl; 	    
	myfile << "Average packet loss: " << av_lostPack/countz<< "\n"; 
  myfile << "\nThroughput: \n----------" << std::endl; 
	myfile << "Total average througput: " << avThz/countz<< "kbps\n"; 
  myfile << "Total non-zero average througput: " << avThnz/countnz<< "kbps\n"; 
  myfile << "Max recorded througput: " << maxTh<< "kbps\n"; 
  myfile << "Min recorded througput: " << minTh<< "kbps\n"; 
  myfile << "Total througput z: " << avThz<< "kbps\n"; 
  myfile << "\nDelay: \n----------" << std::endl; 	  
  myfile << "Max recorded delay: " << maxDel<< "\n"; 
  myfile << "Min recorded delay: " << minDel<< "\n"; 
  myfile << "Average delay: " << avDel/countnz<< "\n"; 
 	//Print for test AP
 	myfile << "\nTest AP: \n----------" << std::endl; 
 	myfile << "Average packet loss: " << test_lostPack/count_test<< "\n"; 
  myfile << "Total average througput: " << tavThz/count_test<< "kbps\n"; 
  myfile << "Max recorded througput: " << tmaxTh<< "kbps\n"; 
  myfile << "Min recorded througput: " << tminTh<< "kbps\n"; 
  myfile << "Total througput z: " << tavThz<< "kbps\n"; 
  myfile << "Max recorded delay: " << tmaxDel<< "\n"; 
  myfile << "Min recorded delay: " << tminDel<< "\n"; 
 	myfile << "Average delay: " << tavDel/count_test<< "\n\n EOF\n";	
 		
  //============================================================================ 
	//End of program  
  return 0;
}
//Null
