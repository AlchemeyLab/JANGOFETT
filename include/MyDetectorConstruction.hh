#ifndef MY_DETECTOR_CONSTRUCTION_HH
#define MY_DETECTOR_CONSTRUCTION_HH

#include <G4VUserDetectorConstruction.hh>
#include <string>

class G4VPhysicalVolume;

class MyDetectorConstruction : public G4VUserDetectorConstruction
{
public:
    MyDetectorConstruction();
    ~MyDetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;

private:
    std::string fGeomFile;
};

#endif


