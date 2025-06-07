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

#include "G4LogicalVolume.hh"   // **new – for logical-volume pointer**
#include "G4ThreeVector.hh"     // **new – to store the random vertex**

class G4ParticleGun;
class G4Event;

struct ParticleData {
    int    fissionEventID;            // CSV column 0
    std::string fragmentIdentifier;   // CSV column 1
    int    A;                         // CSV column 2
    int    Z;                         // CSV column 3
    int    neutrons;                  // CSV column 4
    double momentumDirectionX;        // CSV column 5
    double momentumDirectionY;        // CSV column 6
    double momentumDirectionZ;        // CSV column 7
    double kineticEnergy;             // CSV column 8
    double excitationEnergy;          // CSV column 9
    G4ParticleDefinition* definition; // optional cached pointer
};

class MyPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
    MyPrimaryGeneratorAction(const std::string& csvFile);
    ~MyPrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* anEvent) override;
    void SetCurrentParticle(int i);
    int  GetNumberOfParticles() const;
    
    // ←—— Add this helper:
    /// Returns the highest (i.e. total) fission event ID in the CSV.
    int  GetNumberOfFissionEvents() const;

  private:
    // ---------------- existing members ------------------------------------
    int                 currentFissionEventID { -1 };
    int                 particleCount        { 0  };
    G4ParticleGun*      fParticleGun         { nullptr };
    std::vector<ParticleData> csvData;
    int                 currentIndex         { 0 };

    // ---------------- new members (random vertex support) -----------------
    /** Pointer to sample logical-volume (CfCylinder or UFCylinder); nullptr if absent */
    G4LogicalVolume*    fSampleLV   { nullptr };

    /** True when a sample cylinder is present in the geometry */
    G4bool              fHasSample  { false };

    bool    fSampleLookupDone;

    /** Cylinder dimensions (read once at initialisation) */
    G4double            fRadius     { 0.0 };     ///< outer radius  [cm]
    G4double            fHalfZ      { 0.0 };     ///< half-length    [cm]

    /** Vertex reused for both fragments of the same fission event */
    G4ThreeVector       fCurrentVertex { 0.,0.,0. };

    /** Remember last processed event ID to know when to draw a new vertex */
    G4int               fLastEventID { -1 };
};

#endif /* MY_PRIMARY_GENERATOR_ACTION_HH */

