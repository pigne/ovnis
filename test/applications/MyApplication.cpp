/**
 *
 *
 * Copyright (c) 2010-2011 University of Luxembourg
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
 * @file MyApplication.cpp
 * 
 * @author Yoann Pigné <yoann@pigne.org>
 *
 */

#include "ns3/core-module.h"
#include "ns3/common-module.h"
#include "ns3/node-module.h"
#include "ns3/helper-module.h"
#include "ns3/mobility-module.h"
#include "ns3/contrib-module.h"
#include "ns3/udp-socket-factory.h"
#include "MyApplication.h"
#include "ns3/node-list.h"
#include "ns3/mac48-address.h"
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tag-buffer.h"

#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"

#include "ns3/wifi-net-device.h"

#include <limits.h>
#include "ns3/config.h"
#include "ns3/integer.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/callback.h"

#include <traci-server/TraCIConstants.h>

#include "ovnis.h"
#include "my-constants.h"

#include "applications/ovnis-application.h"


namespace ns3
{
  NS_LOG_COMPONENT_DEFINE ("MyApplication");
  NS_OBJECT_ENSURE_REGISTERED (MyApplication);

  TypeId
  MyApplication::GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::MyApplication") .SetParent<OvnisApplication> () .AddConstructor<MyApplication> ();

    return tid;
  }

  MyApplication::MyApplication() :
    m_port(2000), m_Socket(0)
  {
    m_last_resend = 0;
    m_jam_state = false;

  }

  MyApplication::~MyApplication()
  {
    // TODO Auto-generated destructor  stub
  }

  void
  MyApplication::DoDispose(void)
  {
    NS_LOG_FUNCTION_NOARGS ();

    if (m_Socket != 0)
      {
        m_Socket->Close();
      }
    else
      {
        NS_LOG_WARN("MyApplication found null socket to close in StopApplication");
      }
    // chain up
    OvnisApplication::DoDispose();

  }

  void
  MyApplication::SendMyState()
  {
    NS_LOG_FUNCTION_NOARGS();

    SendState((float) Simulator::Now().GetSeconds(), m_edge);

  }

  void
  MyApplication::SendState(float date, const string & edge)
  {
    NS_LOG_FUNCTION_NOARGS();
    Address realTo = InetSocketAddress(Ipv4Address("255.255.255.255"), MyApplication::m_port);
    Ptr<Packet> p = CreateStatePacket(date, edge);
    ovnis->globalStat("SentData",(double)p->GetSize());

    m_Socket->SendTo(p, 0, realTo);

  }

    Ptr<Packet>
  MyApplication::CreateStatePacket(float date, const string & edge)
  {
    NS_LOG_FUNCTION_NOARGS();
    int size = sizeof(float) + sizeof(int) + edge.size() * sizeof(char);
    uint8_t * t = (uint8_t*) malloc(size);

    TagBuffer tg(t, t + size);

    tg.WriteU32(date);
    tg.WriteU32(edge.size());
    tg.Write((uint8_t*) edge.c_str(), edge.size());

    Ptr<Packet> p = Create<Packet> (t, size);
    free(t);
    return p;
  }



