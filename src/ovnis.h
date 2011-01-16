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
 *
 * @file ovnis.h
 * @date Apr 21, 2010
 *
 * @author Yoann Pigné
 */

#ifndef OVNIS_H_
#define OVNIS_H_

//
// ----- NS-3 related includes
#include <ns3/object.h>
#include "traci/traci-client.h"
#include "ns3/node-container.h"
#include "ns3/nqos-wifi-mac-helper.h"
#include "ns3/ipv4-address-helper.h"

//
// ----- application related includes
#include "helper/ovnis-wifi-helper.h"
#include "devices/wifi/ovnis-wifi-channel.h"



using namespace std;
using namespace traciclient;

namespace ns3
{
  class StatInput
  {
  public:
    double sum;
    int inputs;
  };

  class Ovnis : public ns3::Object
  {
  public:
    static TypeId
        GetTypeId(void);
    Ovnis();
    virtual
    ~Ovnis();

    void
    run();

    void
    globalStat(const string & name, double value);





    static std::string
    outList(std::vector<std::string>& list);

  protected:

    virtual void
    DoDispose(void);

    virtual void
    DoStart(void);

    void
    initializeNetwork();

    void
    initializeLowLayersNetwork();

    void
    initializeHighLayersNetwork();

    void
    CreateNetworkDevices(NodeContainer node_container);

    void
    DestroyNetworkDevices(NodeContainer to_destroy);

    void
    trafficSimulationStep();

    void
    updateInOutVehicles();

    void
    uppdateVehiclesPositions();

    void
    move();

    void
    computeStats();

  protected:

    // for statistics
    typedef map<string, StatInput> statType;
    statType stats;
    uint64_t start,end;
            struct timespec tp;

    Ptr<TraciClient> traciClient;

    // current time in ms !!!!
    int currentTime;

    std::vector<std::string> runningVehicles;

    std::vector<std::string> in;
    std::vector<std::string> out;
    std::vector<std::string> loaded;

    NodeContainer node_container;

    // network
    OvnisWifiPhyHelper phy;
    NqosWifiMacHelper mac;
    OvnisWifiChannelHelper wifiChannel;
    WifiHelper wifi;
    Ipv4AddressHelper address;
    ObjectFactory m_application_factory;
    Ptr<OvnisWifiChannel> channel;

    // application
    std::string m_ovnis_application;

    /**
     * The configuration file for running SUMO
     */
    std::string sumoConfig;

    /**
     * The host machine on which SUMO will run
     */
    std::string sumoHost;

    /**
     * The system path where the SUMO executable is located
     */
    std::string sumoPath;

    /**
     * The port number (network) on the host machine SUMO will run on
     */
    int port;

    bool verbose;

    /**
     * Start time in the simulation scale (in seconds)
     */
    int startTime;

    /**
     * Stop time in the simulation scale (in seconds)
     */
    int stopTime;

    /**
     * Communication range used to subdivide the simulation space (in meters)
     */
    double communicationRange;
    double boundaries[2];

    /**
     * Do we start SUMO?
     */
    bool startSumo;


  };
}
#endif /* OVNIS_H_ */
