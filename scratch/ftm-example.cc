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


using namespace ns3;

//Simulation parameters with their default value
int numberOfBurstsExponent = 1; //2 bursts
int burstDuration = 7; //8 ms duration
int minDeltaFtm = 10; //1000 us between frames
int partialTsfTimer = 0;
bool partialTsfNoPref = true;
bool asapCapable = false;
bool asap = true;
int ftmsPerBurst = 2;
int formatAndBandwidth = 0;
int burstPeriod = 10; //1000 ms between burst periods

int frequency = 0;
int numberOfStations = 2;
int rxGain = 0;
int propagationLossModel = 0;
int channelBandwidth = 20;
int distance = 5;

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
  ftm_params.SetNumberOfBurstsExponent(numberOfBurstsExponent); //2 bursts
  ftm_params.SetBurstDuration(burstDuration); //8 ms burst duration, this needs to be larger due to long processing delay until transmission
  ftm_params.SetMinDeltaFtm(minDeltaFtm); //100 us between frames
  ftm_params.SetPartialTsfTimer(partialTsfTimer);
  ftm_params.SetPartialTsfNoPref(partialTsfNoPref);
  ftm_params.SetAsapCapable(asapCapable);
  ftm_params.SetAsap(asap);
  ftm_params.SetFtmsPerBurst(ftmsPerBurst);
  ftm_params.SetFormatAndBandwidth(formatAndBandwidth);
  ftm_params.SetBurstPeriod(burstPeriod); //1000 ms between burst periods
    
  ftm_params.SetStatusIndication(FtmParams::RESERVED);
  ftm_params.SetStatusIndicationValue(0);


  session->SetFtmParams(ftm_params);

  session->SetSessionOverCallback(MakeCallback(&SessionOver));
  session->SessionBegin();
}

int main (int argc, char *argv[])
{
  //double rss = -80;  // -dBm

  // Parameters and their available values
  CommandLine cmd;
  cmd.AddValue ("numberOfBurstsExponent", "1 - 4", numberOfBurstsExponent);
  cmd.AddValue ("burstDuration", "6 - 11", burstDuration);
  cmd.AddValue ("minDeltaFtm", "0 - 26", minDeltaFtm);
  cmd.AddValue ("partialTsfTimer", "0 - 65535", partialTsfTimer);
  cmd.AddValue ("partialTsfNoPref", "0 or 1", partialTsfNoPref);
  cmd.AddValue ("asapCapable", "Only 0", asapCapable);
  cmd.AddValue ("asap", "0 or 1", asap);
  cmd.AddValue ("ftmsPerBurst", "0 - 7", ftmsPerBurst);
  cmd.AddValue ("formatAndBandwidth", "0 - 63", formatAndBandwidth);
  cmd.AddValue ("burstPeriod", "0 or 1", burstPeriod);
  cmd.AddValue ("frequency", "2.4 (0) or 5 (1) GHz", frequency);
  cmd.AddValue ("rxGain", "(0) - no gain, (1- 3138) - add gain", rxGain);
  cmd.AddValue ("propagationLossModel", "ThreeGpp (0) or Nakagami (1)", propagationLossModel);
  cmd.AddValue ("numberOfStations", "1 - ...", numberOfStations);
  cmd.AddValue ("channelBandwidth", "20/40/80/160 MHz", channelBandwidth);
  cmd.AddValue ("distance", "0 - ... m", distance);

  cmd.Parse (argc, argv);

  //enable FTM through attribute system
  Config::SetDefault ("ns3::RegularWifiMac::FTM_Enabled", BooleanValue(true));

  NodeContainer c;
  c.Create (numberOfStations + 1); // 1 for the AP

  WifiHelper wifi;
  
  if(!frequency){
  	wifi.SetStandard(WIFI_STANDARD_80211n_2_4GHZ);
    std::cout << "Frequency:              2.4 GHz" << std::endl;
  } else {
  	wifi.SetStandard(WIFI_STANDARD_80211n_5GHZ);
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");

  // Setup the rest of the mac
  Ssid ssid = Ssid ("wifi-default");

  // ------------------------------Setup AP -------------------------------
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
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

  //convert net device to wifi net device
  Ptr<WifiNetDevice> wifi_ap = ap->GetObject<WifiNetDevice>();

  // ------------------------------Setup STAs------------------------------
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid));
  // wifiMac.SetType ("ns3::StaWifiMac");

  NetDeviceContainer staDevices[numberOfStations];
  Ptr<ListPositionAllocator> positionAllocs [numberOfStations];
  Ptr<NetDevice> stations[numberOfStations];
  Ptr<WifiNetDevice> wifi_stations[numberOfStations];

  // setup stations.
  for (int i = 0; i < numberOfStations; i++){
    double xCoord, yCoord;
    if (i % 4 == 0) { // First station
      xCoord = distanceBetweenStations;
      yCoord = -(i / 4) * distanceBetweenStations;
    } else if (i % 4 == 1) { // Second station
      xCoord = distanceBetweenStations + (i / 4) * distanceBetweenStations;
      yCoord = distanceBetweenStations;
    } else if (i % 4 == 2) { // Third station
      xCoord = -distanceBetweenStations;
      yCoord = distanceBetweenStations + (i / 4) * distanceBetweenStations;
    } else { // Fourth station
      xCoord = -(i / 4) * distanceBetweenStations;
      yCoord = -distanceBetweenStations;
    }
    
    staDevices[i] = wifi.Install (wifiPhy, wifiMac, c.Get (i+1));
    devices.Add (staDevices[i]);
    positionAllocs[i] = CreateObject<ListPositionAllocator> ();
    positionAllocs[i]->Add (Vector (5 + i*5, 0, 0));
    mobility.SetPositionAllocator (positionAllocs[i]);
    mobility.Install(c.Get (i+1));
    stations[i] = staDevices[i].Get(0);
    wifi_stations[i] = stations[i]->GetObject<WifiNetDevice>();
  }

  std::cout << "Number of stations:     " << c.GetN() -1 << std::endl;
  std::cout << "Channel bandwidth:      " << channelBandwidth << " MHz" << std::endl;

  //enable FTM through the MAC object
//  Ptr<RegularWifiMac> ap_mac = wifi_ap->GetMac()->GetObject<RegularWifiMac>();
//  Ptr<RegularWifiMac> sta_mac = wifi_sta->GetMac()->GetObject<RegularWifiMac>();
//  ap_mac->EnableFtm();
//  sta_mac->EnableFtm();

  //load FTM map for usage
  map = CreateObject<WirelessFtmErrorModel::FtmMap> ();
  map->LoadMap ("src/wifi/ftm_map/FTM_Wireless_Error.map");
  //set FTM map through attribute system
//  Config::SetDefault ("ns3::WirelessFtmErrorModel::FtmMap", PointerValue (map));

  // Tracing
  wifiPhy.EnablePcap ("ftm-example", devices);

  for (int i = 0; i < numberOfStations; i++){
    Simulator::ScheduleNow (&GenerateTraffic, wifi_ap, wifi_stations[i], recvAddr);
  }
  // Simulator::ScheduleNow (&GenerateTraffic, wifi_ap, wifi_sta_2, recvAddr);

  //set the default FTM params through the attribute system
//  Ptr<FtmParamsHolder> holder = CreateObject<FtmParamsHolder>();
//  holder->SetFtmParams(ftm_params);
//  Config::SetDefault("ns3::FtmSession::DefaultFtmParams", PointerValue(holder));

  //set time resolution to pico seconds for the time stamps, as default is in nano seconds. IMPORTANT
  Time::SetResolution(Time::PS);

  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
