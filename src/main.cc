#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4PhysListFactory.hh" 
#include "MyPrimaryGeneratorAction.hh"
#include "MyDetectorConstruction.hh"
#include "MyRunAction.hh"
#include "MyEventAction.hh"
#include "MySteppingAction.hh"
#include "G4DeexPrecoParameters.hh"
#include "G4NuclearLevelData.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4UAtomicDeexcitation.hh"
#include "G4LossTableManager.hh"
#include "G4SystemOfUnits.hh"  

int main(int argc, char** argv) {

    // Retrieve de-excitation parameters and adjust if necessary
    G4DeexPrecoParameters* deexParameters = G4NuclearLevelData::GetInstance()->GetParameters();
    deexParameters->SetMaxLifeTime(3e6 * second);  // Example: 3e6 seconds

    // Construct the run manager
    G4RunManager* runManager = new G4RunManager();

    // Set the detector construction
    runManager->SetUserInitialization(new MyDetectorConstruction());

    // Create and configure the physics list
    G4PhysListFactory physFactory;
  auto physicsList = physFactory.GetReferencePhysList("QGSP_BERT_HPT");
  if (!physicsList) {
    G4cerr << "ERROR: QGSP_BERT_HPT not available!\n";
    return 1;
  }
  runManager->SetUserInitialization(physicsList);

    // Configure atomic deexcitation
    G4UAtomicDeexcitation* atomDeexcitation = new G4UAtomicDeexcitation();
    atomDeexcitation->SetFluo(true);   // Enable fluorescence (x‑ray emission)
    //atomDeexcitation->SetAuger(true);   // Enable Auger electrons (optional)
    G4LossTableManager::Instance()->SetAtomDeexcitation(atomDeexcitation);

    // Register user action classes before initialization
    MyPrimaryGeneratorAction* primaryGen = new MyPrimaryGeneratorAction(argv[1]);
    MyRunAction* runAction = new MyRunAction();
    runManager->SetUserAction(primaryGen);
    runManager->SetUserAction(runAction);
    runManager->SetUserAction(new MyEventAction());
    runManager->SetUserAction(new MySteppingAction(runAction));  

    // Initialize the run manager once all user initialization is complete
    runManager->Initialize();

    // Fire the beam: get total number of particles from your generator
    int totalFissionEvents = primaryGen->GetNumberOfFissionEvents();
    runManager->BeamOn(totalFissionEvents);

    // Clean up
    delete runManager;
    return 0;
}

