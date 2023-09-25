/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
 *
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
 *
 */
/*
 * Based on the "wifi-simple-infra.cc" example.
 * Modified by Christos Laskos.
 * 2022
 *
 * For the purpose of this thesis modified by Krzysztof Kurczab as part of a master's thesis at the AGH University of Kraków.
 * Changed to being based on "wifi-simple-adhoc.cc" example.
 * These features were added:
 *  - Changed wifiMac type from infrastruce to Ad-hoc, in order to get rid off unnecessary traffic
 *  - Arp cache population
 *  - All the command line arguments
 *  - New, detailed outputs
 *  - Automated creations of stations
 *  - The placement of the stations in a circle around the AP
 *  - The time seperation of the stations' starting time
 * 2023
 */

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/ftm-header.h"
#include "ns3/mgt-headers.h"
#include "ns3/ftm-error-model.h"
#include "ns3/pointer.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-helper.h"
#include "ns3/arp-cache.h"
#include "ns3/object-factory.h"
#include "ns3/arp-cache.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/object-vector.h"
#include "ns3/ipv4-interface.h"
#include "ns3/net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/packet.h"
#include "ns3/node-list.h"

#include <iostream>
#include <vector>
#include <math.h>
#include <string>
#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sys/stat.h>
#include <chrono>  // For high resolution clock
#include <cstdlib>

using namespace ns3;
// --- Populate ARP cache ---
void PopulateARPcache () {
    Ptr<ArpCache> arp = CreateObject<ArpCache> ();
    arp->SetAliveTimeout (Seconds (3600 * 24 * 365) );

    for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
        Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT (ip !=0);
        ObjectVectorValue interfaces;
        ip->GetAttribute ("InterfaceList", interfaces);

        for (ObjectVectorValue::Iterator j = interfaces.Begin (); j != interfaces.End (); j++)
        {
            Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
            NS_ASSERT (ipIface != 0);
            Ptr<NetDevice> device = ipIface->GetDevice ();
            NS_ASSERT (device != 0);
            Mac48Address addr = Mac48Address::ConvertFrom (device->GetAddress () );

            for (uint32_t k = 0; k < ipIface->GetNAddresses (); k++)
            {
                Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal();
                if (ipAddr == Ipv4Address::GetLoopback ())
                    continue;

                ArpCache::Entry *entry = arp->Add (ipAddr);
                Ipv4Header ipv4Hdr;
                ipv4Hdr.SetDestination (ipAddr);
                Ptr<Packet> p = Create<Packet> (100);
                entry->MarkWaitReply (ArpCache::Ipv4PayloadHeaderPair (p, ipv4Hdr));
                entry->MarkAlive (addr);
            }
        }
    }

    for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
        Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT (ip !=0);
        ObjectVectorValue interfaces;
        ip->GetAttribute ("InterfaceList", interfaces);

        for (ObjectVectorValue::Iterator j = interfaces.Begin (); j != interfaces.End (); j ++)
        {
            Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
            ipIface->SetAttribute ("ArpCache", PointerValue (arp) );
        }
    }
}

// Simulation parameters with their default value
int numberOfBurstsExponent = 1; //2 bursts
int burstDuration = 11; //2 ms
int minDeltaFtm = 640; //time between frames [100 us]
int partialTsfTimer = 0;
bool partialTsfNoPref = true;
bool asapCapable = false;
bool asap = true;
int ftmsPerBurst = 2;
int formatAndBandwidth = 0;
int burstPeriod = 1; //time between burst periods [100 ms]

int frequency = 1;
int numberOfStations = 1;
int rxGain = 0;
int propagationLossModel = 0;
int channelBandwidth = 20;
int distance = 5;

std::string pcapPath = "ftm-example";

NS_LOG_COMPONENT_DEFINE ("FtmExample");

