#include "MySteppingAction.hh"
#include "MyRunAction.hh"
#include "MyEventAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh" 

MySteppingAction::MySteppingAction(MyRunAction* runAction)
: G4UserSteppingAction(), runAction(runAction)
{
}

MySteppingAction::~MySteppingAction()
{
}

void MySteppingAction::UserSteppingAction(const G4Step* step)
{
    // Get the track of the current step
    G4Track* track = step->GetTrack();
    int trackID = track->GetTrackID();
    int parentID = track->GetParentID();
    int stepNumber = track->GetCurrentStepNumber();  
    int pdgCode = track->GetDefinition()->GetPDGEncoding();  
    G4ThreeVector position = track->GetPosition();

    // Convert energy deposited to keV
    double edep = step->GetTotalEnergyDeposit() / keV;

    // Convert time to ns
    double t = track->GetGlobalTime() / ns;

    // Get event ID
    int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    // Retrieve the volume copy number and skip if it is <= 0
    G4TouchableHandle handle = step->GetPreStepPoint()->GetTouchableHandle();
    int copyNumber = handle->GetReplicaNumber();
    if (copyNumber <= 0) return;  // Skip recording for copy numbers <= 0

    // Only record steps where energy is deposited
    if (edep > 0) {
        // Store step data in a temporary structure
        StepData stepData;
        stepData.TrackID = trackID;
        stepData.ParentID = parentID;
        stepData.StepNumber = stepNumber;
        stepData.PDGCode = pdgCode;
        stepData.Edep = edep;
        stepData.Time = t;
        stepData.VolumeCopyNumber = copyNumber;

        // Add the step data to the event
        auto eventAction = const_cast<MyEventAction*>(static_cast<const MyEventAction*>(G4RunManager::GetRunManager()->GetUserEventAction()));

if (eventAction) {
    StepData stepData;
    stepData.TrackID = track->GetTrackID();
    stepData.ParentID = track->GetParentID();
    stepData.StepNumber = track->GetCurrentStepNumber();
    stepData.PDGCode = track->GetDefinition()->GetPDGEncoding();
    stepData.Edep = step->GetTotalEnergyDeposit();
    stepData.Time = track->GetGlobalTime();
    stepData.VolumeCopyNumber = handle->GetReplicaNumber();

    eventAction->AddStepData(stepData);
}

    }
}

