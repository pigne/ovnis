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
 *
 * @file TraciClientClient.cpp
 *
 * @date Mar 10, 2010
 *
 * @author Yoann Pigné <yoann@pigne.org>
 *
 */

#include "traci-client.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <traci-server/TraCIConstants.h>
#include "ns3/log.h"
#define BUILD_TCPIP
#include <foreign/tcpip/storage.h>
#include <foreign/tcpip/socket.h>
#include <limits.h>
NS_LOG_COMPONENT_DEFINE ("TraciClient");

using namespace std;
using namespace tcpip;

namespace traciclient
{

  TraciClient::TraciClient():socket("localhost",0)

  {

  }

  TraciClient::~TraciClient()
  {
    //delete socket;
  }

  bool
  TraciClient::connect(std::string host, int port)
  {
    std::stringstream msg;
    //socket = new tcpip::Socket(host, port);
    socket = Socket(host,port);
    //    socket.set_blocking(true);
    sleep(1);
    try
      {
        socket.connect();
      }
    catch (SocketException &e)
      {
        msg << "#Error while connecting: " << e.what();
        errorMsg(msg);
        return false;
      }

    return true;
  }

  void
  TraciClient::errorMsg(std::stringstream& msg)
  {
    cerr << msg.str() << endl;

  }

  bool
  TraciClient::close()
  {
    socket.close();
    return true;
  }

