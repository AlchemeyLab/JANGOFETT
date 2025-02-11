#include "MyRunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "Hit.hh"
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <cmath>

extern std::vector<Hit> allHits;

static double ApplyGaussianBlur(double value, double fwhm) {
    static std::default_random_engine generator;
    double sigma = value * fwhm / 2.35482;
    std::normal_distribution<double> distribution(value, sigma);
    return distribution(generator);
}

MyRunAction::MyRunAction() : G4UserRunAction() {
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->CreateNtuple("HitData", "Combined Hit Data");
    analysisManager->CreateNtupleIColumn("VolumeID");      
    analysisManager->CreateNtupleDColumn("HitTime");      
    analysisManager->CreateNtupleDColumn("TotalEdep");     
    analysisManager->FinishNtuple();
}

MyRunAction::~MyRunAction() {}

void MyRunAction::BeginOfRunAction(const G4Run*) {
    auto analysisManager = G4AnalysisManager::Instance();
    G4cout << "Opening ROOT file..." << G4endl;
    analysisManager->OpenFile("setup.root");
}

void MyRunAction::EndOfRunAction(const G4Run*) {

    int DSSDHits = 0;
    // Read all hits from the temporary file
    std::ifstream hitFile("../src/hits_temp.txt");
    std::map<int, std::vector<Hit>> detectorHitsMap;

    if (!hitFile.is_open()) {
        G4cerr << "Error: Could not open hits_temp.txt for reading!" << G4endl;
        return;
    }

    int volumeID;
    double edep, time;

    while (hitFile >> volumeID >> edep >> time) {
        if (time < 0) continue; // Discard hits with negative time

        Hit hit;
        hit.volumeID  = volumeID;
        hit.totalEdep = edep; 
        hit.hitTime   = time;  

        detectorHitsMap[volumeID].push_back(hit);
    }

    hitFile.close();

    // Show how many hits were collected (unmerged) per volume
    /* for (const auto& [volID, hits] : detectorHitsMap) {
        G4cout << "Detector " << volID
               << " | Hits Before Run-Level Merging: "
               << hits.size() << G4endl;
    }

    */

    // WStore final merged hits for all volumes here
    std::vector<Hit> mergedHits;

    std::vector<Hit> tempHits;
    // Det volume by volume
    for (auto& [volID, hits] : detectorHitsMap) {
        // Sort hits in ascending order of time
        std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) {
            return a.hitTime < b.hitTime;
        });

        //G4cout << "Detector " << volID
              // << " | Hits Before Merging: " << hits.size() << G4endl;

        // Merge hits within 20 ns of the earliest in a group
        for (size_t i = 0; i < hits.size(); ++i) {
            double summedEnergy  = hits[i].totalEdep;
            double earliestTime  = hits[i].hitTime;  // Start with the first hit's time
            size_t j            = i + 1;
            bool merged         = false;

            // Merge any subsequent hits that are within 20 ns of earliestTime
            while (j < hits.size() && (hits[j].hitTime - earliestTime) <= 20.0) {
                if (volID/10 == 80) {
                    //G4cout << "test hit loop while loop: " << j
                          // << " time dif: " << (int)hits[j].hitTime 
                          // << " : " << (int)earliestTime << G4endl;
                }
                summedEnergy += hits[j].totalEdep;
                merged       = true;
                ++j;
            }

            if (earliestTime >= 0) {
                tempHits.push_back((Hit){volID, summedEnergy, earliestTime});
            }

            // Jump past the merged block
            i = j - 1;
        }

        // Add this volume's merged hits to the global list
        mergedHits.insert(mergedHits.end(), tempHits.begin(), tempHits.end());
        tempHits.clear();
    }

    // Sort all merged hits across volumes by time from earliest to latest 
    std::sort(mergedHits.begin(), mergedHits.end(),
              [](const Hit& a, const Hit& b) {
                  return a.hitTime < b.hitTime;
              });
   

    // Write the merged data to ROOT ntuple
    auto analysisManager = G4AnalysisManager::Instance();
    for (const auto& hit : mergedHits) {
        if (hit.volumeID / 10 == 80) {
            DSSDHits++;
        }
        analysisManager->FillNtupleIColumn(0, hit.volumeID);
        analysisManager->FillNtupleDColumn(1, hit.hitTime);
        analysisManager->FillNtupleDColumn(2, hit.totalEdep);
        analysisManager->AddNtupleRow();
    }

    //G4cout << "Number of DSSD Hits: " << DSSDHits << G4endl;
    G4cout << "Run Completed" << G4endl;

    analysisManager->Write();
    analysisManager->CloseFile();

    // Remove the temp file if you no longer need it
    std::remove("../src/hits_temp.txt");
}


