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

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Beta5");

/*
================================================================================
void CourseChange (std::string context, Ptr<const MobilityModel> model)
{
  Vector position = model->GetPosition ();
  NS_LOG_UNCOND (context <<
    " x = " << position.x << ", y = " << position.y);
}
================================================================================
*/

//Main starts here
int main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nWifi = 3;
  uint8_t nTest = 3;
  uint8_t testAp = 4;
  int netSize = 9;
  bool tracing = true;
  //User inputs
  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices per AP", nWifi);
  cmd.AddValue ("nTest", "Number of wifi STA devices on tested AP", nTest);
  cmd.AddValue ("testAP", "Select test AP (0-8)", testAp);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.Parse (argc,argv);
  //Start program
  std::cout<<"Beta 5 init.."<<std::endl;
  // No more than 250 wifi nodes!
  if (nWifi > 250|| nTest > 250)
    {
      std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
  //Create nodes:
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
  //PHY channel
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
  //Placing devices AP-AP 5m dist, STA-STA 1m dist
  MobilityHelper mobility;
  for(int i = 0; i<netSize; i++)
    {
      int x = 5*(i%3);
      int y = 5*(int(i/3));
      mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (x),
                                     "MinY", DoubleValue (y),
                                     "DeltaX", DoubleValue (1.0),
                                     "DeltaY", DoubleValue (1.0),
                                     "GridWidth", UintegerValue (4),
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
  //Stack protocol
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
  //UDP echo
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps;
  ApplicationContainer clientApps;
  for(int i = 0; i<netSize; i++)
    {
      //Set server
      serverApps.Add( echoServer.Install (wifiApNode[i].Get(0)));
      serverApps.Start (Seconds (1.0));
      serverApps.Stop (Seconds (10.0));
      //Packet info
      UdpEchoClientHelper echoClient (wifiAPint[i].GetAddress (0), 9);
      echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
      echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
      echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
      //Set client (all devices)
      clientApps.Add( echoClient.Install (wifiStaNodes[i]));//.Get (nWifi - 1));
      clientApps.Start (Seconds (2.0));
      clientApps.Stop (Seconds (10.0));
    }
  //Start simulation
  Simulator::Stop (Seconds (10.0));
  //Enable tracing
  if (tracing == true)
    {
      phy.EnablePcap ("beta2", apDevices[0].Get (0));
    }
  /*
  ==============================================================================
  //Print possitions
  std::ostringstream oss;
  oss <<
    "/NodeList/" << wifiStaNodes.Get (nWifi - 1)->GetId () <<
    "/$ns3::MobilityModel/CourseChange";
  Config::Connect (oss.str (), MakeCallback (&CourseChange));
  ==============================================================================
  */
  std::cout<<"--------------------"<<std::endl; 
  std::cout<<"Starting Simulator:"<<std::endl; 
  std::cout<<"--------------------\n\n"<<std::endl; 
  Simulator::Run ();
  Simulator::Destroy ();
  //Count sent packets
  //uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApps.Get (0))->GetReceived ();
  //double throughput = totalPacketsThrough * 1024 * 8 / (25 * 1000000.0);
  //std::cout<<"---Sent Packets: "<< totalPacketsThrough <<std::endl; 
  return 0;
}
