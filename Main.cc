#include <fstream>
#include <cstdlib>
#include <bits/stdc++.h>
#include <string.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
//#include "drop-tail-queue.h"

using namespace ns3;
using namespace std;

typedef uint32_t uint;
#define ERROR 0.000001
NS_LOG_COMPONENT_DEFINE ("main1");

map<string, double> mapBytesReceivedIPV4, mapMaxThroughput;  //Global Containers
double printGap = 0;

uint max(uint a,uint b) // Functions to find max
{
	if(a<b)
		return a;
	return b;
}


int main (int argc, char *argv[])
{

    uint32_t maxBytes = 0;
	uint32_t port;
	uint32_t packetsize = 1024;
	uint32_t run_time = 1;
	uint32_t for_loop = 1;
	uint32_t offset=0;

	bool simultaneously = false;
    string prot = "TcpHighSpeed";
	
	CommandLine cmd;
    cmd.AddValue ("maxBytes", "Total number of bytes for application to send", maxBytes);
    cmd.AddValue ("packetsize", "Total number of bytes for application to send", packetsize);
    cmd.AddValue ("prot", "Transport protocol to use:TcpHighSpeed, TcpVegas, TcpScalable", prot);
    cmd.AddValue ("for_loop", "no of for loop runs", for_loop);
    cmd.AddValue ("run_time", "run_time in factor of 5", run_time);
    cmd.AddValue ("simultaneously", "run_time in factor of 5", simultaneously);
    cmd.AddValue ("offset", "offset for different start times", offset);

    cmd.Parse (argc, argv);

    cout<<"Command line Arguments:\n";		
	cout<<"packetsize: "<<packetsize<<"\n";
	cout<<"prot: "<<prot<<"\n";
	cout<<"run time: "<<run_time<<"\n";
	cout<<"for_loop: "<<for_loop<<"\n";
	cout<<"Offset: "<<offset<<"\n";
	cout<<"simultaneously: "<<simultaneously<<"\n"<<endl;
	cout<<"**************************************************"<<endl;
	cout<<endl;
		
	
    if (prot.compare ("TcpScalable") == 0)
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpScalable::GetTypeId ()));
    }
    else if (prot.compare ("TcpVegas") == 0)
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
    }
    else if (prot.compare ("TcpHighSpeed") == 0)
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHighSpeed::GetTypeId ()));
    }
    else
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
    }

    Gnuplot2dDataset dataset_udp;	//Datasets used for Gunplot in future
	Gnuplot2dDataset dataset_tcp;
    Gnuplot2dDataset dataset_udp_delay;
	Gnuplot2dDataset dataset_tcp_delay;
	
	uint i=0;// Main LOOP
	while(i<for_loop)
	{		
		i++;
		uint32_t udpPacketSize= packetsize+100*i;  
		uint32_t tcpPacketSize= udpPacketSize;  

		NodeContainer c; // we aee creating six nodes representing devices in our network
		c.Create (6);
		NodeContainer n0n2 = NodeContainer (c.Get (0), c.Get (2));
		NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get (2));
		NodeContainer n2n3 = NodeContainer (c.Get (2), c.Get (3));
		NodeContainer n3n4 = NodeContainer (c.Get (3), c.Get (4));
		NodeContainer n3n5 = NodeContainer (c.Get (3), c.Get (5));


		InternetStackHelper internet;	//  installing internet stack in all nodes
		internet.Install (c);


		uint32_t queueSizeHR = (80000*20)/(udpPacketSize*8);
		uint32_t queueSizeRR = (30000*100)/(udpPacketSize*8);
		string queueSizeHR2=to_string(queueSizeHR)+"p";
		string queueSizeRR2=to_string(queueSizeRR)+"p";


		
		PointToPointHelper p2p; // Point to point helper is used to create p2p links between nodes
		
		// Setting paramaters oof links btw routers and hosts.
		p2p.SetDeviceAttribute ("DataRate", StringValue ("80Mbps"));
		p2p.SetChannelAttribute ("Delay", StringValue ("20ms"));
	    p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue (QueueSize (queueSizeHR2)));

		NetDeviceContainer d0d2 = p2p.Install (n0n2);
		NetDeviceContainer d1d2 = p2p.Install (n1n2);
		NetDeviceContainer d3d4 = p2p.Install (n3n4);
		NetDeviceContainer d3d5 = p2p.Install (n3n5);

		// Setting paramaters of links btw router to router 
		p2p.SetDeviceAttribute ("DataRate", StringValue ("30Mbps"));
		p2p.SetChannelAttribute ("Delay", StringValue ("100ms"));
	    p2p.SetQueue ("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue (QueueSize (queueSizeRR2)));

		NetDeviceContainer d2d3 = p2p.Install (n2n3);

		// Error model for handling any possible error it reports error to output screen.
		Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
	    em->SetAttribute ("ErrorRate", DoubleValue (ERROR));
	    d3d4.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
	    d3d5.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

		
		Ipv4AddressHelper ipv4;
		ipv4.SetBase ("10.1.0.0", "255.255.255.0");
		Ipv4InterfaceContainer i0i2 = ipv4.Assign (d0d2);
		
		ipv4.SetBase ("10.1.1.0", "255.255.255.0");
		Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2);

		ipv4.SetBase ("10.1.3.0", "255.255.255.0");
		Ipv4InterfaceContainer i2i3 = ipv4.Assign (d2d3);

		ipv4.SetBase ("10.1.4.0", "255.255.255.0");
		Ipv4InterfaceContainer i3i4 = ipv4.Assign (d3d4);

		ipv4.SetBase ("10.1.5.0", "255.255.255.0");
		Ipv4InterfaceContainer i3i5 = ipv4.Assign (d3d5);

		
		// Routing Tables 
		Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

		cout << "Assigned IPs to Receivers:" << endl;  
		cout<<"H3: "<<i3i4.GetAddress(1)<<endl;
		cout<<"H4: "<<i3i5.GetAddress(1)<<endl;

		cout << "Assigned IPs to Senders:" << endl;
		cout<<"H1: "<< i0i2.GetAddress(0)<<endl;
		cout<<"H2: "<< i1i2.GetAddress(0)<<endl;
		
		cout << "Assigned IPs to Router:" << endl;
		cout<<"R2<------------H3: "<<i3i4.GetAddress(0)<<endl;
		cout<<"R2<------------H4: "<<i3i5.GetAddress(0)<<endl;
		cout<<"R1<------------H1: "<< i0i2.GetAddress(1)<<endl;
		cout<<"R1<------------H2: "<< i1i2.GetAddress(1)<<endl;
		cout<<"R1<------------R2: "<< i2i3.GetAddress(0)<<endl;
		cout<<"R1------------>R2: "<< i2i3.GetAddress(1)<<endl;
		cout<<endl;
		

		// Printing routing tables for the all the nodes in the container
		Ipv4GlobalRoutingHelper g;
		Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("routing.routes", ios::out);
		g.PrintRoutingTableAllAt (Seconds (2), routingStream);

		
				
											// CBR traffic on UDP
			
		
		port = 9;  

		// On off helper is for CBR traffic, we tell INET socket address here that receiver is HOST-3
		OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (i3i4.GetAddress (1), port)));

		// onoff.SetConstantRate (DataRate ("10000kb/s"),udpPacketSize);
	    onoff.SetAttribute ("PacketSize", UintegerValue (udpPacketSize));

		//install the on off app on HOST-1 and run for 1-5 seconds
		ApplicationContainer udp_apps_s = onoff.Install (n0n2.Get (0));
		
		//runtime =  (total time of simulation in multiple of 10 for given packet size) 
		
		if(simultaneously==false)
		{
			udp_apps_s.Start (Seconds ( (0.0+(10*i))*run_time  ) );
			udp_apps_s.Stop (Seconds ((5.0+(10* i))*run_time) );			
		}
		else if(simultaneously==true)
		{
			udp_apps_s.Start (Seconds ( (0.0+(10*i))*run_time  ) );
			udp_apps_s.Stop (Seconds ((10.0+(10*i))*run_time) );
		}

		PacketSinkHelper sink_udp ("ns3::UdpSocketFactory",Address (InetSocketAddress (Ipv4Address::GetAny (), port))); 	// Creating a packet sink to receive these packets from any ip address. 
		ApplicationContainer udp_apps_d = sink_udp.Install (n3n4.Get (1)); // Installing the reciver at HOST-3
		
		if(simultaneously==false)
		{
			udp_apps_d.Start (Seconds ((0.0+(10*i))*run_time) );
			udp_apps_d.Stop (Seconds ((5.0+(10*i))*run_time) );			
		}
		else if(simultaneously==true)
		{
			udp_apps_d.Start (Seconds ((0.0+(10*i))*run_time) );
			udp_apps_d.Stop (Seconds ((10.0+(10*i))*run_time) );			
		}


			
										//FTP traffic on TCP
			
		

	    port = 12344; // Create a BulkSendApplication and install it on HOST 2
	    BulkSendHelper source ("ns3::TcpSocketFactory",InetSocketAddress (i3i5.GetAddress (1), port));

	    // Set the amount of data to send in bytes.  Zero is unlimited.
	    source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
		source.SetAttribute ("SendSize", UintegerValue (tcpPacketSize));

	    ApplicationContainer tcp_apps_s = source.Install (n1n2.Get (0));
	    
	    if(simultaneously==false)
		{
			tcp_apps_s.Start (Seconds ((5.0+(10*i))*run_time) );
	    	tcp_apps_s.Stop (Seconds ((10.0+(10*i))*run_time) );
		}
		else if(simultaneously==true)
		{
			tcp_apps_s.Start (Seconds ((0.0+(10*i))*run_time) );
	    	tcp_apps_s.Stop (Seconds ((10.0+(10*i))*run_time) );	
		}

	    
	    // Create a PacketSinkApplication and install it on HOST-4
	    PacketSinkHelper sink_tcp ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
	    ApplicationContainer tcp_apps_d = sink_tcp.Install (n3n5.Get (1));

	    if(simultaneously==false)
		{
			tcp_apps_d.Start (Seconds ((5.0+(10*i))*run_time) );
	    	tcp_apps_d.Stop (Seconds ((10.0+(10*i))*run_time) );
		}
		else if(simultaneously==true)
		{
			tcp_apps_d.Start (Seconds ((0.0+(10*i))*run_time+offset ));
	    	tcp_apps_d.Stop (Seconds ((10.0+(10*i))*run_time+offset) );	
		}
	    

				               //LOGGING of PARAMETERS
		

		// Monitoring the FLOW
		Ptr<FlowMonitor> flowmon;
		FlowMonitorHelper flowmonHelper;
		flowmon = flowmonHelper.InstallAll();

		if(!simultaneously)
		Simulator::Stop(Seconds((10+(10*i))*run_time) );
		else if(simultaneously)
		Simulator::Stop(Seconds((10+(10*i))*run_time+offset) );

		Simulator::Run();
		flowmon->CheckForLostPackets();

		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());

		map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats(); // Retrieving flow montor stats for different flows
		
		double throughput_udp;
		double throughput_tcp;
		double delay_udp;
		double delay_tcp;
		
		for (auto i = stats.begin(); i != stats.end(); ++i) 
		{
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

			cout << "FLOW: " << i->first << "\n" << 
			"Source ADRESS: " << t.sourceAddress << "\n" << 
			"Destination ADRESS: " << t.destinationAddress << "\n";


			if(t.sourceAddress == "10.1.0.1") {	
				//UDP FLOW
				throughput_udp = (double)i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstRxPacket.GetSeconds ()) / (double)1000;
				delay_udp = (double)i->second.delaySum.GetSeconds()/(i->second.rxPackets) ;

				dataset_udp.Add (udpPacketSize,throughput_udp);
				dataset_udp_delay.Add (udpPacketSize,delay_udp);

				cout << "UDP Flow over CBR " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n" << 
				"Tx Packets: " << i->second.txPackets << "\n" << 
				"Tx Bytes:" << i->second.txBytes << "\n" << 
				"Rx Packets: " << i->second.rxPackets << "\n" << 
				"Rx Bytes:" << i->second.rxBytes << "\n" << 
				"Net Packet Lost: " << i->second.lostPackets << "\n" << 
				"Lost due to droppackets: " << i->second.packetsDropped.size() << "\n" << 
				"Total Delay(in seconds): " << i->second.delaySum.GetSeconds() << endl << 
				"Mean Delay(in seconds): " << (double)i->second.delaySum.GetSeconds()/(i->second.rxPackets) << endl << 
				"Offered Load: " << (double)i->second.txBytes * 8.0 / (i->second.timeLastTxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) 
				/ (double)1000 << " Kbps" << endl << 
				"Throughput: " << (double)i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstRxPacket.GetSeconds()) 
				/ (double)1000 << " Kbps" << endl << "Mean jitter:" << (double)i->second.jitterSum.GetSeconds() / (i->second.rxPackets - 1) << endl;
			} 
			else if(t.sourceAddress == "10.1.1.1") {	
				//TCP FLOW
				throughput_tcp = (double)i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstRxPacket.GetSeconds ()) / 1000;
				delay_tcp = (double)i->second.delaySum.GetSeconds()/(i->second.rxPackets);
				
				dataset_tcp.Add (tcpPacketSize,throughput_tcp);
				dataset_tcp_delay.Add(tcpPacketSize,delay_tcp);

				cout << prot << " Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n" 
				<< "Tx Packets: " << i->second.txPackets << "\n" << 
				"Tx Bytes:" << i->second.txBytes << "\n" << 
				"Rx Packets: " << i->second.rxPackets << "\n" << 
				"Rx Bytes:" << i->second.rxBytes << "\n" <<
				"Net Packet Lost:	" << i->second.lostPackets << "\n" << 
				"Lost due to droppackets: " << i->second.packetsDropped.size()
				<< "\n" << "Total Delay(in seconds): " << i->second.delaySum.GetSeconds() << endl << 
				"Mean Delay(in secinds): " << (double)i->second.delaySum.GetSeconds()/(i->second.rxPackets) << endl << 
				"Offered Load: " << (double)i->second.txBytes * 8.0 / (i->second.timeLastTxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) 
				/ (double)1000 << " Kbps" << endl <<
				"Throughput: " << (double)i->second.rxBytes * 8.0 /
				(i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstRxPacket.GetSeconds()) / (double)1000 <<  " Kbps" << endl << 
				"Mean jitter:" << (double)i->second.jitterSum.GetSeconds() / (i->second.rxPackets - 1) << endl;

			}
		}

		cout<<"Run: "<<i<<" finished\n"<<endl;
		cout<<"XXXXXXXX=============XXXXXXXX=============XXXXXXXX"<<endl;
		Simulator::Destroy ();
	}

	string simultaneously_str="Seperate";
	if(simultaneously&&offset==0)
		simultaneously_str="Simultaneous_Same_Start";
	else if(simultaneously&&offset!=0)
		simultaneously_str="Simultaneous_Different_Start";

	string fileNameWithNoExtension = prot+"_throughput_"+simultaneously_str;
	string graphicsFileName        = fileNameWithNoExtension + ".pdf";
	string plotFileName            = fileNameWithNoExtension + ".plt";
	string plotTitle               = prot + " vs UDP throughput";
	
	string fileNameWithNoExtension_delay = prot+"_delay_"+simultaneously_str;
	string graphicsFileName_delay        = fileNameWithNoExtension_delay + ".pdf";
	string plotFileName_delay            = fileNameWithNoExtension_delay + ".plt";
	string plotTitle_delay               = prot + " vs UDP delay";

	Gnuplot plot (graphicsFileName); // Instantiate the plot and set its title.
	Gnuplot plot_delay (graphicsFileName_delay);
	
	plot.SetTitle (plotTitle);
	plot_delay.SetTitle (plotTitle_delay);

	// Make the graphics file, which the plot file will create when it
	// is used with Gnuplot, be a PDF file
	plot.SetTerminal ("pdf");
	plot_delay.SetTerminal ("pdf");

	// Set the labels for each axis.
	plot.SetLegend ("Packet Size(in Bytes)", "Throughput Values(in Kbps)");
	plot_delay.SetLegend ("Packet Size(in Bytes)", "Delay(in s)");


	// Instantiate the dataset, set its title, and make the points be
	// plotted along with connecting lines.
	dataset_tcp.SetTitle ("Throughput FTP over TCP");
	dataset_tcp.SetStyle (Gnuplot2dDataset::LINES_POINTS);
	dataset_tcp.SetExtra ("lw 2");
	dataset_udp.SetTitle ("Throughput CBR over UDP");
	dataset_udp.SetStyle (Gnuplot2dDataset::LINES_POINTS);
	dataset_udp.SetExtra ("lw 2");
	
	dataset_tcp_delay.SetTitle ("Delay FTP over TCP");
	dataset_tcp_delay.SetStyle (Gnuplot2dDataset::LINES_POINTS);
	dataset_tcp_delay.SetExtra ("lw 2");
	dataset_udp_delay.SetTitle ("Delay CBR over UDP");
	dataset_udp_delay.SetStyle (Gnuplot2dDataset::LINES_POINTS);
	dataset_udp_delay.SetExtra ("lw 2");

	
	plot.AddDataset (dataset_tcp); // Add the dataset to the plot.
	plot.AddDataset (dataset_udp);
	
	plot_delay.AddDataset (dataset_udp_delay);
	plot_delay.AddDataset (dataset_tcp_delay);

	ofstream plotFile (plotFileName.c_str()); // Open the plot file.
	plot.GenerateOutput (plotFile); // Write the plot file.
	plotFile.close (); // Close the plot file.

	ofstream plotFile_delay (plotFileName_delay.c_str());
	plot_delay.GenerateOutput (plotFile_delay);
	plotFile_delay.close ();

	return 0;   // END OF PROGRAM
}	