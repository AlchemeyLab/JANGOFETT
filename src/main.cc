#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "QGSP_BERT.hh"
#include "MyPrimaryGeneratorAction.hh"
#include "MyDetectorConstruction.hh"
#include "MyRunAction.hh"
#include "MyEventAction.hh"
#include "MySteppingAction.hh"
#include "G4DeexPrecoParameters.hh"
#include "G4NuclearLevelData.hh"
#include "G4RadioactiveDecayPhysics.hh"

int main(int argc, char** argv) {
    // Retrieve the de-excitation parameters
    G4DeexPrecoParameters* deexParameters = G4NuclearLevelData::GetInstance()->GetParameters();
    deexParameters->SetMaxLifeTime(3e6 * CLHEP::second);  // 10^6 seconds

    // Construct the default run manager (single-threaded)
    G4RunManager* runManager = new G4RunManager();

    // Mandatory user initialization classes
    runManager->SetUserInitialization(new MyDetectorConstruction());
    G4VModularPhysicsList* physicsList = new QGSP_BERT;

    // Radioactive decay physics
    physicsList->RegisterPhysics(new G4RadioactiveDecayPhysics);
    runManager->SetUserInitialization(physicsList);
    runManager->Initialize();

    // User action objects
    MyRunAction* runAction = new MyRunAction();
    MyPrimaryGeneratorAction* primaryGen = new MyPrimaryGeneratorAction(argv[1]);

    // User action classes
    runManager->SetUserAction(primaryGen);
    runManager->SetUserAction(runAction);
    runManager->SetUserAction(new MyEventAction());
    runManager->SetUserAction(new MySteppingAction(runAction));  // Pass runAction

    // Initialize G4 kernel
    runManager->Initialize();

    // Instead of firing one particle at a time inside the loop, pass the total number of particles
    int totalParticles = primaryGen->GetNumberOfParticles();
    runManager->BeamOn(totalParticles);  // Fire all particles in one run

    // Clean up
    delete runManager;
    return 0;
}

