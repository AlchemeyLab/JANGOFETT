//1.0
#include "MyEventAction.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "Hit.hh"
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>
#include <iomanip>
#include <limits>
#include <fstream>
#include <unistd.h>
#include <limits.h>

// Function to apply Gaussian blur to energy
double applyGaussianBlurE(double energy, double energyfwhm) {
    static std::default_random_engine generator;
    double sigma = energy * energyfwhm /(2.35482); // Convert FWHM to sigma
    std::normal_distribution<double> distribution(energy, sigma);
    return distribution(generator);
}

// Function to apply Gaussian blur to time
double applyGaussianBlurT(double time, double fwhmns) {
    static std::default_random_engine generator;
    double sigmans = fwhmns/2.35482; //Convert FWHM ns time to sigma ns
    std::normal_distribution<double> distribution(0, sigmans);
    return distribution(generator) + time;
}

// Struct to hold resolution parameters
struct Resolution {
    double timeResolution; // in nanoseconds
    double energyResolution; // as a fraction
};

// Function to get resolutions based on volume ID
Resolution getResolutionForVolume(int volumeID) {
    // Define resolutions for each volume ID range
    if (volumeID >= 1 && volumeID <= 21) {
        return {2.0, 0.002}; // Time: 2 ns, Energy: 0.2%
    } else if (volumeID >= 2001 && volumeID <= 2021) {
        return {5.0, 0.002}; // Time: 5 ns, Energy: 0.2%
    } else if (volumeID >= 1001 && volumeID <= 1022) { //Original volumeID is >10,000 but this is divided by 10 prior to usage of this logic
        return {5.0, 0.002}; // Time: 5 ns, Energy: 0.2%
    } else if ((volumeID >= 601 && volumeID <= 621) || 
               (volumeID >= 1601 && volumeID <= 1621) || 
               (volumeID >= 2601 && volumeID <= 2621)) {
        return {1.3, 0.24}; // Time: 1.3 ns, Energy: 24%
    } else if (volumeID >= 801 && volumeID <= 802) {
        return {2.0, 0.004}; // Time: 2 ns, Energy: 0.4%
    } else {
        // Default resolution if volumeID does not match any range
        return {5.0, 0.002}; // Default Time: 5 ns, Energy: 0.2%
    }
}


std::vector<Hit> allHits;

//Use the fission rate and number of events input by the user to calculate the time shifts
MyEventAction::MyEventAction() : G4UserEventAction() {
    DSSDstepBig = 0;
    double defaultRate = 10000.0;
    const char* envRate = std::getenv("MY_FISSION_RATE");
    if(envRate) {
        double userRate = std::atof(envRate);
        if(userRate > 0.0) {
            fissionRate = userRate;
        } else {
            fissionRate = defaultRate;
        }
    } else {
        fissionRate = defaultRate;
    }

    double defaultCount = 5000.0; 
    const char* envCount = std::getenv("MY_FISSION_COUNT");
    double fissionCount;
    if (envCount) {
        double userCount = std::atof(envCount);
        if (userCount > 0.0) {
            fissionCount = userCount;
        } else {
            fissionCount = defaultCount;
        }
    } else {
        fissionCount = defaultCount;
    }

    // Current experimental time window is the time for all fissions to occur, plus 10 seconds to allow for additional decay
    experimentalTimeWindow = (fissionCount / fissionRate) * 1e9 + 10e9;

    
    std::ifstream timeFile("../src/time_shifts.txt");
    if (timeFile.is_open()) {
        double timeShift;
        int fissionEventID = 1;
        while (timeFile >> timeShift) {
            fissionTimeShifts[fissionEventID] = timeShift;
            fissionEventID++;
        }
        timeFile.close();
    }
}

MyEventAction::~MyEventAction() {}

void MyEventAction::BeginOfEventAction(const G4Event* event) {
    stepDataCollection.clear();
}

