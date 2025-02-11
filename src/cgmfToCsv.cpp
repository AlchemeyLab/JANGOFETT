#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iomanip> 
#include <cmath> 

// Trim leading and trailing spaces from a string
std::string trim(const std::string &s) {
    auto start = s.find_first_not_of(" \t");
    auto end = s.find_last_not_of(" \t");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// Determine if a line starts with two integers (fragment information)
bool startsWithTwoInts(const std::string &line) {
    std::string trimmedLine = trim(line);
    std::istringstream iss(trimmedLine);
    int firstInt, secondInt;
    return (iss >> firstInt && iss >> secondInt);  // If both integers can be extracted, return true
}

// Parse fragment data (A, Z, neutrons, U, and final kinetic energy KEF)
bool parseFragmentLine(const std::string& line, int& A, int& Z, int& neutrons, double& U, double& KEI, double& KEF) {
    std::istringstream iss(trim(line));
    double J;
    int P, gammas, prompt_neutrons;
    if (iss >> A >> Z >> U >> J >> P >> KEI >> KEF >> neutrons >> gammas >> prompt_neutrons) {
        return true;
    }
    return false;
}

// Parse neutron kinetic energy (sum every fourth number)
double parseNeutronKineticEnergy(const std::string& line, int numNeutrons) {
    std::istringstream iss(trim(line));
    double neutronKineticEnergySum = 0.0;
    double value;

    for (int i = 0; i < numNeutrons; ++i) {
        // Skip the first three initial (pre neutron evaporation) momentum numbers (x, y, z) and read the fourth value as the neutron kinetic energy
        for (int j = 0; j < 3; ++j) {
            iss >> value;  
        }

        double neutronKineticEnergy;
        if (iss >> neutronKineticEnergy) {
            neutronKineticEnergySum += neutronKineticEnergy;
        }
    }

    return neutronKineticEnergySum;
}


// Process each fission fragment and calculate excitation energy
bool processFragment(std::ifstream &infile, std::ofstream &outfile, int fissionEvent, int fragmentNumber,
                     const std::string& fragmentLine, const std::map<std::pair<int, int>, double>& massExcessMap) {
    std::string line;

    // Parse the fragment data (A, Z, neutrons, U, initial kinetic energy, and final kinetic energy)
    int A, Z, neutrons;
    double U, KEI, KEF;  // KEI is initial kinetic energy, KEF is final kinetic energy
    if (!parseFragmentLine(fragmentLine, A, Z, neutrons, U, KEI, KEF)) {
        std::cerr << "Error parsing fragment line for event " << fissionEvent << ", fragment " << fragmentNumber << "\n";
        return false;  // Failed to parse fragment data
    }
    /*
    // Debug: Log the parsed values
    std::cout << "Processing Fission Event " << fissionEvent << ", Fragment " << fragmentNumber << "\n";
    std::cout << "A: " << A << ", Z: " << Z << ", Neutrons: " << neutrons << ", U: " << U << "\n";
    std::cout << "KEI: " << KEI << ", KEF: " << KEF << "\n";
    */

    // Read the momentum line to extract the final momentum vectors (last three numbers)
    double px_final, py_final, pz_final;
    if (!std::getline(infile, line)) {
        std::cerr << "Error reading momentum line for event " << fissionEvent << ", fragment " << fragmentNumber << "\n";
        return false;
    }

    std::istringstream momentumStream(trim(line));
    double px_initial, py_initial, pz_initial;  
    if (!(momentumStream >> px_initial >> py_initial >> pz_initial >> px_final >> py_final >> pz_final)) {
        std::cerr << "Error parsing final momentum vectors for event " << fissionEvent << ", fragment " << fragmentNumber << "\n";
        return false;
    }

    //std::cout << "Final momentum: px = " << px_final << ", py = " << py_final << ", pz = " << pz_final << "\n";

    // If there are no neutrons, skip reading neutron information
    if (neutrons == 0) {
        // Set excitation energy to U (initial excitation energy) and KEF to KEI, since there are no neutrons
        double excitationEnergy = U;
        KEF = KEI;

        // Round the excitation energy to two decimal places
        excitationEnergy = std::round(excitationEnergy * 100.0) / 100.0;

        // Write the data to the output, including momentum in the specified column order
        outfile << fissionEvent << "," << fragmentNumber << "," << (A - neutrons) << "," << Z << "," << neutrons << ","
                << px_final << "," << py_final << "," << pz_final << "," << KEF << "," << excitationEnergy << "\n";
        return true;
    }

    // Skip the first neutron information line (unused)
    if (!std::getline(infile, line)) {
        std::cerr << "Error reading first neutron line for event " << fissionEvent << ", fragment " << fragmentNumber << "\n";
        return false;
    }

    // Read the second neutron information line
    if (!std::getline(infile, line)) {
        std::cerr << "Error reading second neutron line for event " << fissionEvent << ", fragment " << fragmentNumber << "\n";
        return false;
    }

    // Parse the neutron kinetic energy by summing every fourth number
    double neutronKineticEnergySum = parseNeutronKineticEnergy(line, neutrons);

    // Get mass excess for the fragment and primary fission product
    bool foundFragmentMass = false, foundPrimaryMass = false;
    double fragmentMassExcess = 0.0, primaryMassExcess = 0.0;

    auto fragmentKey = std::make_pair(Z, A);
    auto primaryKey = std::make_pair(Z, A - neutrons);  // Now using primary fission product A and Z

    if (massExcessMap.count(fragmentKey)) {
        fragmentMassExcess = massExcessMap.at(fragmentKey);
        foundFragmentMass = true;
    }

    if (massExcessMap.count(primaryKey)) {
        primaryMassExcess = massExcessMap.at(primaryKey);
        foundPrimaryMass = true;
    }

    // Calculate new excitation energy based on the formula
    double excitationEnergy = KEI + U + (A * 931.5 + fragmentMassExcess)
                             - (neutrons * 939.6)
                             - ((A - neutrons) * 931.5 + primaryMassExcess)
                             - KEF
                             - neutronKineticEnergySum;

    // Ensure excitation energy is not negative
    if (excitationEnergy < 0) {
        std::cerr << "Warning: Negative excitation energy for event " << fissionEvent << ", fragment " << fragmentNumber << ". Clamping to zero.\n";
        excitationEnergy = 0;
    }

    // Round the excitation energy to two decimal places
    excitationEnergy = std::round(excitationEnergy * 100.0) / 100.0;

    // Write CSV data, now including final momentum information and reordering the columns as requested
    outfile << fissionEvent << "," << fragmentNumber << "," << (A - neutrons) << "," << Z << "," << neutrons << ","
            << px_final << "," << py_final << "," << pz_final << "," << KEF << "," << excitationEnergy << "\n";

    return true;
}


// Function to process the entire file
void processFile(const std::string &inputFilename, const std::string &outputFilename, const std::map<std::pair<int, int>, double> &massExcessMap) {
    std::ifstream infile(inputFilename);
    std::ofstream outfile(outputFilename);

    if (!infile.is_open()) {
        std::cerr << "Error: Could not open input file: " << inputFilename << "\n";
        return;
    }

    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open output file: " << outputFilename << "\n";
        return;
    }

    // Write CSV header in the new order
    outfile << "FissionEvent,FragmentNumber,PrimaryA,Z,Neutrons,Px,Py,Pz,KEF,ExcitationEnergy\n";

    std::string line;
    int eventIndex = 0;
    int fragmentIndex = 0;

    // Process the input file
    while (std::getline(infile, line)) {
        if (line[0] == '#') {  // Skip comment lines
            continue;
        }

        if (startsWithTwoInts(line)) {
            // Process the fragment (fragment 1 or fragment 2)
            fragmentIndex++;
            if (fragmentIndex == 1) {
                eventIndex++;  // Only increment the event index when processing the first fragment
            }

            // Call processFragment for the current fragment
            if (!processFragment(infile, outfile, eventIndex, fragmentIndex, line, massExcessMap)) {
                std::cerr << "Error processing fragment in event " << eventIndex << ", fragment " << fragmentIndex << "\n";
            }

            // Reset fragmentIndex after processing fragment 2
            if (fragmentIndex == 2) {
                fragmentIndex = 0;  // Reset after both fragments of an event are processed
            }
        }
    }

    infile.close();
    outfile.close();
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_cgmf_file> <output_csv_file> <mass_excess_file>\n";
        return 1;
    }

    // Load the mass excess data from the file into a map
    std::ifstream massExcessFile(argv[3]);
    std::map<std::pair<int, int>, double> massExcessMap;

    if (!massExcessFile.is_open()) {
        std::cerr << "Error: Could not open mass excess file: " << argv[3] << "\n";
        return 1;
    }

    std::string massLine;
    while (std::getline(massExcessFile, massLine)) {
        std::istringstream iss(massLine);
        int Z, A;
        double massExcess;
        char comma;
        if (iss >> Z >> comma >> A >> comma >> massExcess) {
            massExcessMap[{Z, A}] = massExcess;
        }
    }

    massExcessFile.close();

    // Process the input CGMF file
    processFile(argv[1], argv[2], massExcessMap);

    std::cout << "Processing complete. Data saved to " << argv[2] << std::endl;

    return 0;
}

