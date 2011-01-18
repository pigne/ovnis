=====
OVNIS
=====

:Date: Jan 16, 2011
:Author: Yoann Pigné
:Organization: University of Luxembourg
:Contact: yoann@pigne.org
:Version: 0.1
:Copyright: 2010-2011, University of Luxembourg

.. This document is a general introduction to the project. Check the wiki for more information. 

OVNIS stands for Online Vehicular Network Integrated Simulation. It is a platform dedicated to the simulation of vehicular network applications. OVNIS integrates network simulator `ns-3`_ and traffic microsimulator `SUMO`_. Both simulators are coupled so that the mobility of vehicles in SUMO is injected in the mobility model of ns-3. Inversely, any simulated network application in ns-3 can influence the traffic simulation and, for instance, reroute simulated vehicles. 



.. _ns-3: http://www.nsnam.org/
.. _SUMO: http://sumo.sourceforge.net/