  void
  TraciClient::commandClose()
  {
    tcpip::Storage outMsg;
    tcpip::Storage inMsg;
    std::stringstream msg;

    if (socket.port() == 0)
      {
        msg << "#Error while sending command: no connection to server";
        errorMsg(msg);
        return;
      }

    // command length
    outMsg.writeUnsignedByte(1 + 1);
    // command id
    outMsg.writeUnsignedByte(CMD_CLOSE);

    // send request message
    try
      {
        socket.sendExact(outMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while sending command: " << e.what();
        errorMsg(msg);
        return;
      }

    // receive answer message
    try
      {
        socket.receiveExact(inMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return;
      }

    // validate result state
    if (!reportResultState(inMsg, CMD_CLOSE))
      {
        return;
      }
  }

  bool
  TraciClient::reportResultState(tcpip::Storage& inMsg, int command, bool ignoreCommandId)
  {
	uint32_t cmdLength;
    int cmdId;
    int resultType;
    uint32_t cmdStart;
    std::string msg;

    try
      {
        cmdStart = inMsg.position();
        cmdLength = inMsg.readUnsignedByte();
        cmdId = inMsg.readUnsignedByte();
        if (cmdId != command && !ignoreCommandId)
          {
            //			answerLog << "#Error: received status response to command: "
            //					<< cmdId << " but expected: " << command << endl;
            return false;
          }
        resultType = inMsg.readUnsignedByte();
        msg = inMsg.readString();
      }
    catch (std::invalid_argument e)
      {
        int p = inMsg.position();
        NS_LOG_DEBUG( "#Error: an exception was thrown while reading result state message"
            << endl << "---" << p << endl);
        return false;
      }
    switch (resultType)
      {
      case RTYPE_ERR:
        NS_LOG_DEBUG( ".. Answered with error to command (" << cmdId
            << "), [description: " << msg << "]" << endl);
        return false;
      case RTYPE_NOTIMPLEMENTED:
        NS_LOG_DEBUG(".. Sent command is not implemented (" << cmdId
            << "), [description: " << msg << "]" << endl);
        return false;
      case RTYPE_OK:
        //	  NS_LOG_DEBUG(".. Command acknowledged (" << cmdId
        //				<< "), [description: " << msg << "]" << endl);
        break;
      default:
        NS_LOG_DEBUG(".. Answered with unknown result code(" << resultType
            << ") to command(" << cmdId << "), [description: " << msg
            << "]" << endl);
        return false;
      }
    if ((cmdStart + cmdLength) != inMsg.position())
      {
        NS_LOG_DEBUG("#Error: command at position " << cmdStart
            << " has wrong length" << endl);
        return false;
      }

    return true;
  }

  void
  TraciClient::submission(int start, int stop)
  {

    tcpip::Storage outMsg, inMsg;
    std::stringstream msg;
    std::string s = "*";
    outMsg.writeUnsignedByte(0);
    outMsg.writeInt(5 + 1 + 4 + 4 + 4 + (int) s.length() + 1 + 3);
    outMsg.writeUnsignedByte(CMD_SUBSCRIBE_SIM_VARIABLE);
    outMsg.writeInt((start));
    outMsg.writeInt((stop));
    outMsg.writeString(s);
    outMsg.writeUnsignedByte(3);
    //  outMsg.writeUnsignedByte(VAR_LOADED_VEHICLES_IDS);
    outMsg.writeUnsignedByte(VAR_TIME_STEP);
    outMsg.writeUnsignedByte(VAR_DEPARTED_VEHICLES_IDS);
    outMsg.writeUnsignedByte(VAR_ARRIVED_VEHICLES_IDS);

    // send request message
    try
      {
        socket.sendExact(outMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while sending command: " << e.what();
        errorMsg(msg);
        return;
      }

    // receive answer message
    try
      {
        socket.receiveExact(inMsg);
        if (!reportResultState(inMsg, CMD_SUBSCRIBE_SIM_VARIABLE))
          {
            return;
          }
      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return;
      }

    // don't validate anything from the answer
  }

  bool
  TraciClient::simulationStep(int startTime, int & time, std::vector<std::string> & in,
      std::vector<std::string> & out)
  {
    tcpip::Storage outMsg;
    tcpip::Storage inMsg;
    std::stringstream msg;

    if (socket.port() == 0)
      {
        msg << "#Error while sending command: no connection to server";
        errorMsg(msg);
        return false;
      }

    // command length
    outMsg.writeUnsignedByte(1 + 1 + 4);
    // command id
    outMsg.writeUnsignedByte(CMD_SIMSTEP2);
    outMsg.writeInt((startTime));
    // send request message
    try
      {
        socket.sendExact(outMsg);
      }
    catch (SocketException &e)
      {
        msg << "--Error while sending command: " << e.what();
        errorMsg(msg);
        return false;
      }
    // receive answer message
    try
      {
        socket.receiveExact(inMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return false;
      }
    // validate result state
    if (!reportResultState(inMsg, CMD_SIMSTEP2))
      {
        return false;
      }
    // validate answer message
    try
      {
        int noSubscriptions = inMsg.readInt();
        for (int s = 0; s < noSubscriptions; ++s)
          {

            try
              {
            	uint32_t respStart = inMsg.position();
                //                  std::cout << respStart << endl;
                int extLength = inMsg.readUnsignedByte();
                //                  std::cout << extLength << endl;
                int respLength = inMsg.readInt();
                //                  std::cout << respLength << endl ;

                int cmdId = inMsg.readUnsignedByte();
                //                 std::cout<<cmdId<<endl<< "--------------" << endl;

                if (cmdId < 0xe0 || cmdId > 0xef)
                  {
                    NS_LOG_DEBUG( "#Error: received response with command id: " << cmdId
                        << " but expected a subscription response (0xe0-0xef)" << endl);
                    return false;
                  }
                NS_LOG_DEBUG( "  CommandID=" << cmdId);
                string oId = inMsg.readString();
                NS_LOG_DEBUG("  ObjectID=" << oId);
                unsigned int varNo = inMsg.readUnsignedByte();
                NS_LOG_DEBUG( "  #variables=" << varNo << endl);

                switch (cmdId)
                  {
                  case RESPONSE_SUBSCRIBE_VEHICLE_VARIABLE:
                    for (int i = 0; i < varNo; ++i)
                      {
                        if (inMsg.readUnsignedByte() == ID_LIST && inMsg.readUnsignedByte() == RTYPE_OK)
                          {
                            // value data type
                            inMsg.readUnsignedByte();

                            vector<string> s = inMsg.readStringList();

                            NS_LOG_DEBUG( " string list value: [ " << endl);
                            for (vector<string>::iterator i = s.begin(); i != s.end(); ++i)
                              {
                                if (i != s.begin())
                                  {
                                    NS_LOG_DEBUG(", ");
                                  }
                                NS_LOG_DEBUG( "'" << *i);// << "' (");
                                //                                  Position2D *p = &silentAskPosition2D(*i);
                                //                                  NS_LOG_DEBUG(p->x << "," << p->y << "):" << silentAskRoad(*i));
                              }
                            NS_LOG_DEBUG( " ]" << endl);

                          }
                      }
                    break;

                  case RESPONSE_SUBSCRIBE_SIM_VARIABLE:
                    for (int i = 0; i < varNo; ++i)
                      {
                        int varId = inMsg.readUnsignedByte();
                        NS_LOG_DEBUG( "      VariableID=" << varId);
                        bool ok = inMsg.readUnsignedByte() == RTYPE_OK;
                        int valueDataType = inMsg.readUnsignedByte();

                        //----------  Get the list of vehicle that entered the simulation.
                        if (varId == VAR_DEPARTED_VEHICLES_IDS && ok)
                          {

                            vector<string> s = inMsg.readStringList();

                            NS_LOG_DEBUG( " string list value: [ " << endl);
                            for (vector<string>::iterator i = s.begin(); i != s.end(); ++i)
                              {
                                if (i != s.begin())
                                  {
                                    NS_LOG_DEBUG( ", ");
                                  }
                                NS_LOG_DEBUG( "'" << *i);

                                in.push_back((*i));

                                //                                  if (rand() % 100 > 80)
                                //                                    {
                                //                                      silentChangeRoute(*i, "middle", (double) INT_MAX);
                                //                                      NS_LOG_DEBUG( "(changed)");
                                //                                    }
                              }
                            NS_LOG_DEBUG( " ]" << endl);

                          }
                        //-------------- Get the list of vehicles that finished their trip and got out of the simulation.
                        else if (varId == VAR_ARRIVED_VEHICLES_IDS && ok)
                          {
                            vector<string> s = inMsg.readStringList();

                            NS_LOG_DEBUG( " string list value: [ " << endl);
                            for (vector<string>::iterator i = s.begin(); i != s.end(); ++i)
                              {
                                if (i != s.begin())
                                  {
                                    NS_LOG_DEBUG( ", ");
                                  }
                                NS_LOG_DEBUG( "'" << *i);

                                out.push_back((*i));

                                //                                  if (rand() % 100 > 80)
                                //                                    {
                                //                                      silentChangeRoute(*i, "middle", (double) INT_MAX);
                                //                                      NS_LOG_DEBUG( "(changed)");
                                //                                    }
                              }
                            NS_LOG_DEBUG( " ]" << endl);

                          }
                        //----------- Get The simulation time step.
                        else if (varId == VAR_TIME_STEP && ok)
                          {
                            time = inMsg.readInt();
                            NS_LOG_DEBUG( " Time value (ms): " << time << endl);
                          }
                        else
                          {
                            readAndReportTypeDependent(inMsg, valueDataType);
                          }
                      }
                    break;

                  default:

                    for (uint32_t i = 0; i < varNo; ++i)
                      {
                        int vId = inMsg.readUnsignedByte();
                        NS_LOG_DEBUG( "      VariableID=" << vId);
                        bool ok = inMsg.readUnsignedByte() == RTYPE_OK;
                        NS_LOG_DEBUG( "      ok=" << ok);
                        int valueDataType = inMsg.readUnsignedByte();
                        NS_LOG_DEBUG( " valueDataType=" << valueDataType);
                        readAndReportTypeDependent(inMsg, valueDataType);
                      }
                  }
              }
            catch (std::invalid_argument e)
              {
                NS_LOG_DEBUG( "#Error while reading message:" << e.what() << endl);
                return false;
              }

          }
      }
    catch (std::invalid_argument e)
      {
        NS_LOG_DEBUG( "#Error while reading message:" << e.what() << endl);
        return false;
      }
    return true;
  }

  float
  TraciClient::getFloat(u_int8_t dom, u_int8_t cmd, const std::string & node)
  {
    tcpip::Storage outMsg, inMsg;
    std::stringstream msg;
    outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) node.length()));
    outMsg.writeUnsignedByte(dom);
    outMsg.writeUnsignedByte(cmd);
    outMsg.writeString(node);

    // send request message
    try
      {
        socket.sendExact(outMsg);
      }
    catch (SocketException &e)
      {
        msg << "+++Error while sending command: " << e.what();
        errorMsg(msg);
        return false;
      }
    // receive answer message
    try
      {
        socket.receiveExact(inMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return false;
      }
    // validate result state
    if (!reportResultState(inMsg, dom))
      {
        return false;
      }
    // validate answer message
    try
      {
    	uint32_t respStart = inMsg.position();
    	uint32_t extLength = inMsg.readUnsignedByte();
    	uint32_t respLength = inMsg.readInt();
    	uint32_t cmdId = inMsg.readUnsignedByte();
        if (cmdId != (dom + 0x10))
          {
            NS_LOG_DEBUG( "#Error: received response with command id: " << cmdId << "but expected: " << (int) (dom
                    + 0x10) << endl);
            return false;
          }
        NS_LOG_DEBUG( "  CommandID=" << cmdId);
        int vId = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( "  VariableID=" << vId);
        string oId = inMsg.readString();
        NS_LOG_DEBUG( "  ObjectID=" << oId);
        int valueDataType = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( " valueDataType=" << valueDataType);

        //data should be string list
        //          readAndReportTypeDependent(inMsg, valueDataType);
        if (valueDataType == TYPE_FLOAT)
          {
            float f = inMsg.readFloat();
            NS_LOG_DEBUG( " float value:  " <<f<< endl);
            return f;
          }
        else
          {

            return 0;
          }

      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return false;
      }
    return 0;

  }

  bool
  TraciClient::getString(u_int8_t dom, u_int8_t cmd, const std::string & node, std::string & value)
  {
    tcpip::Storage outMsg, inMsg;
    std::stringstream msg;
    outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) node.length()));
    outMsg.writeUnsignedByte(dom);
    outMsg.writeUnsignedByte(cmd);
    outMsg.writeString(node);
    // send request message