void SessionOver (FtmSession session)
{
  NS_LOG_UNCOND ("RTT: " << session.GetMeanRTT ());
  // std::cout << "\nIndivudual RTT: " << std::endl;
  // for (const double& strength : session.GetIndividualRTT()) { //GetIndividualSignalStrength
  //       std::cout << strength << std::endl;
  // }
  std::cout << "\nFTM params: " << session.GetFtmParams() << std::endl;
  std::cout << "\nMean RTT [ps]: " << session.GetMeanRTT() << std::endl;
  std::cout << "Mean Signal Strength [dBm]: " << session.GetMeanSignalStrength () << std::endl;
  std::cout << "Number of Measurements: " << session.GetIndividualRTT().size() << std::endl;
}

Ptr<WirelessFtmErrorModel::FtmMap> map;

static void GenerateTraffic (Ptr<WifiNetDevice> ap, Ptr<WifiNetDevice> sta, Address recvAddr)
{
  Ptr<RegularWifiMac> sta_mac = sta->GetMac()->GetObject<RegularWifiMac>();
 
  Mac48Address to = Mac48Address::ConvertFrom (recvAddr);

  Ptr<FtmSession> session = sta_mac->NewFtmSession(to);
  if (session == 0)
    {
      NS_FATAL_ERROR ("ftm not enabled");
    }

  //create the wired error model
//  Ptr<WiredFtmErrorModel> wired_error = CreateObject<WiredFtmErrorModel> ();
//  wired_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);

  //create wireless error model
  //map has to be created prior
//  Ptr<WirelessFtmErrorModel> wireless_error = CreateObject<WirelessFtmErrorModel> ();
//  wireless_error->SetFtmMap(map);
//  wireless_error->SetNode(sta->GetNode());
//  wireless_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);

  //create wireless signal strength error model
  //map has to be created prior
  Ptr<WirelessSigStrFtmErrorModel> wireless_sig_str_error = CreateObject<WirelessSigStrFtmErrorModel> ();
  wireless_sig_str_error->SetFtmMap(map);
  wireless_sig_str_error->SetNode(sta->GetNode());
  switch(channelBandwidth){
  	case 20:
      wireless_sig_str_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);
      // std::cout << "Channel Bandwidth:      20 MHz" << std::endl;
      break;
    case 40:
	    wireless_sig_str_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_40_MHz);
      // std::cout << "Channel Bandwidth:      40 MHz" << std::endl;
      break;
    case 80:
	    wireless_sig_str_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_80_MHz);
      // std::cout << "Channel Bandwidth:      80 MHz" << std::endl;
      break;
    case 160:
	    wireless_sig_str_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_160_MHz);
      // std::cout << "Channel Bandwidth:      160 MHz" << std::endl;
      break;
  }

  //using wired error model in this case
  session->SetFtmErrorModel(wireless_sig_str_error);

  //create the parameter for this session and set them
  FtmParams ftm_params;  
  ftm_params.SetStatusIndication(FtmParams::RESERVED);
  ftm_params.SetStatusIndicationValue(0);

  ftm_params.SetNumberOfBurstsExponent(numberOfBurstsExponent);
  ftm_params.SetBurstDuration(burstDuration);
  ftm_params.SetMinDeltaFtm(minDeltaFtm);
  ftm_params.SetPartialTsfTimer(partialTsfTimer);
  ftm_params.SetPartialTsfNoPref(partialTsfNoPref);
  ftm_params.SetAsapCapable(asapCapable);
  ftm_params.SetAsap(asap);
  ftm_params.SetFtmsPerBurst(ftmsPerBurst);
  ftm_params.SetFormatAndBandwidth(formatAndBandwidth);
  ftm_params.SetBurstPeriod(burstPeriod);

  session->SetFtmParams(ftm_params);

  session->SetSessionOverCallback(MakeCallback(&SessionOver));
  session->SessionBegin();
}

