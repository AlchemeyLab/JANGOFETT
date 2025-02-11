#include "MyDetectorConstruction.hh"
#include "G4tgbVolumeMgr.hh"
#include "G4PhysicalVolumeStore.hh"

MyDetectorConstruction::MyDetectorConstruction() : G4VUserDetectorConstruction() {}

MyDetectorConstruction::~MyDetectorConstruction() {}

G4VPhysicalVolume* MyDetectorConstruction::Construct() {
    // Load the geometry from the text file
    G4tgbVolumeMgr* volumeMgr = G4tgbVolumeMgr::GetInstance();
    volumeMgr->AddTextFile("DetCon.tg");
    
    // Read and construct the geometry
    G4VPhysicalVolume* physWorld = volumeMgr->ReadAndConstructDetector();
    
    return physWorld;
}

