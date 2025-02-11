#ifndef MY_STEPPING_ACTION_HH
#define MY_STEPPING_ACTION_HH
#include "MyEventAction.hh" 
#include "G4UserSteppingAction.hh"
#include "globals.hh"

class MyRunAction;

class MySteppingAction : public G4UserSteppingAction {
public:
    MySteppingAction(MyRunAction* runAction);
    virtual ~MySteppingAction();

    virtual void UserSteppingAction(const G4Step*);

private:
    MyRunAction* runAction;

    // Declare member variables
    int trackID;
    int parentID;
    int eventID;
    int copyNumber;  // Volume copy number
    int stepNumber;  // Step number 
    int pdgCode;     // PDG encoding of the particle
    double x, y, z;  // World coordinates of the step
    double edep;     // Energy deposited
    double t;        // Time (global time in ns)
};

#endif  // MY_STEPPING_ACTION_HH