int main (int argc, char *argv[])
{
  //double rss = -80;  // -dBm

  // Parameters and their available values
  CommandLine cmd;
  cmd.AddValue ("numberOfBurstsExponent", "1 - 8", numberOfBurstsExponent);
  cmd.AddValue ("burstDuration", "2 - 11", burstDuration);
  cmd.AddValue ("minDeltaFtm", "1 - ...", minDeltaFtm);
  cmd.AddValue ("partialTsfTimer", "0 - 65535", partialTsfTimer);
  cmd.AddValue ("partialTsfNoPref", "0 or 1", partialTsfNoPref);
  cmd.AddValue ("asapCapable", "Only 0", asapCapable);
  cmd.AddValue ("asap", "0 or 1", asap);
  cmd.AddValue ("ftmsPerBurst", "1 - ...", ftmsPerBurst);
  cmd.AddValue ("formatAndBandwidth", "0", formatAndBandwidth);
  cmd.AddValue ("burstPeriod", "1 - ...", burstPeriod);
  cmd.AddValue ("frequency", "2.4 (0) or 5 (1) GHz", frequency);
  cmd.AddValue ("rxGain", "(0) - no gain, (1) - add gain", rxGain);
  cmd.AddValue ("propagationLossModel", "ThreeGpp (0) or Nakagami (1)", propagationLossModel);
  cmd.AddValue ("numberOfStations", "1 - ...", numberOfStations);
  cmd.AddValue ("channelBandwidth", "20 / 40 / 80 / 160 MHz", channelBandwidth);
  cmd.AddValue ("distance", "0 - ... m", distance);
  cmd.AddValue ("pcapPath", "---", pcapPath);

  cmd.Parse (argc, argv);

  //enable FTM through attribute system
  Config::SetDefault ("ns3::RegularWifiMac::FTM_Enabled", BooleanValue(true));

  NodeContainer c;
  c.Create (numberOfStations + 1); // 1 for the AP

  WifiHelper wifi;
  
  if(!frequency){
  	wifi.SetStandard (WIFI_STANDARD_80211n_2_4GHZ);
    std::cout << "Frequency:              2.4 GHz" << std::endl;
  } else {
  	wifi.SetStandard (WIFI_STANDARD_80211n_5GHZ);
    std::cout << "Frequency:              5 GHz" << std::endl;
  }
  YansWifiPhyHelper wifiPhy;
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (rxGain) );
  std::cout << "Rx Gain:                " << rxGain << std::endl;
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  //wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));

  if(!propagationLossModel){
  	wifiChannel.AddPropagationLoss ("ns3::ThreeGppIndoorOfficePropagationLossModel");
    std::cout << "Propagation Loss Model: ThreeGppIndoorOffice" << std::endl;
  } else{ 
	  wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
    std::cout << "Propagation Loss Model: Nakagami" << std::endl;
  }
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.
  MobilityHelper mobility;

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager"
  //,                                "RtsCtsThreshold", StringValue ("200")
                                ); // so as to force RTS/CTS for data frames

  // Setup the rest of the mac
  Ssid ssid = Ssid ("wifi-default");

  // --- Setup AP ---
  wifiMac.SetType ("ns3::AdhocWifiMac");
  // ,                   "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, c.Get (0));
  NetDeviceContainer devices = apDevice;

  // AP (0)
  Ptr<ListPositionAllocator> positionAlloc_AP = CreateObject<ListPositionAllocator> ();
  positionAlloc_AP->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc_AP);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c.Get(0));
  
  Ptr<NetDevice> ap = apDevice.Get(0);

  Address recvAddr = ap->GetAddress();
  std::cout << "AP address: " << recvAddr << std::endl;
  //convert net device to wifi net device
  Ptr<WifiNetDevice> wifi_ap = ap->GetObject<WifiNetDevice>();

  // --- Setup STAs ---
  wifiMac.SetType ("ns3::AdhocWifiMac");
                  //  "Ssid", SsidValue (ssid));
                  //  ,"ActiveProbing", BooleanValue (false));
  // wifiMac.SetType ("ns3::StaWifiMac");

  NetDeviceContainer staDevices[numberOfStations];
  Ptr<ListPositionAllocator> positionAllocs [numberOfStations];
  Ptr<NetDevice> stations[numberOfStations];
  Ptr<WifiNetDevice> wifi_stations[numberOfStations];
  Address sta[numberOfStations];

  // setup stations.
  for (int i = 0; i < numberOfStations; i++){
     // Set up the stations in a circle around the AP
    double angle = 2 * M_PI * i / numberOfStations;
    double xCoord = distance * cos(angle);
    double yCoord = distance * sin(angle);
    if ((xCoord > -9.18485e-10 && xCoord < 0) || (xCoord < 9.18485e-10 && xCoord > 0)){
      xCoord = 0.0;
    }
    if ((yCoord > -9.18485e-10 && yCoord < 0) || (yCoord < 9.18485e-10 && yCoord > 0)){
      yCoord = 0.0;
    }
    std::cout << "Station " << i + 1 << ": " << "(" << xCoord << ", " << yCoord << ")" << std::endl;

    staDevices[i] = wifi.Install (wifiPhy, wifiMac, c.Get (i+1));
    devices.Add (staDevices[i]);
    positionAllocs[i] = CreateObject<ListPositionAllocator> ();
    positionAllocs[i]->Add (Vector (xCoord, yCoord, 0));
    mobility.SetPositionAllocator (positionAllocs[i]);
    mobility.Install(c.Get (i+1));
    stations[i] = staDevices[i].Get(0);
    wifi_stations[i] = stations[i]->GetObject<WifiNetDevice>();
    
    sta[i] = stations[i]->GetAddress();
    std::cout << "Station " << i + 1 << " address: " << sta[i] << std::endl;
  }

  std::cout << "Number of stations:     " << c.GetN() - 1 << std::endl;
  std::cout << "Channel bandwidth:      " << channelBandwidth << " MHz" << std::endl;

  // --- Populate ARP cache ---
  InternetStackHelper stack;
  stack.Install (c.Get(0));
  for(int i = 0; i < numberOfStations; i++){
	  stack.Install (c.Get(i + 1));
  }

  Ipv4AddressHelper address;

  for(int i = 0; i < 1; i++){
	  std::string addrString;
	  addrString =  "10.1." + std::to_string(i) + ".0";
	  const char *cstr = addrString.c_str(); //convert to constant char
	  address.SetBase (Ipv4Address(cstr), "255.255.255.0");
    if(i == 0){
	    address.Assign (apDevice.Get(i));
    }
    address.Assign (staDevices[i]);
  }
  
  PopulateARPcache ();

  //enable FTM through the MAC object
