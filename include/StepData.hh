// StepData.hh
#ifndef STEP_DATA_HH
#define STEP_DATA_HH

struct StepData {
    int TrackID;
    int ParentID;
    int StepNumber;
    int PDGCode;
    double Edep;
    double Time;
    int VolumeCopyNumber;
};

#endif // STEP_DATA_HH

