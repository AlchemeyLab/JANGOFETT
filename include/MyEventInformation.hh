#ifndef MY_EVENT_INFORMATION_HH
#define MY_EVENT_INFORMATION_HH

#include "G4VUserEventInformation.hh"
#include "G4ios.hh"

class MyEventInformation : public G4VUserEventInformation {
private:
    int fissionEventID;

public:
    MyEventInformation() : fissionEventID(0) {}
    void SetFissionEventID(int id) { fissionEventID = id; }
    int GetFissionEventID() const { return fissionEventID; }

    void Print() const override {
        G4cout << "Fission Event ID: " << fissionEventID << G4endl;
    }
};

#endif

