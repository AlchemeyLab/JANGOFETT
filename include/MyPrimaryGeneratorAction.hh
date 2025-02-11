#ifndef MY_PRIMARY_GENERATOR_ACTION_HH
#define MY_PRIMARY_GENERATOR_ACTION_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"
#include <vector>
#include <string>
#include "G4ParticleDefinition.hh"
#include "G4IonTable.hh"  
#include "G4VUserEventInformation.hh"
#include "G4ios.hh"
#include "MyEventInformation.hh"


class G4ParticleGun;
class G4Event;

struct ParticleData {
    int A;  // Mass number
    int Z;  // Atomic number
    double momentumDirectionX;
    double momentumDirectionY;
    double momentumDirectionZ;
    double kineticEnergy;
    double excitationEnergy;
    G4ParticleDefinition* definition;  // Pointer to particle definition
};

class MyPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
  public:
    void SetCurrentParticle(int index);
    MyPrimaryGeneratorAction(const std::string& csvFile);  // Constructor with CSV file parameter
    virtual ~MyPrimaryGeneratorAction();

    virtual void GeneratePrimaries(G4Event*);

    // Add this line to declare the missing function
    int GetNumberOfParticles() const;

  private:
    int currentFissionEventID;  // Tracks the current fission event ID
    int particleCount;          // Counts particles fired within a fission event
    G4ParticleGun* fParticleGun; 
    std::vector<ParticleData> csvData;
    int currentIndex;
};

#endif