void
  MyApplication::StartApplication(void)
  {
    NS_LOG_FUNCTION ("");


     // get the traci helper
     traciClient = Names::Find<TraciClient>("TraciClient");

     // get the main class for statistics.
     ovnis = Names::Find<Ovnis>("Ovnis");

     // get the mobility model (for speed requests)
     Ptr<Object> object = GetNode();
     mobilityModel = object->GetObject<ConstantVelocityMobilityModel> ();


     // ask my name
     m_name = Names::FindName(GetNode());


     // ask for my route
     traciClient->getStringList(CMD_GET_VEHICLE_VARIABLE, VAR_EDGES, m_name, m_route);




     // ask for my edge
     GetEdgeInfo();


      Ptr<SocketFactory> socketFactory = GetNode()->GetObject<UdpSocketFactory> ();
      m_Socket = socketFactory->CreateSocket();
      m_Socket->SetRecvCallback(MakeCallback(&MyApplication::ReceiveData, this));
      m_Socket->Bind(InetSocketAddress(Ipv4Address("0.0.0.0"), m_port));
      // store the own address
      Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4> ();



     m_actionEvent
         = Simulator::Schedule(Seconds(rando.GetValue(0, PROACTIVE_INTERVAL)), &MyApplication::Action, this);



  }

  void
  MyApplication::Action()
  {
    NS_LOG_FUNCTION(m_name<<Simulator::Now().GetSeconds());

    if (m_stopTime != TimeStep(0) && Simulator::Now() >= m_stopTime)
      {
        //      cerr<<m_name<<" shall stop now"<<endl;
        return;
      }
    if (m_realStartDate == TimeStep(0))
      m_realStartDate = Simulator::Now();

    if (m_edge.empty())
      {
        GetEdgeInfo();
      }
    else
      {
        // ask for my current edge
        string new_edge;
        traciClient ->getString(CMD_GET_VEHICLE_VARIABLE, VAR_ROAD_ID, m_name, new_edge);
        // if I changed edge then I ask for new speed limits
        if (new_edge.empty())
          {
            // if new_edge is empty, then there is a problem. The vehicle has been removed from
            cerr << m_name << " can't access it cunning edge. It has probably been removed in SUMO. Let stop it."
                << endl;
            m_stopTime == Simulator::Now();
            return;
          }
        if (new_edge != m_edge)
          {
            m_edge = new_edge;
            string lane;
            traciClient ->getString(CMD_GET_VEHICLE_VARIABLE, VAR_LANE_ID, m_name, lane);
            m_max_speed = traciClient ->getFloat(CMD_GET_LANE_VARIABLE, VAR_MAXSPEED, lane);
            NS_LOG_DEBUG(m_name<<" moving to new edge "<< new_edge<<" lane "<< lane << " max-speed" << m_max_speed);
          }

      }
    // ask for my speed
    m_speed = traciClient ->getFloat(CMD_GET_VEHICLE_VARIABLE, VAR_SPEED, m_name);

    //decide if JAME or not according to a threshold
    if (m_speed < (SPEED_THRESHOLD * m_max_speed))
      {
        NS_LOG_DEBUG(m_name << " JAM?");
        if (m_jam_state)
          {
            // I am in a jam for more than PROACTIVE_INTERVAL
            NS_LOG_DEBUG(m_name<<" JAM.");
            m_last_resend = Simulator::Now().GetSeconds();
            m_sendEvent = Simulator::ScheduleNow(&MyApplication::SendMyState, this);
          }
        m_jam_state = true;
      }
    else
      {
        // if ever I was in JAM state, then I am not anymore
        if (m_jam_state)
          {
            NS_LOG_DEBUG(m_name<<" NO MORE JAM.");
            m_jam_state = false;
          }
      }

    m_actionEvent = Simulator::Schedule(Seconds(PROACTIVE_INTERVAL), &MyApplication::Action, this);

  }

  void
  MyApplication::ReceiveData(Ptr<Socket> socket)
  {

    Ptr<Packet> packet;
    Address neighborMacAddress;
    packet = socket->RecvFrom(neighborMacAddress);
    //    InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom(neighborMacAddress);
    //    Ipv4Address neighborIPv4Addr = inetSourceAddr.GetIpv4();

    ovnis->globalStat("ReceivedData" ,(double)packet->GetSize());

    std::string received_edge;
    float received_date;


    RetrieveStatePacket(packet, received_date, received_edge);

    // set the JAM flag if it's my edge however my speed is not 0
    if (m_edge == received_edge)
      m_jam_state = true;

    // do I move

    if (m_ignored_edges.find(received_edge) == m_ignored_edges.end())
      {

        vector<string>::iterator it = m_route.begin();
        while (it != m_route.end() && (*it) != received_edge)
          ++it;
        if (it != m_route.end())
          {
            // this JAMed edge is in the route. Let's try to re-route
            traciClient->changeRoad(m_name, received_edge, (float) INT_MAX);
            m_route.clear();
            traciClient->getStringList(CMD_GET_VEHICLE_VARIABLE, VAR_EDGES, m_name, m_route);

            it = m_route.begin();
            while (it != m_route.end() && (*it) != received_edge)
              ++it;
            if (it != m_route.end())
              {
                m_ignored_edges.insert(received_edge);
              }
            else
              {
               // cerr << m_name << " REROUTED: " << outList(m_route) << endl;
              }
          }
      }

    // do I re-send
    double now = Simulator::Now().GetSeconds();
    if (m_last_resend < (now - RESEND_INTERVAL))
      {

        if (received_date > now - PACKET_TTL)
          {

            m_sendEvent = Simulator::Schedule(Seconds(MyApplication::rando.GetValue(0, RESEND_INTERVAL)),
                &MyApplication::SendState, this, received_date, received_edge);

            m_last_resend = now;

          }
      }

    //Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4> ();
    //    std::cout << Names::FindName(GetNode()) << " (" << ipv4->GetAddress(1, 0).GetLocal() << ") recieved x=" << x
    //        << " s=" << s << "  from " << neighborIPv4Addr << std::endl;

  }

  void
  MyApplication::RetrieveStatePacket(Ptr<Packet> & packet, float & date, string & edge)
  {
    uint8_t * t;
    t = (uint8_t*) malloc(200 * sizeof(uint8_t));
    int size = packet->CopyData(t, 200);
    TagBuffer tg(t, t + size);

    date = (float) tg.ReadU32();
    uint32_t s_size = tg.ReadU32();

    char *s = (char*) malloc(s_size * sizeof(char));
    tg.Read((uint8_t*) s, s_size);
    edge.assign(s, s_size);
    free(s);
    free(t);
  }





}
