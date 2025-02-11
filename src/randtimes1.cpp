#include <iostream>  
#include <vector>
#include <random>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// Function to generate interarrival times using the inverse Poisson CDF
double invpoisscdf(double random_uniform, double rate) {
    return -log(1 - random_uniform) / rate;
}

// Generate time shifts for fission events
vector<double> randtimes(int num_events, double fission_rate) {
    random_device rd;
    mt19937 gen(rd());  // Seed with random device
    uniform_real_distribution<double> random_uniform_gen(0.0, 1.0);

    vector<double> ShiftedTimes;
    ShiftedTimes.reserve(num_events);
    
    double randtime = 0.0;
    ShiftedTimes.push_back(randtime);

    for (int i = 1; i < num_events; i++) {
        randtime += invpoisscdf(random_uniform_gen(gen), fission_rate);
        ShiftedTimes.push_back(randtime);
    }
    return ShiftedTimes;
}

int main(int argc, char* argv[]) {
    // Ensure correct number of arguments
    if (argc != 3) {
        return 1;
    }

    int numEvents = atoi(argv[1]);  // Convert command-line argument to int
    double fissionRate = atof(argv[2]);  // Convert to double

    // Generate the time shifts
    vector<double> times = randtimes(numEvents, fissionRate);

    // Print each time shift to stdout
    for (size_t i = 0; i < times.size(); i++) {
        cout << setprecision(15) << times[i] << endl;
      //  printf("%f\n",times[i]);
    }


    return 0;
}

