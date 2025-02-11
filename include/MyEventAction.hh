#ifndef MY_EVENT_ACTION_HH
#define MY_EVENT_ACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"
#include "MyEventInformation.hh"
#include <vector>
#include <map>
#include <fstream>
#include "StepData.hh" 


class MyEventAction : public G4UserEventAction {
public:
    MyEventAction();
    virtual ~MyEventAction();

    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);

    void AddStepData(const StepData& step);  
    std::vector<StepData> GetStepDataFromEvent(const G4Event* event) const;

private:
    G4int DSSDstepBig;
    std::map<int, double> fissionTimeShifts;
    std::vector<StepData> stepDataCollection;
    double fissionRate;              
    double experimentalTimeWindow;
};

#endif

