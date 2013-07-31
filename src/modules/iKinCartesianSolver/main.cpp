/* 
 * Copyright (C) 2010 RobotCub Consortium, European Commission FP6 Project IST-004370
 * Author: Ugo Pattacini
 * email:  ugo.pattacini@iit.it
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

/** 
\defgroup iKinCartesianSolver iKinCartesianSolver 
 
@ingroup icub_module  
 
Just a container which runs the \ref iKinSlv "Cartesian Solver" 
taking parameters from a configuration file. 
 
Copyright (C) 2010 RobotCub Consortium
 
Author: Ugo Pattacini 

CopyPolicy: Released under the terms of the GNU GPL v2.0.

\section intro_sec Description 
See \ref iKinSlv "Cartesian Solver" for documentation. 
 
\section lib_sec Libraries 
- YARP libraries. 
- \ref iKin "iKin" library (it requires IPOPT: see the 
  <a
  href="http://wiki.icub.org/wiki/Installing_IPOPT">wiki</a>).

\section parameters_sec Parameters
--part \e type [mandatory]
- select the part to run. \e type is the group name within the 
  configuration file (e.g. left_arm, right_arm, ...).

--context \e directory [optional]
- allow specifying a different path where to search the 
  configuration file.
 
--from \e file [optional]
- allow specifying a different configuration file from the 
  default one which is \e cartesianSolver.ini.
 
\section portsa_sec Ports Accessed
 
All ports which allow the access to motor interface shall be 
previously open. 

\section portsc_sec Ports Created 
 
- /<solverName>/in : for requests in streaming mode.
- /<solverName>/rpc : for requests and replies.
- /<solverName>/out : for output streaming. 
 
\note for a detailed description, see \ref iKinSlv.
 
\section conf_file_sec Configuration Files
 
Here's how the configuration file will look like: 
 
\code 
[left_arm]
robot          icub
name           cartesianSolver/left_arm
type           left
period         20
dof            (0 0 0 1 1 1 1 1 1 1)
rest_pos       (0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0)
rest_weights   (1.0 1.0 1.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0) 
pose           full
mode           shot
verbosity      off
maxIter        200
tol            0.001
xyzTol         0.000001
interPoints    off 
ping_robot_tmo 20.0 
 
[right_arm]
robot          icub
name           cartesianSolver/right_arm
type           right
period         20
dof            (0 0 0 1 1 1 1 1 1 1)
rest_pos       (0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0)
rest_weights   (1.0 1.0 1.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0) 
pose           full
mode           shot
verbosity      off
maxIter        200
tol            0.001   
xyzTol         0.000001
interPoints    off 
ping_robot_tmo 20.0 
 
[left_leg]
robot          icub
name           cartesianSolver/left_leg
type           left
period         20
dof            (1 1 1 1 1 1)
rest_pos       (0.0 0.0 0.0 0.0 0.0 0.0)
rest_weights   (0.0 0.0 0.0 0.0 0.0 0.0) 
pose           full
mode           shot
verbosity      off
maxIter        200
tol            0.001   
xyzTol         0.000001
interPoints    off
ping_robot_tmo 20.0 

[right_leg]
robot          icub
name           cartesianSolver/right_leg
type           right
period         20
dof            (1 1 1 1 1 1)
rest_pos       (0.0 0.0 0.0 0.0 0.0 0.0)
rest_weights   (0.0 0.0 0.0 0.0 0.0 0.0) 
pose           full
mode           shot
verbosity      off
maxIter        200
tol            0.001   
xyzTol         0.000001
interPoints    off
ping_robot_tmo 20.0 
\endcode 
 
\note for a detailed description of options, see \ref iKinSlv
      "Cartesian Solver".
 
\section tested_os_sec Tested OS
Windows, Linux

\author Ugo Pattacini
*/ 

#include <iostream>
#include <iomanip>
#include <string>

#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>

#include <iCub/iKin/iKinSlv.h>

using namespace std;
using namespace yarp::os;
using namespace iCub::iKin;


/************************************************************************/
class SolverModule: public RFModule
{
protected:
    CartesianSolver *slv;

public:
    /************************************************************************/
    SolverModule()
    {
        slv=NULL;
    }

    /************************************************************************/
    bool configure(ResourceFinder &rf)
    {                
        string part, slvName;

        if (rf.check("part"))
            part=rf.find("part").asString().c_str();
        else
        {
            cout<<"Error: part option is not specified"<<endl;
            return false;
        }

        Bottle &group=rf.findGroup(part.c_str());

        if (group.isNull())
        {
            cout<<"Error: unable to locate "<<part<<" definition"<<endl;
            return false;
        }

        if (group.check("name"))
            slvName=group.find("name").asString().c_str();
        else
        {
            cout<<"Error: name option is missing"<<endl;
            return false;
        }

        if ((part=="left_arm") || (part=="right_arm"))
            slv=new iCubArmCartesianSolver(slvName);
        else if ((part=="left_leg") || (part=="right_leg"))
            slv=new iCubLegCartesianSolver(slvName);
        else
        {
            cout<<"Error: "<<part<<" is invalid"<<endl;
            return false;
        }

        if (slv->open(group))
            return true;
        else
        {    
            delete slv;
            return false;
        }
    }

    /************************************************************************/
    bool interruptModule()
    {
        if (slv!=NULL)
            slv->interrupt();

        return true;
    }

    /************************************************************************/
    bool close()
    {
        if (slv!=NULL)
            delete slv;

        return true;
    }

    /************************************************************************/
    double getPeriod()
    {
        return 1.0;
    }

    /************************************************************************/
    bool updateModule()
    {
        if (slv->isClosed() || slv->getTimeoutFlag())
            return false;
        else
            return true;
    }
};


/************************************************************************/
int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        cout<<"YARP server not available!"<<endl;
        return -1;
    }

    ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefaultContext("cartesianSolver");
    rf.setDefaultConfigFile("cartesianSolver.ini");
    rf.configure(argc,argv);

    SolverModule mod;
    return mod.runModule(rf);
}



