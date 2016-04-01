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
//==============================================================================
//Author: Christos Kyprianou
//Subject: Wireless Networking
//==============================================================================

//==============================================================================
//Libraries etc..
//==============================================================================
//NS3 libraries
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/channel.h"
#include "ns3/olsr-helper.h"
#include "ns3/netanim-module.h"
//Other libraries
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>

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
// Extended Version - Neiburghs of neiburghs included (5x5 grid)
// 1 test AP + 24 other, each 5m away from their neiburhg                         
// AP can take several possitions
//==============================================================================


//==============================================================================
// Fun part begins:
//==============================================================================
using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("WiFi Project");

//==============================================================================
//Function for assigning number of nodes/devices to each AP:
//==============================================================================
void NodeStruct(uint8_t nTest, uint8_t nOther, uint32_t* p){
   std::srand(std::time(0)); // use current time as seed for random generator
  //Set number of nodes for other APs
  if (nOther >5 || nOther<1) //If between 1-5 set number else set random number
    for (int i=0 ; i<9; i++)    
      p[i] = std::rand()%5 +1;
  else
    for (int i=0 ; i<9; i++) 
      p[i] = nOther;
  //Set number of nodes for Test AP
  if (nTest>3 || nTest<1) //If between 1-3 ok else set 1
    p[4] = 1;
  else
    p[4] = nTest;
  //Print number of nodes for every AP
  std::cout << "System properties: \n--------------------\n"<< std::endl; 
  std::cout << "System properties: \n--------------------\n" 
            << "Nodes/Devices per AP: " << std::endl;
  for (int i=0;i<9;i++)
    if (i==4)
      std::cout << "  AP " <<  i <<" (Test AP): " << int(p[i]) << std::endl;
    else
      std::cout << "  AP " <<  i <<": " << int(p[i]) << std::endl;
   std::cout<<"\n"<<std::endl;
}


