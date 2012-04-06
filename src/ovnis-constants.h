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
 * @file ovnis-constants.h
 * @date Apr 21, 2010
 *
 * @author Yoann Pigné
 */

#ifndef OVNIS_CONSTANTS_H_
#define OVNIS_CONSTANTS_H_

#ifndef PI
#define PI 3.14159265
#endif


/// interval of time between 2 active decisions about JAMEs
#ifndef PROACTIVE_INTERVAL
#define PROACTIVE_INTERVAL  5
#endif


/// period of the movement steps
#ifndef MOVE_INTERVAL
#define MOVE_INTERVAL  1
#endif

/// The system path where the SUMO executable is located
#ifndef SUMO_PATH
#define SUMO_PATH "/opt/sumo/bin/sumo"
#endif

#ifndef SUMO_HOST
#define SUMO_HOST "localhost"
#endif

#ifndef SUMO_CONFIG
#define SUMO_CONFIG "./test.sumo.cfg"
#endif
#endif /* OVNIS_CONSTANTS_H_ */
