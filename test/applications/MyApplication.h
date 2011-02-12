/*
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
 * MyApplication.h
 *
 *  Created on: Mar 26, 2010
 *      Author: Yoann Pigné
 */

#ifndef MYAPPLICATION_H_
#define MYAPPLICATION_H_

#include "ns3/application.h"

#include "ns3/nstime.h"
#include "ns3/boolean.h"

#include "ns3/simulator.h"

#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/mac48-address.h"

#include "ns3/address.h"

#include "ns3/event-id.h"
#include "ns3/traced-callback.h"
#include "ns3/callback.h"
#include "ns3/global-value.h"
#include <vector>
#include "ns3/data-rate.h"

#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include <set>

#include "traci/traci-client.h"
#include "ovnis.h"
#include "applications/ovnis-application.h"


using namespace traciclient;
using namespace std;

namespace ns3
{

  class MyApplication : public ns3::OvnisApplication
  {
  public:
    static TypeId
    GetTypeId(void);

    UniformVariable  rando;

    MyApplication();
    virtual
    ~MyApplication();


     virtual void
    ReceiveData(Ptr<Socket> );

  protected:
    virtual void
    DoDispose(void);





  private:

    // inherited from Application base class.
    virtual void
    StartApplication(void);



    void
       SendMyState();

    void
       SendState(float date, const string & edge);

    void
    Action();
    Ptr<Packet>
    CreateStatePacket(float date, const string & edge);
    void
    RetrieveStatePacket(Ptr<Packet> & packet, float & date, string & edge);
    bool m_verbose;

    uint16_t m_port;

    Ipv4Address m_addr;

    /**
     * Socket used to communicate.
     */
    Ptr<Socket> m_Socket;


    /**
     * Edges that are ignored in the changing route process because not avoidable.
     */
    std::set<std::string> m_ignored_edges;


    /**
     * Last time a packet was re-send.
     */
    double m_last_resend;

    /**
     * True if this vehicle is considered in a traffic jam.
     */
    bool m_jam_state;




  };

}

#endif /* MYAPPLICATION_H_ */