void MyEventAction::EndOfEventAction(const G4Event* event) {
    // Retrieve the raw step data (unshifted times)
    const auto& stepData = GetStepDataFromEvent(event);

    // Determine the fission event ID
    int fissionEventID = -1;
    const MyEventInformation* eventInfo =
        static_cast<const MyEventInformation*>(event->GetUserInformation());
    if (eventInfo) {
        fissionEventID = eventInfo->GetFissionEventID();
        //G4cout<<"fission event ID: "<<fissionEventID<<G4endl;
    }

    // 1) Shift times for all steps and discard out-of-window or zero-edep steps
    //    Temporarily store them as Hits, but *only* with the shifted time.
    std::vector<Hit> shiftedHits;
    shiftedHits.reserve(stepData.size()); 

    for (const auto& step : stepData) {
        if (step.Edep <= 0) continue;  // Discard zero-dep steps

        // Apply the time shift associated with the step's fissionEventID
        double time = step.Time;
        if (fissionTimeShifts.find(fissionEventID) != fissionTimeShifts.end()) {
            double shiftNs = fissionTimeShifts.at(fissionEventID) * 1e9; // s → ns
            time += shiftNs;
        }

        // Discard step if outside the total experimental time window
        if (time > experimentalTimeWindow) continue;

        // VolumeID adjustment for preset geometry
        int adjustedVolumeID = (step.VolumeCopyNumber > 10000)
                               ? step.VolumeCopyNumber / 10
                               : step.VolumeCopyNumber;

        // Create a Hit structure with time-shifted data
        Hit h;
        h.volumeID   = adjustedVolumeID;
        h.totalEdep  = step.Edep;
        h.hitTime    = time;

        //if(adjustedVolumeID/10==80&&step.Edep>30.0){DSSDstepBig++;}
        shiftedHits.push_back(h);
    }

    // 2) Group the shifted hits by detector volume
    std::map<int, std::vector<Hit>> detectorHitsMap;
    for (const auto& h : shiftedHits) {
        detectorHitsMap[h.volumeID].push_back(h);
    }

    // 3) For each volume, sort steps by time and merge steps that are within 20 ns into hits 
    std::ofstream hitFile("../src/hits_temp.txt", std::ios::app);
    if (!hitFile.is_open()) {
        G4cerr << "Error: Could not open hits_temp.txt for appending!" << G4endl;
        return;
    }

    for (auto& [volumeID, hits] : detectorHitsMap) {
        // Sort in ascending time
        std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) {
            return a.hitTime < b.hitTime;
        });

        // Prepare container for final merged hits
        std::vector<Hit> mergedHits;
        mergedHits.reserve(hits.size());

        // Merge hits within 20 ns of the earliest in a cluster
        for (size_t i = 0; i < hits.size(); ++i) {
            double summedEnergy   = hits[i].totalEdep;
            double earliestTime   = hits[i].hitTime;
            size_t j = i + 1;

            while (j < hits.size() && (hits[j].hitTime - earliestTime) <= 20.0) { // Modify this value to modify the "hit" time window
                summedEnergy += hits[j].totalEdep;
                ++j;
            }
           
            int volumeID = hits[i].volumeID;

            // Get the appropriate resolutions for the given volumeID
            Resolution res = getResolutionForVolume(volumeID);
            //Apply energy blurring
            hits[i].totalEdep = applyGaussianBlurE(hits[i].totalEdep, res.energyResolution);

       
   

            // Decide which threshold to apply
            double energyThreshold = (volumeID == 801 || volumeID == 802)
                                     ? 30.0  // 30 MeV for DSSD
                                     : 0.020; // 20 keV for other volumes

            // Check if the merged deposit exceeds threshold
            if (summedEnergy >= energyThreshold) {
                Hit finalHit;
                finalHit.volumeID   = volumeID;
                double blurredEnergy = applyGaussianBlurE(summedEnergy, res.energyResolution);
                finalHit.totalEdep  = blurredEnergy;

                // Apply time blurring
                finalHit.hitTime = applyGaussianBlurT(earliestTime, res.timeResolution);


                // Discard if negative time after blur
                if (finalHit.hitTime >= 0) {
                    mergedHits.push_back(finalHit);
                }
            } 
      
            
            i = j - 1; // skip past merged block
        }

        // Write all merged hits to the temp file
        for (const auto& hit : mergedHits) {
            hitFile <<hit.volumeID << " "
                    << std::fixed << std::setprecision(std::numeric_limits<double>::max_digits10) <<hit.totalEdep << " "
                    << std::fixed << std::setprecision(std::numeric_limits<double>::max_digits10) <<hit.hitTime   << "\n";
        }
    }
 
    hitFile.close();
}

std::vector<StepData> MyEventAction::GetStepDataFromEvent(const G4Event* event) const {
    return stepDataCollection;
}

void MyEventAction::AddStepData(const StepData& step) {
    stepDataCollection.push_back(step);
}

