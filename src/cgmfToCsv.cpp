#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <iomanip>
#include <cmath>

// Trim leading and trailing spaces from a string
std::string trim(const std::string &s) {
    auto start = s.find_first_not_of(" \t");
    auto end   = s.find_last_not_of(" \t");
    return (start == std::string::npos)
         ? ""
         : s.substr(start, end - start + 1);
}

// Determine if a line starts with two integers (fragment information)
bool startsWithTwoInts(const std::string &line) {
    std::istringstream iss(trim(line));
    int a, b;
    return (iss >> a && iss >> b);
}

// Parse fragment data (A, Z, neutrons, U, KEI, KEF, etc.)
bool parseFragmentLine(const std::string& line,
                       int& A, int& Z, int& neutrons,
                       double& U, double& KEI, double& KEF)
{
    std::istringstream iss(trim(line));
    double J;
    int P, gammas, prompt_neutrons;
    if (iss >> A >> Z >> U >> J >> P >> KEI >> KEF
            >> neutrons >> gammas >> prompt_neutrons) {
        return true;
    }
    return false;
}

// global toggle: include neutrons?
static bool gIncludeNeutrons = true;

// Process one fragment (and then inject its neutrons)
bool processFragment(std::ifstream &infile,
                     std::ofstream &outfile,
                     int fissionEvent,
                     int fragmentNumber,
                     const std::string& fragmentLine,
                     const std::map<std::pair<int,int>, double>& massExcessMap)
{
    // 1) Parse fragment info
    int A, Z, neutrons;
    double U, KEI, KEF;
    if (!parseFragmentLine(fragmentLine, A, Z, neutrons, U, KEI, KEF)) {
        std::cerr << "Error parsing fragment line for event "
                  << fissionEvent << ", fragment " << fragmentNumber << "\n";
        return false;
    }

    // 2) Read fragment momentum line
    std::string line;
    if (!std::getline(infile, line)) {
        std::cerr << "Error reading momentum line for event "
                  << fissionEvent << ", fragment " << fragmentNumber << "\n";
        return false;
    }
    std::istringstream mom(trim(line));
    double px_i, py_i, pz_i;
    double px_f, py_f, pz_f;
    if (!(mom >> px_i >> py_i >> pz_i >> px_f >> py_f >> pz_f)) {
        std::cerr << "Error parsing final momentum vectors for event "
                  << fissionEvent << ", fragment " << fragmentNumber << "\n";
        return false;
    }

    // 3) Write fragment row
    if (neutrons == 0) {
        double excitationEnergy = std::round(U * 100.0) / 100.0;
        outfile << fissionEvent << ","
                << fragmentNumber << ","
                << (A - neutrons) << ","
                << Z            << ","
                << neutrons     << ","
                << px_f  << ","
                << py_f  << ","
                << pz_f  << ","
                << KEF    << "," 
                << std::fixed << std::setprecision(2)
                << excitationEnergy
                << "\n";
        return true;
    }


    // 4) Skip the first neutron‐info line
    if (!std::getline(infile, line)) {
        std::cerr << "Error skipping first neutron line for event "
                  << fissionEvent << ", fragment " << fragmentNumber << "\n";
        return false;
    }
    // 5) Read the second neutron‐info line (px,py,pz, KE) × neutrons
    if (!std::getline(infile, line)) {
        std::cerr << "Error reading neutron line for event "
                  << fissionEvent << ", fragment " << fragmentNumber << "\n";
        return false;
    }

    // 6) Parse all neutron values into a vector
    std::vector<double> vals;
    {
        std::istringstream iss(trim(line));
        double v;
        while (iss >> v) vals.push_back(v);
    }
    if ((int)vals.size() != neutrons * 4) {
        std::cerr << "Warning: expected " << neutrons*4
                  << " values for neutrons, got " << vals.size()
                  << " (event " << fissionEvent << ")\n";
    }

    // 7) Sum neutron KE for excitation‐energy calc
    double neutronKESum = 0.0;
    for (int i = 0; i < neutrons; ++i) {
        neutronKESum += vals[i*4 + 3];
    }

    // 8) Look up mass excesses
    double fragMassEx = 0.0, prodMassEx = 0.0;
    auto keyFrag = std::make_pair(Z, A);
    auto keyProd = std::make_pair(Z, A - neutrons);
    if (massExcessMap.count(keyFrag)) fragMassEx = massExcessMap.at(keyFrag);
    if (massExcessMap.count(keyProd)) prodMassEx = massExcessMap.at(keyProd);

    // 9) Compute excitation energy
    double excitationEnergy =
        KEI + U
      + (A * 931.5 + fragMassEx)
      - (neutrons * 939.6)
      - ((A - neutrons) * 931.5 + prodMassEx)
      - KEF
      - neutronKESum;
    if (excitationEnergy < 0) excitationEnergy = 0.0;
    excitationEnergy = std::round(excitationEnergy * 100.0) / 100.0;

    // 10) Write fragment row 
    outfile << fissionEvent << ","
            << fragmentNumber << ","
            << (A - neutrons) << ","
            << Z            << ","
            << neutrons     << ","  
            << px_f  << ","
            << py_f  << ","
            << pz_f  << ","
            << KEF    << ","
            << std::fixed << std::setprecision(2)
            << excitationEnergy
            << "\n";

    // 11) Now write one row per neutron, only if enabled
    if (gIncludeNeutrons) {
      for (int i = 0; i < neutrons; ++i) {
        double n_px = vals[i*4 + 0];
        double n_py = vals[i*4 + 1];
        double n_pz = vals[i*4 + 2];
        double n_ke = vals[i*4 + 3];

        outfile << fissionEvent << ","
        << "Neutron" << ","
        << 1         << ","   // PrimaryA
        << 0         << ","   // Z
        << 0         << ","   // Neutrons = 0 for a neutron row
        << n_px      << ","
        << n_py      << ","
        << n_pz      << ","
        << n_ke      << ","
        << "0.00"
        << "\n";
      }
    }

    return true;
}

