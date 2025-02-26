
TO SET UP JANGOFETT
============================================================================================

mkdir build

cd build

cmake -DGEANT4_CUSTOM_PATH=/path/to/Geant4/geant4-version-install

make

cd ..

cd src

chmod +x run_jangofett.sh

--------------------------------------------------------------------------------------------
TO RUN CGMF AND GEANT4:

     ./run_jangofett.sh - i 

TO RUN WITH AN EXISTING CGMF SIMULATION:


    ./run_jangofett.sh - f path/to/cgmffile.cgmf.0

--------------------------------------------------------------------------------------------------

Upon the first run, you will be prompted to include the path to the cgmf.x executable and the path to the root install folder


Outputs are accessed in the JANGOFETT build directory


DEFAULT GEOMETRY EXPLANATION
============================================================================================

The default detector geometry for JANGOFETT utilizes a fission fragment detection array with DSSDs, HPGEs, and BGOs.  



CHANGING SENSITIVE VOLUMES
============================================================================================
This document describes how to modify sensitive volumes (detectors) in the Geant4 simulation. Sensitive volumes are assigned in DetCon.tg and are processed by the Geant4 program. If VolumeID values are changed, or another .tg file is used with different values, additional updates are required in MyEventAction.cc to ensure correct behavior.

--------------------------------------------------------------------------------------------
1. Function to Determine Resolutions Based On Volume
--------------------------------------------------------------------------------------------
A Gaussian blur is applied to hit energy and time to simulate detector resolution limitations. This is defined in MyEventAction.cc (starting at line 40).

    Energy resolution: Defined as a fractional value (e.g., 0.002 for 0.2%).
    Time resolution: Defined in nanoseconds of uncertainty.

Update the VolumeID ranges to match the detector types in your geometry.

--------------------------------------------------------------------------------------------
2. Energy Thresholds
--------------------------------------------------------------------------------------------
Different detectors may have varying minimum energy thresholds for recording hits. These are defined in the energyThreshold assignment in MyEventAction.cc. 

    Default settings:
        DSSDs (Volumes 801 and 802) have a 30 MeV threshold.
        All other detectors have a 20 keV threshold.

Modify if your detector setup requires different thresholds.


--------------------------------------------------------------------------------------------
3. VolumeID Adjustment 
--------------------------------------------------------------------------------------------
In some cases, the VolumeID assigned to a hit may need to be modified. The current implementation adjusts VolumeID values over 10,000 by dividing them by 10 when recording a time-shifted step. This logic is handled in adjustedVolumeID.

Modify or remove this adjustment as needed to match your detector setup.

ACKNOWLEDGEMENTS
============================================================================================
This software runs CGMF (`cgmf.x`) but does not include CGMF’s source code or binaries.  
CGMF is developed by Los Alamos National Laboratory and is licensed under the BSD 3-Clause License.  
Users must install CGMF separately and comply with its licensing terms, found at
https://github.com/lanl/CGMF/blob/main/LICENSE

This software requires a standard Geant4 installation. It does not modify the Geant4 source code but extends its functionality through additional features. Users must install Geant4 separately and comply with its licensing terms.Geant4 is developed by the Geant4 Collaboration and is licensed under the Geant4 Software License Version 1.0  
For more details, visit: [http://cern.ch/geant4/license](http://cern.ch/geant4/license).

============================================================================================

For comments, questions, or feature suggestions, email chemeya@oregonstate.edu or shirel@oregonstate.edu.
