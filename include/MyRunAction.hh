#ifndef MyRunAction_h
#define MyRunAction_h 1

#include "G4UserRunAction.hh"

class MyRunAction : public G4UserRunAction {
public:
    MyRunAction();
    virtual ~MyRunAction();

    virtual void BeginOfRunAction(const G4Run*);
    virtual void EndOfRunAction(const G4Run*);
};

#endif