//  Ptr<RegularWifiMac> ap_mac = wifi_ap->GetMac()->GetObject<RegularWifiMac>();
//  Ptr<RegularWifiMac> sta_mac = wifi_sta->GetMac()->GetObject<RegularWifiMac>();
//  ap_mac->EnableFtm();
//  sta_mac->EnableFtm();

  //load FTM map for usage
  map = CreateObject<WirelessFtmErrorModel::FtmMap> ();
  map->LoadMap ("src/wifi/ftm_map/50x50.map"); // [-25, 25]
  //set FTM map through attribute system
//  Config::SetDefault ("ns3::WirelessFtmErrorModel::FtmMap", PointerValue (map));

  // Tracing
  wifiPhy.EnablePcap (pcapPath, devices);

  for (int i = 0; i < numberOfStations; i++){
    // Simulator::ScheduleNow (&GenerateTraffic, wifi_ap, wifi_stations[i], recvAddr);
    Simulator::Schedule (Seconds(i * 200), &GenerateTraffic, wifi_ap, wifi_stations[i], recvAddr);
  }

  //set the default FTM params through the attribute system
//  Ptr<FtmParamsHolder> holder = CreateObject<FtmParamsHolder>();
//  holder->SetFtmParams(ftm_params);
//  Config::SetDefault("ns3::FtmSession::DefaultFtmParams", PointerValue(holder));

  //set time resolution to pico seconds for the time stamps, as default is in nano seconds. IMPORTANT
  Time::SetResolution(Time::PS);

  Simulator::Stop (Seconds (100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