// Process the entire file
void processFile(const std::string &inFile,
                 const std::string &outFile,
                 const std::map<std::pair<int,int>, double> &massExcessMap)
{
    std::ifstream infile(inFile);
    std::ofstream outfile(outFile);
    outfile << std::setprecision(std::numeric_limits<double>::digits10 + 1);
    if (!infile || !outfile) {
        std::cerr << "Error opening files.\n";
        return;
    }

    // Header
    outfile << "FissionEvent,FragmentNumber,PrimaryA,Z,Neutrons,Px,Py,Pz,KEF,ExcitationEnergy\n";

    std::string line;
    int eventIdx = 0, fragIdx = 0;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0]=='#') continue;
        if (startsWithTwoInts(line)) {
            ++fragIdx;
            if (fragIdx == 1) ++eventIdx;
            if (!processFragment(infile, outfile,
                                 eventIdx, fragIdx,
                                 line, massExcessMap))
            {
                std::cerr << "Failed on event " << eventIdx
                          << ", fragment " << fragIdx << "\n";
            }
            if (fragIdx == 2) fragIdx = 0;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <input.cgmf> <output.csv> <mass_excess.csv>"
                  << " [--no-neutrons]\n";
        return 1;
    }

    // Optional flag
    if (argc == 5 && std::string(argv[4]) == "--no-neutrons") {
        gIncludeNeutrons = false;
    }

    // Load mass‐excess map
    std::ifstream mex(argv[3]);
    std::map<std::pair<int,int>, double> massEx;
    std::string mline;
    while (std::getline(mex, mline)) {
        std::istringstream iss(mline);
        int Z,A; double me; char comma;
        if (iss >> Z >> comma >> A >> comma >> me) {
            massEx[{Z,A}] = me;
        }
    }

    // Run the parsing
    processFile(argv[1], argv[2], massEx);
    std::cout << "Done. Wrote " << argv[2] << "\n";
    return 0;
}

