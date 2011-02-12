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
 * @file ovnis-application.cpp
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
#include "ovnis-application.h"
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
#include "ovnis-constants.h"

namespace ns3
{
  NS_LOG_COMPONENT_DEFINE ("OvnisApplication");
  NS_OBJECT_ENSURE_REGISTERED (OvnisApplication);

  TypeId
  OvnisApplication::GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::OvnisApplication") .SetParent<Application> () .AddConstructor<OvnisApplication> ();
    return tid;
  }

  OvnisApplication::OvnisApplication()
  {
  }

  OvnisApplication::~OvnisApplication()
  {
  }

  void
  OvnisApplication::DoDispose(void)
  {
    NS_LOG_FUNCTION_NOARGS ();

    if (m_realStartDate != TimeStep(0))
    {
      double duration;
      if (m_stopTime != TimeStep(0))
        duration = m_stopTime.GetSeconds() - m_realStartDate.GetSeconds();
      else
        duration = Simulator::Now().GetSeconds() - m_realStartDate.GetSeconds();
      std::cout<<duration<<std::endl;
      ovnis->globalStat("JourneyDuration", duration);
      ovnis->globalStat("JourneyLength", (double) m_route.size());
    }

    Simulator::Cancel(m_sendEvent);
    Simulator::Cancel(m_actionEvent);

    // chain up
    Application::DoDispose();
  }

  void
  OvnisApplication::GetEdgeInfo(void)
  {
    NS_LOG_DEBUG(m_name);


    traciClient ->getString(CMD_GET_VEHICLE_VARIABLE, VAR_ROAD_ID, m_name, m_edge);


    // ask for max_speed
    string lane;
    traciClient ->getString(CMD_GET_VEHICLE_VARIABLE, VAR_LANE_ID, m_name, lane);
    m_max_speed = traciClient ->getFloat(CMD_GET_LANE_VARIABLE, VAR_MAXSPEED, lane);

  }

  void
  OvnisApplication::StartApplication(void)
  {
    NS_LOG_FUNCTION (m_name);

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

    m_actionEvent
        = Simulator::Schedule(Seconds(rando.GetValue(0, PROACTIVE_INTERVAL)), &OvnisApplication::Action, this);

  }

  void
  OvnisApplication::Action()
  {
    NS_LOG_FUNCTION(m_name<<Simulator::Now().GetSeconds());

    if (m_stopTime != TimeStep(0) && Simulator::Now() >= m_stopTime)
    {
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
        cerr << m_name << " can't access it cunning edge. It has probably been removed in SUMO. Let stop it." << endl;
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





    ///


    m_actionEvent = Simulator::Schedule(Seconds(PROACTIVE_INTERVAL), &OvnisApplication::Action, this);

  }

  void
  OvnisApplication::StopApplication(void)
  {

  }

  void
  OvnisApplication::ReceiveData(Ptr<Socket> x)
  {
  }

}
