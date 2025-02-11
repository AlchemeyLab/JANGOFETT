#include "MyPrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"  
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

// Strip spaces and quotation marks from strings
std::string Strip(const std::string& s) {
    std::string result = s;
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), result.end());
    result.erase(std::remove(result.begin(), result.end(), '"'), result.end());
    return result;
}

// Split by commas
std::vector<std::string> SplitByComma(const std::string &s) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, ',')) {
        tokens.push_back(Strip(token));
    }
    return tokens;
}

MyPrimaryGeneratorAction::MyPrimaryGeneratorAction(const std::string& csvFile)
: fParticleGun(nullptr), currentIndex(0), currentFissionEventID(1), particleCount(0) {
    // Initialize Particle Gun with 1 particle
    fParticleGun = new G4ParticleGun(1);

    // Open and read the CSV file
    std::ifstream file(csvFile);
    if (!file.is_open()) {
        G4cerr << "Error: Could not open CSV file: " << csvFile << G4endl;
        return;
    }
    std::string line;
    // Skip the header row
    std::getline(file, line);

    // Read and parse the CSV file
    while (std::getline(file, line)) {
        std::vector<std::string> columns = SplitByComma(line);

        // Ensure the line has enough columns
        if (columns.size() < 10) {
            G4cerr << "Error: Incorrect number of columns in CSV file." << G4endl;
            continue;
        }

        // Extract the relevant columns
        ParticleData particle;
        particle.A = std::stoi(columns[2]);  // A (mass number)
        particle.Z = std::stoi(columns[3]);  // Z (atomic number)

        // Set the momentum direction vector
        particle.momentumDirectionX = std::stod(columns[5]);
        particle.momentumDirectionY = std::stod(columns[6]);
        particle.momentumDirectionZ = std::stod(columns[7]);

        // Set kinetic and excitation energies
        particle.kineticEnergy = std::stod(columns[8]);  
        particle.excitationEnergy = std::stod(columns[9]);  

        // Store the particle data
        csvData.push_back(particle);
    }

    file.close();
}

MyPrimaryGeneratorAction::~MyPrimaryGeneratorAction() {
    delete fParticleGun;
}

// Set the current particle to be fired from CSV data
void MyPrimaryGeneratorAction::SetCurrentParticle(int index) {
    if (index < csvData.size()) {
        currentIndex = index;  // Set the current index to point to the correct particle
    }
}

// GeneratePrimaries fires the particle gun using the particle at currentIndex
void MyPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent) {
    // Use the event ID to determine which particle to fire
    G4int eventID = anEvent->GetEventID();

    // Ensure enough particles in the CSV file
    if (eventID < csvData.size()) {
        // Get the particle data corresponding to the current event ID
        ParticleData particle = csvData[eventID];

        // Get the ion table from G4ParticleTable
        G4IonTable* ionTable = G4ParticleTable::GetParticleTable()->GetIonTable();

        // Retrieve the ion using Z (atomic number), A (mass number), and excitation energy
        G4ParticleDefinition* ion = ionTable->GetIon(particle.Z, particle.A, particle.excitationEnergy * MeV);

        if (!ion) {
            G4cerr << "Error: Ion with A = " << particle.A << ", Z = " << particle.Z 
                   << " and ExcitationEnergy = " << particle.excitationEnergy << " MeV not found." << G4endl;
            return;  // Stop if the ion cannot be found
        }

        // Set up the particle gun to fire the ion
        fParticleGun->SetParticleDefinition(ion);
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(particle.momentumDirectionX,
                                                                 particle.momentumDirectionY,
                                                                 particle.momentumDirectionZ));
        fParticleGun->SetParticleEnergy(particle.kineticEnergy * MeV);

        // Fire the particle
        fParticleGun->GeneratePrimaryVertex(anEvent);

        // Assign FissionEventID
        int fissionEventID = currentFissionEventID;
        particleCount++;
        if (particleCount % 2 == 0) {
            currentFissionEventID++;
        }

        // Attach FissionEventID to the event
        MyEventInformation* eventInfo = new MyEventInformation();
        eventInfo->SetFissionEventID(fissionEventID);
        anEvent->SetUserInformation(eventInfo);

        // Log the firing process
        //G4cout << "Fired particle with A = " << particle.A << ", Z = " << particle.Z
        //       << ", FissionEventID = " << fissionEventID << G4endl;
    }
}

int MyPrimaryGeneratorAction::GetNumberOfParticles() const {
    return csvData.size();
}