    if (socket.port() == 0)
          {

            std::cerr<< "Error while sending command: no connection to server";
            std::flush(std::cerr);

          }
    try
      {
        socket.sendExact(outMsg);


      }
    catch (SocketException &e)
      {
          msg << "Error while sending command: " << e.what();
        errorMsg(msg);
        return false;
      }

   // receive answer message
    try
      {
        socket.receiveExact(inMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return false;
      }
    // validate result state
    if (!reportResultState(inMsg, dom))
      {
        return false;
      }

    // validate answer message
    try
      {
        int respStart = inMsg.position();
        int extLength = inMsg.readUnsignedByte();
        int respLength = inMsg.readInt();
        int cmdId = inMsg.readUnsignedByte();
        if (cmdId != (dom + 0x10))
          {
            NS_LOG_DEBUG( "#Error: received response with command id: " << cmdId << "but expected: " << (int) (dom
                    + 0x10) << endl);
            return false;
          }
        NS_LOG_DEBUG( "  CommandID=" << cmdId);
        int vId = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( "  VariableID=" << vId);
        string oId = inMsg.readString();
        NS_LOG_DEBUG( "  ObjectID=" << oId);
        int valueDataType = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( " valueDataType=" << valueDataType);

        //data should be string list
        //          readAndReportTypeDependent(inMsg, valueDataType);
        if (valueDataType == TYPE_STRING)
          {
            value.assign(inMsg.readString());
            NS_LOG_DEBUG( " string value:  " <<value<< std::endl);
            return true;
          }
        else
          {

            return 0;
          }

      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return false;
      }
    return 0;

  }

