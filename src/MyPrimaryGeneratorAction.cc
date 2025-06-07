#include "MyPrimaryGeneratorAction.hh"
#include "MyEventInformation.hh"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4VSolid.hh"
#include "G4Tubs.hh"
#include "Randomize.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>


static std::string Strip(const std::string& s) {
    std::string result = s;
    result.erase(result.begin(), std::find_if(result.begin(), result.end(),
                 [](unsigned char ch){ return !std::isspace(ch); }));
    result.erase(std::find_if(result.rbegin(), result.rend(),
                 [](unsigned char ch){ return !std::isspace(ch); }).base(), result.end());
    result.erase(std::remove(result.begin(), result.end(), '"'), result.end());
    return result;
}
static std::vector<std::string> SplitByComma(const std::string& s) {
    std::vector<std::string> tokens; std::string tok; std::istringstream ss(s);
    while (std::getline(ss,tok,',')) tokens.push_back(Strip(tok));
    return tokens;
}

// ctor
MyPrimaryGeneratorAction::MyPrimaryGeneratorAction(const std::string& csvFile)
 : fParticleGun(nullptr)
 , currentIndex(0)
 , currentFissionEventID(0)
 , particleCount(0)
 , fSampleLV(nullptr)
 , fHasSample(false)
 , fRadius(0.0)
 , fHalfZ(0.0)
 , fLastEventID(-1)
 , fSampleLookupDone(false)
{
    fParticleGun = new G4ParticleGun(1);

    // Read CSV
    std::ifstream file(csvFile);
    if(!file.is_open()){
        G4cerr << "Error: Could not open CSV file: " << csvFile << G4endl;
        return;
    }
    std::string line;
    std::getline(file, line);  // Skip header
    while (std::getline(file, line)) {
        auto c = SplitByComma(line);
        if (c.size() < 9) {
            G4cerr << "CSV format error\n";
            continue;
        }
        ParticleData p;
        p.fissionEventID     = std::stoi(c[0]);
        p.fragmentIdentifier = c[1];
        p.A                  = std::stoi(c[2]);
        p.Z                  = std::stoi(c[3]);
        p.neutrons           = std::stoi(c[4]);
        p.momentumDirectionX = std::stod(c[5]);
        p.momentumDirectionY = std::stod(c[6]);
        p.momentumDirectionZ = std::stod(c[7]);
        p.kineticEnergy      = std::stod(c[8]);
        p.excitationEnergy   = std::stod(c[9]);
        csvData.push_back(p);
    }
    file.close();
}


MyPrimaryGeneratorAction::~MyPrimaryGeneratorAction() {
    delete fParticleGun;
}

void MyPrimaryGeneratorAction::SetCurrentParticle(int i) {
    if (i < static_cast<int>(csvData.size())) {
        currentIndex = i;
    }
}


void MyPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent) {
     // One-time lookup of Cf or U cylinder after geometry has been built:
    if (!fSampleLookupDone) {
        fSampleLookupDone = true;
        const std::vector<G4String> candidateNames = { "CfCylinder", "UFCylinder" };
        auto* lvs = G4LogicalVolumeStore::GetInstance();
        for (auto const& name : candidateNames) {
            if (auto lv = lvs->GetVolume(name, false)) {
                fSampleLV  = lv;
                fHasSample = true;
                if (auto tubs = dynamic_cast<G4Tubs*>(lv->GetSolid())) {
                    fRadius = tubs->GetOuterRadius() / cm;
                    fHalfZ  = tubs->GetZHalfLength() / cm;
                } else {
                    // not a tube → disable random vertex
                    fHasSample = false;
                }
                break;
            }
        }
    }


    if (currentIndex >= static_cast<int>(csvData.size())) return;
    int thisEventID = csvData[currentIndex].fissionEventID;

     // Choose a random vertex once per fission event:
    if (fHasSample && thisEventID != fLastEventID) {
        G4double u   = G4UniformRand();
        G4double v   = G4UniformRand();
        G4double r   = fRadius * std::sqrt(u);
        G4double phi = CLHEP::twopi * v;
        G4double z   = (2*G4UniformRand() - 1.0) * fHalfZ;
        fCurrentVertex = G4ThreeVector(
            r * std::cos(phi) * cm,
            r * std::sin(phi) * cm,
            z * cm
        );
        fLastEventID = thisEventID;
    }

    // Fire one primary per CSV row of this event:
    while (currentIndex < static_cast<int>(csvData.size()) &&
           csvData[currentIndex].fissionEventID == thisEventID)
    {
        const auto& p = csvData[currentIndex];

        // Choose particle type 
        if (p.fragmentIdentifier == "Gamma" || p.fragmentIdentifier == "gamma") {  // Although CGMF gammas are currently not parsed, the functionality has been implemented to manage them in a fission event
            // gamma
            fParticleGun->SetParticleDefinition(
                G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
                fParticleGun->SetParticleCharge(0.0);
            fParticleGun->SetParticleMomentumDirection({
                p.momentumDirectionX,
                p.momentumDirectionY,
                p.momentumDirectionZ
            });
            fParticleGun->SetParticleEnergy(p.kineticEnergy * MeV);

        } else if (p.fragmentIdentifier == "Neutron") {
            // neutron
            fParticleGun->SetParticleDefinition(
                G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
                fParticleGun->SetParticleCharge(0.0);
            fParticleGun->SetParticleMomentumDirection({
                p.momentumDirectionX,
                p.momentumDirectionY,
                p.momentumDirectionZ
            });
            fParticleGun->SetParticleEnergy(p.kineticEnergy * MeV);

                } else {
            // excited ion
            auto ion = G4ParticleTable::GetParticleTable()
                           ->GetIonTable()
                           ->GetIon(
                                p.Z,
                                p.A,
                                p.excitationEnergy * MeV   // <-- set nuclear excitation
                           );
            if (!ion) {
                G4cerr << "Ion not found A=" << p.A
                       << " Z=" << p.Z << G4endl;
                ++currentIndex;
                continue;
            }

            // set up ion
            fParticleGun->SetParticleDefinition(ion);

            // fully-stripped ion: charge = +Z·e
            fParticleGun->SetParticleCharge(p.Z * eplus);

            // direction and energy
            fParticleGun->SetParticleMomentumDirection({
                p.momentumDirectionX,
                p.momentumDirectionY,
                p.momentumDirectionZ
            });
            fParticleGun->SetParticleEnergy(p.kineticEnergy * MeV);
        }


        //set the vertex
        if (fHasSample) {
            fParticleGun->SetParticlePosition(fCurrentVertex);
        } else {
            fParticleGun->SetParticlePosition(G4ThreeVector(0,0,0));
        }

        /*       G4cout
      << "DEBUG: Firing particle: "
      << p.fragmentIdentifier
      << "  (A=" << p.A << ", Z=" << p.Z << ")"
      << "  KE=" << p.kineticEnergy << " MeV"
      << "  Exc=" << p.excitationEnergy  << " MeV"
      << G4endl;
    
    */

    // Fire
    fParticleGun->GeneratePrimaryVertex(anEvent);
    ++currentIndex;
    }

    // Attach event ID info:
    auto* evtInfo = new MyEventInformation();
    evtInfo->SetFissionEventID(thisEventID);
    anEvent->SetUserInformation(evtInfo);
}

int MyPrimaryGeneratorAction::GetNumberOfFissionEvents() const {
    int maxID = 0;
    for (const auto& p : csvData) {
        maxID = std::max(maxID, p.fissionEventID);
    }
    return maxID;
}

int MyPrimaryGeneratorAction::GetNumberOfParticles() const {
    return static_cast<int>(csvData.size());
}

