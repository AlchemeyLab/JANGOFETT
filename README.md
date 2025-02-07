# JANGOFETT
mkdir build

cd build

cmake -DGEANT4_CUSTOM_PATH=/path/to/geant4 ..  (ex /path/to/Geant4/geant4-v11.2.1)

make

cd ..

cd src

chmod +x run_jangofett.sh

-------------------------------------------------------------------------------------------------
TO RUN CGMF AND GEANT4:

     ./run_jangofett.sh - i 

TO RUN WITH AN EXISTING CGMF SIMULATION:


    ./run_jangofett.sh - f path/to/cgmffile

--------------------------------------------------------------------------------------------------

Upon the first run, you will be prompted to include the path to the cgmf.x executable and the path to the root install folder


Outputs are accessed in the JANGOFETT build folder
