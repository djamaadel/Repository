/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/



#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/csma-module.h"
#include "ns3/log.h"
#include <ns3/ndnSIM/model/ndn-common.hpp>
#include <ns3/ndnSIM/apps/ndn-app.hpp>
#include <ns3/ndnSIM/NFD/daemon/face/face.hpp>
#include <ns3/log.h>

#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/packet.h>
#include <ns3/point-to-point-module.h>

namespace ns3 {


//{****************************FUNCTIONS DECLARATION*******************************************************
//********************************************************************************************************************		
		void logSentInterest(std::shared_ptr<const ndn::Interest> interest, ns3::Ptr<ns3::ndn::App> app,
                         std::shared_ptr<nfd::face::Face> face){
        auto id = app->GetNode()->GetId();
        std::cout<<ns3::Simulator::Now().GetSeconds()<<"  Node"<<id
		<< " Interest Sent :::  "<<interest->getName()<<" From face "<<face->getId() <<std::endl;
       }

    void logInterestReceived(std::shared_ptr<const ndn::Interest> interest, ns3::Ptr<ns3::ndn::App> app,
                            std::shared_ptr<nfd::face::Face> face){
        auto id = app->GetNode()->GetId();
        std::cout<<ns3::Simulator::Now().GetSeconds()<<"  Node"<<id
		<<" Interest Received :::  "<<"  " <<interest->getName()<<"  "<<face->getId() <<std::endl;
    }
	
	    void receiveFromNetDevice(Ptr<NetDevice> device, Ptr<const ns3::Packet> p, uint16_t protocol, const Address &from, const Address &to,
            NetDevice::PacketType packetType) {
        //checks if it's a hello message or a normal message

        auto copy = p->Copy();

        std::cout<<" MAC received packet "<< copy->GetSize() <<std::endl;

    }
	
static void StateChangeNotification (std::string context, Time now, LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState)
{
  NS_LOG_UNCOND (context << " state change at " << now.GetSeconds ()
                         << " from " << LrWpanHelper::LrWpanPhyEnumerationPrinter (oldState)
                         << " to " << LrWpanHelper::LrWpanPhyEnumerationPrinter (newState));
}



static void DataIndication (McpsDataIndicationParams params, Ptr<Packet> p)
{

	NS_LOG_UNCOND ("Dst Adrs:  "<< params.m_dstAddr <<" Received packet of size: " << p->GetSize ()
										<< " At time: " << Simulator::Now().GetSeconds()<<" From Node: "<<params.m_srcAddr);
}

static void DataConfirm (McpsDataConfirmParams params)
{
  NS_LOG_UNCOND ("LrWpanMcpsDataConfirmStatus = " << params.m_status);
}

//********************************************************************************************************************
//}********************************************************************************************************************	
int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  //Config::SetDefault("ns3::QueueBase::MaxPackets", UintegerValue(20));
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(2);

  //PointToPointHelper p2p;
  //p2p.Install(nodes.Get(0), nodes.Get(1));
  // Add and install the LrWpanNetDevice for each node
  LrWpanHelper lrwpanH;
  NetDeviceContainer lrwpanDevices = lrwpanH.Install(nodes);
  // Fake PAN association and short address assignment.
  lrwpanH.AssociateToPan (lrwpanDevices, 10);

  Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice> ();
  Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice> ();

  dev0->SetAddress (Mac16Address ("00:01"));
  dev1->SetAddress (Mac16Address ("00:02"));

  // Each device must be attached to the same channel
  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);

  dev0->SetChannel (channel);
  dev1->SetChannel (channel);
  // To complete configuration, a LrWpanNetDevice must be added to a node
  nodes.Get(0)->AddDevice (dev0);
  nodes.Get(1)->AddDevice (dev1);
  
  // Trace state changes in the phy
  dev0->GetPhy ()->TraceConnect ("TrxState", std::string ("phy0"), MakeCallback (&StateChangeNotification));
  dev1->GetPhy ()->TraceConnect ("TrxState", std::string ("phy1"), MakeCallback (&StateChangeNotification));
  
  Ptr<ConstantPositionMobilityModel> sender0Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender0Mobility->SetPosition (Vector (0,0,0));
  dev0->GetPhy ()->SetMobility (sender0Mobility);
  Ptr<ConstantPositionMobilityModel> sender1Mobility = CreateObject<ConstantPositionMobilityModel> ();
  // Configure position 10 m distance
  sender1Mobility->SetPosition (Vector (0,10,0));
  dev1->GetPhy ()->SetMobility (sender1Mobility);
  //
  McpsDataConfirmCallback cb0;
  cb0 = MakeCallback (&DataConfirm);
  dev0->GetMac ()->SetMcpsDataConfirmCallback (cb0);

  McpsDataIndicationCallback cb1;
  cb1 = MakeCallback (&DataIndication);
  dev0->GetMac ()->SetMcpsDataIndicationCallback (cb1);

  McpsDataConfirmCallback cb2;
  cb2 = MakeCallback (&DataConfirm);
  dev1->GetMac ()->SetMcpsDataConfirmCallback (cb2);

  McpsDataIndicationCallback cb3;
  cb3 = MakeCallback (&DataIndication);
  dev1->GetMac ()->SetMcpsDataIndicationCallback (cb3);
  
  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();


  // Choosing forwarding strategy
  ns3::ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

  // Installing applications

  // Consumer
  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("1")); // 10 interests a second
  auto consumers = consumerHelper.Install(nodes.Get(0));// first node
  // Producer
  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/prefix%");
  producerHelper.SetAttribute("PayloadSize", StringValue("64"));
  
  auto producers =producerHelper.Install(nodes.Get(1)); // last node
  Ptr<Node> producer = nodes.Get(1);      
  producer->RegisterProtocolHandler(MakeCallback(&receiveFromNetDevice),
                                          ndn::L3Protocol::ETHERNET_FRAME_TYPE, producer->GetDevice(0), true);

  
  Simulator::Stop(Seconds(0.5));

  //{********************RESULTS*****************************************************
  //*************************************************************************************
  //Ptr<ns3::ndn::App> apps = CreateObject<ns3::ndn::App> ();
  //apps->GetObject(0).TraceConnectWithoutContext("TransmittedInterests", ns3::MakeCallback(logSentInterest));
   consumers.Get(0)->TraceConnectWithoutContext("TransmittedInterests", ns3::MakeCallback(logSentInterest));
   producers.Get(0)->TraceConnectWithoutContext("ReceivedInterests", ns3::MakeCallback(logInterestReceived));
  //ns3::ndn::L3RateTracer::InstallAll("results/rate-trace.txt", Seconds(0.3));
  //L2RateTracer::InstallAll("results/drop-trace.txt", Seconds(0.3));
  
  //AsciiTraceHelper ascii;
  //lrwpanH.EnableAsciiAll (ascii.CreateFileStream ("./results/ndn-lr-wpan-simle.tr"));
  //lrwpanH.EnablePcapAll (std::string ("ndn-lr-wpan-simle"), true);
  //*************************************************************************************
  //}*************************************************************************************
  Simulator::Run();
  Simulator::Destroy();
////////////////////////////////////
	//////////////////////////////////////
  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
