#ifndef HIT_HH
#define HIT_HH

#include <vector>

struct Hit {
    int volumeID;
    double totalEdep;
    double hitTime;
};

extern std::vector<Hit> allHits; // Global storage for all hits

#endif // HIT_HH