//==============================================================================
//Main
//==============================================================================
int main (int argc, char *argv[]){
  
  //Initialize stuff
  uint32_t AP_nodes[] = {1,1,1,1,1,1,1,1,1}; //AP table: shows number of devices per AP
  int nTest = 1; //Initial number of devices on test AP
  int nOther = 1;// Inital number of devices for oteher APs
  //Wifi frequency bands: (1 channel in 5Mhz band and channels 1-13 for 2.4Ghz)
  //int freq_chans[] = {5200, 2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472}; 

  //User defined stuff
  CommandLine cmd;
  cmd.AddValue ("nTest", "Number of nodes/devics on tested AP \n Accepted values: 1-3" , nTest);
  cmd.AddValue ("nOther", "Number of nodes/devices connected on other APs \n Accepted values: 1-5 or 0:random. ", nOther);
  cmd.Parse (argc,argv);

  //Enabling some logs
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  NS_LOG_INFO ("Starting..");
  
  //Assign number of nodes to each AP:
  NodeStruct( nTest, nOther, AP_nodes);
  std::cout<<"Creating nodes.."<<std::endl; 
  //Create WiFi Nodes:
  //STA: nodes/devices per AP | AP_clan: contains all APs
  NodeContainer  STA[9], AP_clan; 
  for (int i = 0; i<9; i++){
    STA[i].Create(AP_nodes[i]);
  }
  AP_clan.Create(9); //create 9 AP nodes
  std::cout<<"Creating nodes Complete.."<<std::endl; 

//==============================================================================
  //CSMA connection between APs
  
  
  std::cout<<"Installing csma connection.."<<std::endl;
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (AP_clan);
  std::cout<<"Csma connection installed.\n\n"<<std::endl; 
  
  
  // P2P conncection between APs and main (instead of csma)
  
  /*NodeContainer  main;
  main.Create(1);
  std::cout<<"Installing p2p connection.."<<std::endl; 
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  
  NetDeviceContainer p2pDevices;
  for (int i = 0; i<9; i++){
    p2pDevices.Add(pointToPoint.Install (main.Get(0),AP_clan.Get(i)));
    std::cout<<"main to AP_"<<i<<" complete.."<<std::endl;
  }
  std::cout<<"P2p connection installed.\n\n"<<std::endl; 
  */
//==============================================================================    
  std::cout<<"Installing WiFi connections:"<<std::endl; 
  
  //PHY layer config
  std::cout<<"Configuring physical layer.."<<std::endl; 
  
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());
  
  //MAC layer config
  std::cout<<"Configuring MAC layer.."<<std::endl; 
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager"); //Rate control algorithm
  
  NqosWifiMacHelper mac = NqosWifiMacHelper::Default (); //No Qos
  Ssid ssid;
  std::string txt = "AP_";
  std::string SSIDtxt;
  
  NetDeviceContainer staDevices[9];
  NetDeviceContainer apDevices[9];

  //Setting wifi Devices
  std::cout<<"Setting WiFi devices.."<<std::endl; 
  for (int i=0;i<9;i++){
    //Set station nodes in WiFi  (each station has a different SSID)
    std::stringstream sstm;
    sstm << txt << i;
    SSIDtxt = sstm.str();
    //std::cout<<SSIDtxt<<std::endl;
    ssid = Ssid (SSIDtxt);
    //std::cout<<ssid<<std::endl;
    mac.SetType ("ns3::StaWifiMac",
                 "Ssid", SsidValue (ssid),
                 "ActiveProbing", BooleanValue (false));
    staDevices[i] = wifi.Install (phy, mac, STA[i]);

    //Set AP in WiFi
    mac.SetType ("ns3::ApWifiMac",
                 "Ssid", SsidValue (ssid),
                 "BeaconGeneration", BooleanValue (true),
                 "BeaconInterval", TimeValue(Seconds(2.5)));
    apDevices[i] = wifi.Install (phy, mac, AP_clan.Get(i)); 
  }
  std::cout<<"All WiFi stations set.. \n"<<std::endl;   
  
  //Positioning AP
  std::cout<<"Placing AP devices.."<<std::endl; 
  //===========================================================================
  //Explain:
  //Target: AP_clan (APs), Constant positions
  //Start placing at position (minX,minY)-> (5,5)
  //Every AP has distance 5 up and down and 3 AP in every row
  //===========================================================================
  MobilityHelper mobility; 
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                               "MinX", DoubleValue (5),
                               "MinY", DoubleValue (5),
                               "DeltaX", DoubleValue (5.0),
                               "DeltaY", DoubleValue (5.0),
                               "GridWidth", UintegerValue (3),
                               "LayoutType", StringValue ("RowFirst"));
  //Apply to AP
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (AP_clan); 
    
  //Positioning STA
    std::cout<<"Placing STA devices.."<<std::endl; 
  //===========================================================================
  //Explain:
  //Target: STA (per AP), moving positions
  //Place all STA next to AP ( start with highest signal quality)
  //Every STA can move in a box 4x4 around its AP
  //===========================================================================
  int minp,maxp;
  for (int i=0; i<9; i++){
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                               "MinX", DoubleValue (i*5+5),
                               "MinY", DoubleValue (i*5+5),
                               "DeltaX", DoubleValue (0.0),
                               "DeltaY", DoubleValue (0.0),
                               "GridWidth", UintegerValue (6),
                               "LayoutType", StringValue ("RowFirst"));
    maxp = 5*(i+1)+2;
    minp = 5*(i+1)-2;
    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                   "Bounds", RectangleValue (Rectangle (minp, maxp, minp, maxp)));
    mobility.Install (STA[i]); 
  }
  std::cout<<"Possitioning complete.\n\n"<<std::endl; 
  

  //Create protocol stack & assign IP address
  std::cout<<"Protocol Stack and WiFi IP assigning.."<<std::endl; 
  InternetStackHelper stack;
  
  // Enable OLSR 
  
  //std::cout<<"Enabling OLSR routing on all stations"<<std::endl;
  //OlsrHelper olsr;
  //stack.SetRoutingHelper (olsr);
 
  
  char ip_t[] = "10.1.1.0" ; 
  Ipv4AddressHelper address;
  
  Ipv4InterfaceContainer wifi_interface[9];// AP_int[9]; //Keep track of Ipv4 address
  stack.Install (AP_clan);  
  for (int i = 0; i<9; i++){
    //Protocol stack
    //stack.SetRoutingHelper (olsr);
    NodeContainer stations = (AP_clan.Get(i),STA[i]);
    stack.Install (stations);  
    //stack.SetRoutingHelper (olsr);
    //stack.Install (STA[i]);
   }
  
  //Assign IP address
  address.SetBase (ip_t, "255.255.255.0");
  for (int i = 0; i<9; i++){
    //ip_t[5] = i+1 + '0';
    //NetDeviceContainer Station(apDevices[i],staDevices[i]);
    
    wifi_interface[i] = address.Assign(apDevices[i]);
    wifi_interface[i].Add (address.Assign(staDevices[i]));
    address.NewNetwork ();
       
  }
  //Print IP addresses
  std::cout<<"IP assignment complete.. "<<" \nGenerated IPs:"<< std::endl;  
  for (int i = 0; i<9; i++){
    std::cout<<"Station "<<i<<":"<<std::endl;
    for (int j = 0; j< int(AP_nodes[i]+1); j++)
      std::cout<<"Device "<<j << ": " 
      << wifi_interface[i].GetAddress(j)<<std::endl;
  }

