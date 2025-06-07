JANGOFETT v1.01 UPDATES
============================================================================================
* Added X-ray florescence. 
* Primary fission products (PFPs) are simulated as ionized. (neutral in v1.0)
* Added timer to track JANGOFETT run time.
* Added ability to use either a pre-parsed .csv file or a .cgmf.0 output file.
* Added ability to shift Geant4 root output so prior simulations with appropriate event IDs and timestamps may be shifted into an experimental timeline.
* Added option to include evaporated neutrons in Geant4 simulation.    
* Added option to include 235U or 252Cf in simulation geometry as a cylindrical target/source. Radius and thickness may be modified by the user. 
  Fissioning source is set on a 1.5 cm x 1.5 cm Carbon backing, 50 nm thick. When a source is present, fission events will originate from random
  coordinates within the cylindrical volume. 
    * 235U geometry assumes vapor deposited UF4.
    * 252Cf geometry assumes electrodeposited CfC–H₂O, where for each Cf atom there are 500 C atoms and 200 H2O molecules. 


TO SET UP JANGOFETT
============================================================================================

mkdir build

cd build

cmake -DGEANT4_CUSTOM_PATH=/path/to/Geant4/geant4-version-install ..

make

cd ..

cd src

chmod +x run_jangofett.sh

--------------------------------------------------------------------------------------------
TO RUN CGMF AND GEANT4:

     ./run_jangofett.sh - ci 

TO RUN GEANT4 WITH AN EXISTING CGMF SIMULATION:

    ./run_jangofett.sh - cf path/to/cgmffile.cgmf.0

TO RUN GEANT4 WITH AN EXISTING PARSED CSV FILE:
    
    ./run_jangofett.sh -csv path/to/csvfile.csv

TO APPLY AN EXPERIMENTAL TIMELINE TO AN EXISTING GEANT4 OUTPUT:

    ./run_jangofett.sh -rt path/to/output.root
    
    In order to execute properly, the root output will need the timestamps of the recorded step/hit data, 
    as well as tracks on the event ID in reference to the all primaries/secondaries from a single fission event.
    
    The original timestamps will be shifted chronologically according to their eventID. 
    

--------------------------------------------------------------------------------------------------

Upon the first run, you will be prompted to include the path to the cgmf.x executable and the path to the root install folder


Outputs are accessed in the JANGOFETT build directory

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