  bool
  TraciClient::getStringList(u_int8_t dom, u_int8_t cmd, const std::string & node, std::vector<std::string>& list)
  {
    tcpip::Storage outMsg, inMsg;
    std::stringstream msg;
    outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) node.length()));
    outMsg.writeUnsignedByte(dom);
    outMsg.writeUnsignedByte(cmd);
    outMsg.writeString(node);

    // send request message
    try
      {
        socket.sendExact(outMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while sending command: " << e.what();
        errorMsg(msg);
        return false;
      }
    // receive answer message
    try
      {
        socket.receiveExact(inMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return false;
      }
    // validate result state
    if (!reportResultState(inMsg, dom))
      {
        return false;
      }
    // validate answer message
    try
      {
        int respStart = inMsg.position();
        int extLength = inMsg.readUnsignedByte();
        int respLength = inMsg.readInt();
        int cmdId = inMsg.readUnsignedByte();
        if (cmdId != (dom + 0x10))
          {
            NS_LOG_DEBUG( "#Error: received response with command id: " << cmdId << "but expected: " << (int) (dom
                    + 0x10) << endl);
            return false;
          }
        NS_LOG_DEBUG( "  CommandID=" << cmdId);
        int vId = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( "  VariableID=" << vId);
        string oId = inMsg.readString();
        NS_LOG_DEBUG( "  ObjectID=" << oId);
        int valueDataType = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( " valueDataType=" << valueDataType);

        //data should be string list
        //          readAndReportTypeDependent(inMsg, valueDataType);
        if (valueDataType == TYPE_STRINGLIST)
          {
            vector<string> s = inMsg.readStringList();
            NS_LOG_DEBUG( " string list value: [ " << endl);
            for (vector<string>::iterator i = s.begin(); i != s.end(); ++i)
              {
                if (i != s.begin())
                  {
                    NS_LOG_DEBUG( ", ");
                  }
                //                          std::cout<<(*i)<<endl;
                list.push_back((*i));
                NS_LOG_DEBUG( '"' << *i << '"');
              }
            NS_LOG_DEBUG( " ]" << endl);
          }
        else
          {

            return false;
          }

      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return false;
      }
    return true;
  }

  bool
  TraciClient::readAndReportTypeDependent(tcpip::Storage &inMsg, int valueDataType)
  {
    if (valueDataType == TYPE_UBYTE)
      {
        int ubyte = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( " Unsigned Byte Value: " << ubyte << endl);
      }
    else if (valueDataType == TYPE_BYTE)
      {
        int byte = inMsg.readByte();
        NS_LOG_DEBUG( " Byte value: " << byte << endl);
      }
    else if (valueDataType == TYPE_INTEGER)
      {
        int integer = inMsg.readInt();
        NS_LOG_DEBUG( " Int value: " << integer << endl);
      }
    else if (valueDataType == TYPE_FLOAT)
      {
        float floatv = inMsg.readFloat();
        //        if (floatv < 0.1 && floatv > 0)
        //          {
        //            answerLog.setf(std::ios::scientific, std::ios::floatfield);
        //          }
        NS_LOG_DEBUG( " float value: " << floatv << endl);
      }
    else if (valueDataType == TYPE_DOUBLE)
      {
        double doublev = inMsg.readDouble();
        NS_LOG_DEBUG( " Double value: " << doublev << endl);
      }
    else if (valueDataType == TYPE_BOUNDINGBOX)
      {
        BoundingBox box;
        box.lowerLeft.x = inMsg.readFloat();
        box.lowerLeft.y = inMsg.readFloat();
        box.upperRight.x = inMsg.readFloat();
        box.upperRight.y = inMsg.readFloat();
        NS_LOG_DEBUG( " BoundaryBoxValue: lowerLeft x=" << box.lowerLeft.x
            << " y=" << box.lowerLeft.y << " upperRight x="
            << box.upperRight.x << " y=" << box.upperRight.y << endl);
      }
    else if (valueDataType == TYPE_POLYGON)
      {
        int length = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( " PolygonValue: ");
        for (int i = 0; i < length; i++)
          {
            float x = inMsg.readFloat();
            float y = inMsg.readFloat();
            NS_LOG_DEBUG( "(" << x << "," << y << ") ");
          }
        NS_LOG_DEBUG( endl);
      }
    else if (valueDataType == POSITION_3D)
      {
        float x = inMsg.readFloat();
        float y = inMsg.readFloat();
        float z = inMsg.readFloat();
        NS_LOG_DEBUG( " Position3DValue: " << std::endl);
        NS_LOG_DEBUG( " x: " << x << " y: " << y << " z: " << z << std::endl);
      }
    else if (valueDataType == POSITION_ROADMAP)
      {
        std::string roadId = inMsg.readString();
        float pos = inMsg.readFloat();
        int laneId = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( " RoadMapPositionValue: roadId=" << roadId << " pos="
            << pos << " laneId=" << laneId << std::endl);
      }
    else if (valueDataType == TYPE_TLPHASELIST)
      {
        int length = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( " TLPhaseListValue: length=" << length << endl);
        for (int i = 0; i < length; i++)
          {
            string pred = inMsg.readString();
            string succ = inMsg.readString();
            int phase = inMsg.readUnsignedByte();
            NS_LOG_DEBUG( " precRoad=" << pred << " succRoad=" << succ
                << " phase=");
            switch (phase)
              {
              case TLPHASE_RED:
                NS_LOG_DEBUG( "red" << endl);
                break;
              case TLPHASE_YELLOW:
                NS_LOG_DEBUG("yellow" << endl);
                break;
              case TLPHASE_GREEN:
                NS_LOG_DEBUG( "green" << endl);
                break;
              default:
                NS_LOG_DEBUG( "#Error: unknown phase value" << (int) phase
                    << endl);
                return false;
              }
          }
      }
    else if (valueDataType == TYPE_STRING)
      {
        string s = inMsg.readString();
        NS_LOG_DEBUG( " string value: " << s << endl);
      }
    else if (valueDataType == TYPE_STRINGLIST)
      {
        vector<string> s = inMsg.readStringList();
        NS_LOG_DEBUG( " string list value: [ " << endl);
        for (vector<string>::iterator i = s.begin(); i != s.end(); ++i)
          {
            if (i != s.begin())
              {
                NS_LOG_DEBUG( ", ");
              }
            NS_LOG_DEBUG( '"' << *i << '"');
          }
        NS_LOG_DEBUG( " ]" << endl);
      }
    else if (valueDataType == TYPE_COMPOUND)
      {
        int no = inMsg.readInt();
        NS_LOG_DEBUG( " compound value with " << no << " members: [ " << endl);
        for (int i = 0; i < no; ++i)
          {
            int currentValueDataType = inMsg.readUnsignedByte();
            NS_LOG_DEBUG( " valueDataType=" << currentValueDataType);
            readAndReportTypeDependent(inMsg, currentValueDataType);
          }
        NS_LOG_DEBUG( " ]" << endl);
      }
    else if (valueDataType == POSITION_2D)
      {
        float xv = inMsg.readFloat();
        float yv = inMsg.readFloat();
        NS_LOG_DEBUG( " position value: (" << xv << "," << yv << ")" << endl);
      }
    else if (valueDataType == TYPE_COLOR)
      {
        int r = inMsg.readUnsignedByte();
        int g = inMsg.readUnsignedByte();
        int b = inMsg.readUnsignedByte();
        int a = inMsg.readUnsignedByte();
        NS_LOG_DEBUG(" color value: (" << r << "," << g << "," << b << "," << a
            << ")" << endl);
      }
    else
      {
        NS_LOG_DEBUG( "#Error: unknown valueDataType!" << endl);
        return false;
      }
    return true;
  }

  Position2D
  TraciClient::getPosition2D(std::string &veh)

  {
    Position2D p;
    tcpip::Storage outMsg, inMsg;
    std::stringstream msg;
    if (socket.port() == 0)
      {
        msg << "#Error while sending command: no connection to server";
        errorMsg(msg);
        return p;
      }
    // command length
    outMsg.writeUnsignedByte(1 + 1 + 1 + 4 + (int) veh.length());
    // command id
    outMsg.writeUnsignedByte(CMD_GET_VEHICLE_VARIABLE);
    // variable id
    outMsg.writeUnsignedByte(VAR_POSITION);
    // object id
    outMsg.writeString(veh);

    // send request message
    try
      {
        socket.sendExact(outMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while sending command: " << e.what();
        errorMsg(msg);
        return p;
      }

    // receive answer message
    try
      {
        socket.receiveExact(inMsg);
        if (!reportResultState(inMsg, CMD_GET_VEHICLE_VARIABLE))
          {
            return p;
          }
      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return p;
      }
    // validate result state
    try
      {
    	uint32_t respStart = inMsg.position();
    	int extLength = inMsg.readUnsignedByte();
        int respLength = inMsg.readInt();
        int cmdId = inMsg.readUnsignedByte();
        if (cmdId != (CMD_GET_VEHICLE_VARIABLE + 0x10))
          {
            NS_LOG_DEBUG( "#Error: received response with command id: " << cmdId
                << "but expected: " << (int) (CMD_GET_VEHICLE_VARIABLE
                    + 0x10) << endl);
            return p;
          }
        //  VariableID=" <<
        inMsg.readUnsignedByte();
        //answerLog << "  ObjectID=" <<
        inMsg.readString();
        //int valueDataType =
        inMsg.readUnsignedByte();

        p.x = inMsg.readFloat();
        p.y = inMsg.readFloat();
        //answerLog << xv << "," << yv << endl;
        return p;
      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return p;
      }

  }

  void
  TraciClient::changeRoad(std::string nodeId, std::string roadId, float travelTime)
  {
    tcpip::Storage outMsg;
    tcpip::Storage inMsg;
    std::stringstream msg;

    if (socket.port() == 0)
      {
        msg << "#Error while sending command: no connection to server";
        errorMsg(msg);
        return;
      }

    // command length
    outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) nodeId.length()) + 1 + 4 + (1 + 4) + (1 + 4) + (1 + (4
        + (int) roadId.length())) + (1 + 4));
    // command id
    outMsg.writeUnsignedByte(CMD_SET_VEHICLE_VARIABLE);
    // var id VAR_EDGE_TRAVELTIME
    outMsg.writeUnsignedByte(VAR_EDGE_TRAVELTIME);
    // vehicle id
    outMsg.writeString(nodeId);
    //type of value (compound)
    outMsg.writeUnsignedByte(TYPE_COMPOUND);

    // compoung value for edge travael time;
    //number of elements (always=4)
    outMsg.writeInt(4);

    //int begin
    outMsg.writeUnsignedByte(TYPE_INTEGER);
    outMsg.writeInt(0);
    //int end
    outMsg.writeUnsignedByte(TYPE_INTEGER);
    outMsg.writeInt(INT_MAX);
    //string edge
    outMsg.writeUnsignedByte(TYPE_STRING);
    outMsg.writeString(roadId);
    //float value
    outMsg.writeUnsignedByte(TYPE_FLOAT);
    outMsg.writeFloat(travelTime);

    // send request message
    try
      {
        socket.sendExact(outMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while sending command: " << e.what();
        errorMsg(msg);
        return;
      }

    // receive answer message
    try
      {
        socket.receiveExact(inMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return;
      }

    // validate result state
    if (!reportResultState(inMsg, CMD_SET_VEHICLE_VARIABLE))
      {
        return;
      }

    outMsg.reset();
    inMsg.reset();

    // command length
    outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) nodeId.length()) + 1 + 4);
    // command id
    outMsg.writeUnsignedByte(CMD_SET_VEHICLE_VARIABLE);
    // var id VAR_EDGE_TRAVELTIME
    outMsg.writeUnsignedByte(CMD_REROUTE_TRAVELTIME);
    // vehicle id
    outMsg.writeString(nodeId);
    //type of value (compound)
    outMsg.writeUnsignedByte(TYPE_COMPOUND);

    // compoung value for reroute on  travael time;
    //number of elements (always=4)
    outMsg.writeInt(0);

    // send request message
    try
      {
        socket.sendExact(outMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while sending command: " << e.what();
        errorMsg(msg);
        return;
      }

    // receive answer message
    try
      {
        socket.receiveExact(inMsg);
      }
    catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return;
      }

    // validate result state
    if (!reportResultState(inMsg, CMD_SET_VEHICLE_VARIABLE))
      {
        return;
      }

  }


}