//==============================================================================      
  //Csma IP assigning
  
  std::cout<<"Csma IP assigning.."<<std::endl;
  address.SetBase ("10.10.0.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInt;
  csmaInt =  address.Assign (csmaDevices);    
  std::cout<<"IP assigning complete.\n\n"<<std::endl; 
  
  
  //Point to point ip address
  /*
  std::cout<<"P2p IP assigning.."<<std::endl; 
  stack.Install (main);
  address.SetBase ("10.10.0.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInt;
  p2pInterfaces = address.Assign (p2pDevices);    
  std::cout<<"IP assigning complete.\n\n"<<std::endl; 
  */  
//==============================================================================

  //Simple Echo message (check model works)
  std::cout<<"\nEstablishing UDP echo server.."<<std::endl; 
  
  //Simple udp- only   main AP
   UdpServerHelper echoServer (9);
   ApplicationContainer serverApps = echoServer.Install (AP_clan.Get(4));
   serverApps.Start (Seconds (0.0));
   serverApps.Stop (Seconds (10.0));
 
   UdpClientHelper echoClient (wifi_interface[4].GetAddress (0), 9);
   echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
   echoClient.SetAttribute ("Interval", TimeValue (Seconds(1.0))); //packets/s
   echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
 
   ApplicationContainer clientApps = echoClient.Install (STA[4]);
   clientApps.Start (Seconds (2.0));
   clientApps.Stop (Seconds (10));
  
//==============================================================================
  // MULTIPLE ECHO MSGS
  /*UdpEchoServerHelper echoServer (9);
  //Add echo client to the first device of every AP
  
  ApplicationContainer serverApps[9];
  ApplicationContainer clientApps[9];
  for (int i =0; i<9; i++){
    //serverApps[i] = echoServer.Install (AP[i].Get (0));
    serverApps[i] = echoServer.Install (AP_clan.Get (i));
    serverApps[i].Start (Seconds (1.0));
    serverApps[i].Stop (Seconds (10.0 +i));
    
    ip_t[5] = i + 1 + '0';
    std::cout<<"Server: AP_"<< i; 
      
    //UdpEchoClientHelper echoClient (Ipv4Address(ip_t), 9);
    //UdpEchoClientHelper echoClient (csmaInt.GetAddress (i), 9);
    UdpEchoClientHelper echoClient (wifi_interface[i].GetAddress (0), 9); //Client targets AP_i
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  
    clientApps[i] = echoClient.Install (STA[i].Get (AP_nodes[i]-1));
    clientApps[i].Start (Seconds (1.0+i));
    clientApps[i].Stop (Seconds (10.0+i));
    std::cout<<" / Client station "<<i<<" device "<< int(AP_nodes[i]) <<std::endl;
  }*/
//==============================================================================

  std::cout<<"UDP echo server established.\n\n"<<std::endl; 
  
//==============================================================================  
  //Create routing table
  /*
  std::cout<<"Populatng routing tables.."<<std::endl;
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  std::cout<<"Routing tables ready.\n\n"<<std::endl; 
  */
//==============================================================================
  
  //AnimationInterface anim ("mixed-wireless.xml");

  //Simultor bla bla:   
  Simulator::Stop (Seconds (25.0));
  std::cout<<"--------------------"<<std::endl; 
  std::cout<<"Starting Simulator:"<<std::endl; 
  std::cout<<"--------------------\n\n"<<std::endl; 
  
  //Generate Ascii traces:
  AsciiTraceHelper ascii;
  phy.EnableAsciiAll (ascii.CreateFileStream ("WiFitrace.tr"));
  phy.EnablePcapAll ("wifiP");
  Simulator::Run ();
  Simulator::Destroy ();
  
  
  uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApps.Get (0))->GetReceived ();
  //double throughput = totalPacketsThrough * 1024 * 8 / (25 * 1000000.0);
  
  std::cout<<"---Sent Packets: "<< totalPacketsThrough <<std::endl; 
  
 return 0;
}
