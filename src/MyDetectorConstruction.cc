#include "MyDetectorConstruction.hh"
 
#include "G4tgbVolumeMgr.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"
#include "G4Exception.hh"
 
#include <cstdlib>   
#include <string>
 
MyDetectorConstruction::MyDetectorConstruction()
: G4VUserDetectorConstruction()
{
    if ( const char* tg = std::getenv("G4_TG_FILE") ) {
        fGeomFile = tg;
    } else {
        fGeomFile = "DetCon.tg";
        G4cout << "[MyDetectorConstruction] WARNING: "
<< "G4_TG_FILE not set, using default: "
<< fGeomFile << G4endl;
    }
}
 
MyDetectorConstruction::~MyDetectorConstruction() = default;
 
G4VPhysicalVolume* MyDetectorConstruction::Construct()
{
    // Clean up any previous geometry (in case of re-init)
    G4PhysicalVolumeStore::GetInstance()->Clean();
    G4LogicalVolumeStore ::GetInstance()->Clean();
    G4SolidStore        ::GetInstance()->Clean();
 
    // Tell the TGB reader which file to load
    auto* volMgr = G4tgbVolumeMgr::GetInstance();
    volMgr->AddTextFile(fGeomFile);


    // Build it
    G4VPhysicalVolume* world = volMgr->ReadAndConstructDetector();

    if (!world) {
        G4Exception("MyDetectorConstruction::Construct()",
                    "GeomBuildErr", FatalException,
                    ("Failed to build geometry from " + fGeomFile).c_str());
    }
    return world;
}
