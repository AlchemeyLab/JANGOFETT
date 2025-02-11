//written by Jack Shire, Liam Walker, Jacob Jaffe, and Sasha Chemey
//this version integrates the combine steps to hits function, and outputs a single file that is much smaller than the original exported file - 607 kB/10k runs

#include <cstdio>
#include <iostream>
#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <vector>
#include <random>
#include <cmath>
#include <map>
#include <fstream>
#include <algorithm>
#include <chrono> 
#include <TH2D.h>
#include <TCanvas.h>
#include <TBranch.h>
#include <TROOT.h>
#include <TStyle.h>
using namespace std;
using namespace std::chrono;

// Structure to hold individual event data
struct EventData {
  int TrackID;
  int ParentID;
  int EventID;
  int VolumeCopyNumber;
  int StepNumber;
  int PDGCode;
  double X;
  double Y;
  double Z;
  double Edep;
  double Time;        // Original time from the ROOT file
  double GlobalTime;  // Shifted time (calculated)
};

// Function to generate shifted times using inverse Poisson CDF
double invpoisscdf(double random_uniform, double rate){
  return -log(1 - random_uniform) / rate;
}

// Function to generate random shifted times for each fission event
  std::vector<double> randtimes(int num_events, double fission_rate) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> random_uniform_gen(0.0, 1.0);
  std::vector<double> ShiftedTimes;
  ShiftedTimes.reserve(num_events);
  double randtime = 0.0;
  ShiftedTimes.push_back(randtime);  // Start the first event at time = 0
  for (int i = 1; i < num_events; i++) {
    randtime += invpoisscdf(random_uniform_gen(gen), fission_rate);
    ShiftedTimes.push_back(randtime);
  }
  return ShiftedTimes;
}

class Detector{
private:
  int detidbase;
  int num_detectors;
  vector<int> detids;
  double det_check_time;
  double det_check_energy;
  double det_htime;
  double E_resolution;
  double ns_resolution;
public:
  Detector(int ndetid,int nnum_detectors, double ndet_check_time,double ndet_check_energy, double ndet_htime, double percent_E_resolution, double ns_time_resolution){
    detidbase = ndetid;
    num_detectors=nnum_detectors;
    for(int i=0;i<num_detectors;i++){detids.push_back(detidbase+i);}
    det_check_time = ndet_check_time;
    det_htime = ndet_htime;
    det_check_energy =ndet_check_energy;
    E_resolution = percent_E_resolution/100;
    ns_resolution = ns_time_resolution;
  }
  int GetIdBase(){
    return detidbase;
  }
  int GetNumDet(){
    return  num_detectors;
  }
  vector<int> GetIds(){
    return detids;
  }
  double GetCheckTime(){
    return det_check_time;
  }
  double GetCheckEnergy(){
    return det_check_energy;
  }
  double GetEnergyResolution(){
    return E_resolution;
  }
  double GetTimeResolution(){
    return ns_resolution;
  }
  double GetHTime(){
    return det_htime;
  }
};

class Detectors{
private:
  vector<Detector> detlist;
  int num_detectors;
  vector<int> detidlist;
public:
  Detectors(vector<Detector> ndetlist){
    detlist=ndetlist;
    num_detectors=0;
    for (int i=0;i<detlist.size();i++){
      vector<int> idlist = detlist[i].GetIds();   // double det_sum_window=2e-6; //sum window chosen to be 2 microsec
      double en_thresh=20;
      num_detectors += idlist.size();
      detidlist.insert(detidlist.end(),idlist.begin(),idlist.end());
    }
  }
  Detector GetDetector(int id){
    for (int i=0;i<detlist.size();i++){
      vector<int> idlist = detlist[i].GetIds();
      if (find(idlist.begin(),idlist.end(),id)!=idlist.end()){
	return detlist[i];
      }
    }
    //   cout<<id<<" not a detector ID."<<endl;
    return detlist[0];
  }
  int GetNumDet(){
    return num_detectors;
  }
  vector<int> GetDetIdList(){
    return detidlist;
  }
};

// Function to apply Gaussian blur to energy
double applyGaussianBlurE(double energy, double energyfwhm) {
  static std::default_random_engine generator;
  double sigma = energy*energyfwhm/2.35482; // Convert FWHM to sigma
  std::normal_distribution<double> distribution(energy, sigma);
  return distribution(generator);
}
// Function to apply Gaussian blur to T
double applyGaussianBlurT(double time, double fwhmns) {
  static std::default_random_engine generator;
  //double distr = time+energyfwhm/2.35482; // Convert FWHM to sigma
  std::normal_distribution<double> distribution(0, fwhmns);
  //   double distr = time + distribution;
  return distribution(generator)+time;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <root_file>" << std::endl;
    return 1;
  }
  
    auto ti = high_resolution_clock::now();

  // Open the original ROOT file
  TFile *file = TFile::Open(argv[1]);
  if (!file || file->IsZombie()) {
    std::cerr << "Error opening ROOT file: " << argv[1] << std::endl;
    return 1;
  }

  // Access the TTree (with correct name "StepData")
  TTree *tree = (TTree*)file->Get("StepData");
  if (!tree) {
    std::cerr << "Error: TTree 'StepData' not found in ROOT file." << std::endl;
    file->Close();
    return 1;
  }

  // Variables to hold event data
  int TrackID, ParentID, EventID, VolumeCopyNumber, StepNumber, PDGCode;
  double X, Y, Z, Edep, Time, GlobalTime;

  // Set branch addresses to read the data
  tree->SetBranchAddress("TrackID", &TrackID);
  tree->SetBranchAddress("ParentID", &ParentID);
  tree->SetBranchAddress("EventID", &EventID);
  tree->SetBranchAddress("VolumeCopyNumber", &VolumeCopyNumber);
  tree->SetBranchAddress("StepNumber", &StepNumber);
  tree->SetBranchAddress("PDGCode", &PDGCode);
  tree->SetBranchAddress("X", &X);
  tree->SetBranchAddress("Y", &Y);
  tree->SetBranchAddress("Z", &Z);
  tree->SetBranchAddress("Edep", &Edep);
  tree->SetBranchAddress("Time", &Time);  // Read the original time
  

  // Step 1: Determine the maximum EventID in the ROOT file
  int maxEventID = -1;
  Long64_t nentries = tree->GetEntries();
    
  for (Long64_t i = 0; i < nentries; i++) {
    tree->GetEntry(i);
    if (EventID > maxEventID) {
      maxEventID = EventID;
    }
  }

  // Step 2: Calculate the number of fission events
  int numFissionEvents = (maxEventID + 1) / 2;

  // Generate shifted times for each fission event, updated and recompiled by run_jangofett.sh
  std::vector<double> shiftedTimes = randtimes(numFissionEvents, 10000.0);  

  // Calculate time differences between consecutive fissions and create a histogram
  std::vector<double> timeDifferences;
  for (size_t i = 1; i < shiftedTimes.size(); i++) {
    double diff = (shiftedTimes[i] - shiftedTimes[i - 1]) * 1e9; // Convert to nanoseconds
    timeDifferences.push_back(diff);
  }

  // Create a histogram of the time differences in nanoseconds
  TH1D *timeDiffHist = new TH1D("TimeBetweenFissions", "Time Between Fissions (ns)", 100, 0, *std::max_element(timeDifferences.begin(), timeDifferences.end()));
  for (const auto& diff : timeDifferences) {
    timeDiffHist->Fill(diff);
  }

  cout<<"opening outputhits.root"<<endl;
  string outfilename = "OutputHits.root";
  // Create a new ROOT file for the output
  TFile *outFile = TFile::Open(outfilename.c_str(), "RECREATE");  
  TTree *newTree = new TTree("StepData", "Particle Step Data with Adjusted Global Time");
  newTree -> SetAutoSave(100000);
 

  cout<<"opened newTree"<<endl;
  // Set branch addresses for the new tree, without saving the old Time
  newTree->Branch("TrackID", &TrackID, "TrackID/I");
  newTree->Branch("ParentID", &ParentID, "ParentID/I");
  newTree->Branch("EventID", &EventID, "EventID/I");
  newTree->Branch("VolumeCopyNumber", &VolumeCopyNumber, "VolumeCopyNumber/I");
  newTree->Branch("StepNumber", &StepNumber, "StepNumber/I");
  newTree->Branch("PDGCode", &PDGCode, "PDGCode/I");
  newTree->Branch("X", &X, "X/D");
  newTree->Branch("Y", &Y, "Y/D");
  newTree->Branch("Z", &Z, "Z/D");
  newTree->Branch("Edep", &Edep, "Edep/D");
  newTree->Branch("GlobalTime", &GlobalTime, "GlobalTime/D");  // This will store the shifted time
  cout<<"made new branches"<<endl;
  // Step 3: Apply shifts to the original time for each entry in each fission event
  for (Long64_t i = 0; i < nentries; i++) {
    tree->GetEntry(i);

    // Determine which fission event this entry belongs to
    int fissionEventID = EventID / 2;  // Each pair (even, odd) EventID belongs to one fission event

    // Apply the time shift for this fission event to all entries within it
    double shift = shiftedTimes[fissionEventID] * 1e9; // Convert shift to nanoseconds
    GlobalTime = Time + shift;
    //  cout<<"i "<<i<<"\t";
    // Fill the new tree with the shifted GlobalTime
    newTree->Fill();
    // cout<<"filled newtree for "<<i<<endl;
  }
  
  std::cout<<"writing timediff hist"<<std::endl;
  // Write the tree and histogram to the file once and close the output file
  //newTree->AutoSave();
  timeDiffHist->Write();  // Write the histogram to the output file
  //outFile->Close();
  // outFile->Delete();
  
  // Close the original ROOT file
  // file->Close();

  //   std::cout << "File has been successfully processed and saved as OutputTimeline.root.\n";
  std::cout<<" ending the time adjustment, beginning the CS2H\n";


  //void CS2H_Update2_2(
  //  const char *input="OutputTimeline.root",
  //const char *output="hits.root";
  double locationresolution=0.78125/*mm*/;
  int fissions = 3000000;//numFissionEvents;/*total fissions*///originally hard-coded
  int fission_rate = 1000;/*per second*///)
    

  double experimental_time = (fissions/fission_rate)*1e+9; //how long do we run our experiment for in seconds
  double exp_t_win = 0;
  if(fissions < 100){
    exp_t_win = experimental_time*10;
  }else if(fissions < 1000){
    exp_t_win = experimental_time*4;
  }else{
    exp_t_win = experimental_time;
  }
  exp_t_win += 10e+9;//time plus ten seconds for later decay/isomers/transitions

  // Section initializing the detector information
  double det_sum_window=20.; //sum window chosen to be EzrrL: 20 nanosec
  double det_FWHM=8000./3.; 
  double en_thresh=20.;

  Detector PL_HPGe = Detector(1,21,det_sum_window,en_thresh,det_FWHM,.2,2);
  Detector CL_HPGe = Detector(1001,21,det_sum_window,en_thresh,det_FWHM,.2,5);
  // Detector CL_HPGe_sum = Detector(1001,21,det_sum_window,en_thresh,det_FWHM,.2);
  Detector GMX_HPGe = Detector(2001,21,det_sum_window,en_thresh,det_FWHM,.2,5);

  Detector PL_BGO = Detector(601,21,det_sum_window,en_thresh,det_FWHM,24.,1.3); 
  Detector CL_BGO = Detector(1601,21,det_sum_window,en_thresh,det_FWHM,24.,1.3); 
  Detector GMX_BGO = Detector(2601,21,det_sum_window,en_thresh,det_FWHM,24.,1.3); 

  Detector DSSD = Detector(801,2,det_sum_window,1e3,det_FWHM,.4,2); //overly conservative timing resolution in ns

  vector<Detector> detectors={DSSD,PL_HPGe,CL_HPGe,GMX_HPGe,PL_BGO,CL_BGO,GMX_BGO};
  Detectors dets = Detectors(detectors);
  int num_det = dets.GetNumDet();
  vector<int> detectoridlist = dets.GetDetIdList();

  vector<int> idlist = {
    801, 802, 
    1, 2, 3, 4, 6, 7, 8, 11, 12, 13, 15, 16, 17, 18, 19, 20, 21, 
    1005, 1009, 1010, 1014, 
    601, 602, 603, 604, 606, 607, 608, 611, 612, 613, 615, 616, 617, 618, 619, 620, 621, 
    1605, 1609, 1610, 1614
  };
  bool useCustomIDlist = true;
  if (useCustomIDlist){detectoridlist.assign(idlist.begin(),idlist.end());num_det=idlist.size();}
  //vector<int> runing_detectors;

  // Loading the input file as a TTree only using the Edep, VolumeCopyNumber and GlobalTime branches
  //TFile* filein = new TFile(input);
  //outFile->cd();
  TTree* StepData = (TTree*)outFile->Get("StepData");
  double de, time;
  int vlm;
  TBranch *be, *bv, *bt;
  StepData->SetBranchAddress("Edep",&de, &be); // energy deposition
  StepData->SetBranchAddress("VolumeCopyNumber",&vlm, &bv);
  StepData->SetBranchAddress("GlobalTime",&time, &bt); // time
  Long64_t nevt = StepData->GetEntries(); // total number of steps simulated

  Long64_t *index = new Long64_t[nevt];
  StepData->SetEstimate(nevt); 
  StepData->Draw("GlobalTime","","goff");
  TMath::Sort(nevt,StepData->GetV1(),index,false);
  //I->Print();
  StepData->LoadBaskets(1000000000);
  //  cout<<endl;

  // output, which contains a tree filled with combined hits
  //TFile *fileout = new TFile(output, "recreate");
  outFile->cd();
  TTree *hittree = new TTree("h","combined hits");
  // parameters of combined hits
  double henergy, htime;
  // double henergy_detid[10000] = {};
  // double htime_detid[10000] = {};
  int hvlm;
  hittree->Branch("energy",&henergy,"energy/D");
  hittree->Branch("time",&htime,"time/D");
  hittree->Branch("vlm",&hvlm,"vlm/I");

  // main loop to combine step points
  //  num_det = 1;//just to set the loop through once because
  
  double check_sum_window_detid[20000] = {};
  double hit_sum_window_detid[20000] = {};
  double det_energy_thresh_detid[20000] = {};
  double percent_energy_resln_detid[20000] = {};
  double ns_time_resln_detid[20000] = {};
  for (int i=0;i<num_det;i++){// Detector Loop
    // auto tds = high_resolution_clock::now();
    int id = detectoridlist[i];

    Detector CurrentDetector = dets.GetDetector(id);
    check_sum_window_detid[id] = CurrentDetector.GetCheckTime();
    hit_sum_window_detid[id] = CurrentDetector.GetHTime();
    det_energy_thresh_detid[id] = CurrentDetector.GetCheckEnergy();
    percent_energy_resln_detid[id] = CurrentDetector.GetEnergyResolution();
    ns_time_resln_detid[id] = CurrentDetector.GetTimeResolution();
  }
    
  // cout<<"Detector: "<<id<<" (Running)."<<endl;
  int num_hits = 0;
  double hit_delay = 0;
  int pti = 0;

  // nevt = 2000;
  //cout<<"Total DSSD Hit Energy: "<<totalDSSDhiten<<" keV"<<endl;
  cout<<"15 GB nevt: "<<nevt<</*", min E dep of "<<det_energy_thresh<<*/" with an experimental time of "<<experimental_time<<" ns"<<endl;

  int vectorsizing = 20000;
  double henergy_detid[20000] = {};
  double htime_detid[20000] = {};
  double check_en_sum_detid[20000] = {};
  vector<double> finaltime_detid = {};
  double check_en_sum[20000] = {};
  double startwin_detid[20000] = {};
  double triggertime_detid[20000] = {};
  vector<double> lasttime_detid = {};
  vector<bool> trig_detid {};
  double numevt_detid[20000] = {};
  for(int i = 0; i < vectorsizing; i++){
    trig_detid.push_back(false);
    finaltime_detid.push_back(0);
    lasttime_detid.push_back(0.001);
  }
  
  int jdbl = 0;
  for(int j=0;j<nevt; j++){
    //   bool testbreak = true;
    jdbl = j;
    
    StepData->GetEntry(index[j]);
    // if(detectoridlist[vlm] = 0){
    // continue;
    //}
    if(vlm > 10000){
      vlm = vlm/10;
    }

    if(j%1000000 == 0){
      cout<<jdbl<<" complete of "<<nevt<<"\t"<<time<<" of "<<exp_t_win<<" and it has taken ";
      auto tn = high_resolution_clock::now();
      int curr_durationmin = duration_cast<minutes>(tn - ti).count();
      int curr_durationsec = duration_cast<seconds>(tn - ti).count()%60;
      cout<<curr_durationmin<<" min "<<curr_durationsec<<" sec"<<endl;
    }
 
    if(time > exp_t_win){//prevent the eternity issue
      //      cout<<jdbl<<" complete of "<<nevt<<"\t"<<time<<" of "<<exp_t_win<<endl;
      goto endofthisloop;
    }

    if (time<hit_delay){continue;}//negative time veto

    if(numevt_detid[vlm]>0){//zeroth time through blocker
      if(time > finaltime_detid[vlm]){//put this before the check trigger time, so it will continue easily
	//save trigger time, energy deposited to vlm; turn off flag; zero numbers, continue
	lasttime_detid[vlm] = finaltime_detid[vlm];
	if(((vlm == 801)||(vlm == 802))&&(henergy_detid[vlm]<25000)){
	  //	  cout<<"\n"<<vlm<<" lowEveto "<<henergy_detid[vlm]<<endl;
	  trig_detid[vlm] = false;
	  check_en_sum_detid[vlm] = 0;
	  henergy_detid[vlm] = 0;
	  triggertime_detid[vlm] = 0;
	  startwin_detid[vlm] = 0;
	  finaltime_detid[vlm] = 0;
	}
	if(trig_detid[vlm]){
	  henergy = henergy_detid[vlm];
	  double henoblur = henergy;
	  double detEresln = percent_energy_resln_detid[vlm];
	  henergy = applyGaussianBlurE(henergy,detEresln);
	  htime = triggertime_detid[vlm];
	  double htnoblur = htime;
	  double detTresln = ns_time_resln_detid[vlm];
	  htime = applyGaussianBlurT(htime,detTresln);
	  //cout<<"vlm "<<vlm<<" time no blur "<<htnoblur<<" blurred "<<htime<<" time diff "<<fabs(htnoblur-htime)/detTresln<<" of fwhm"<<endl;
	  //    cout<<fabs(htnoblur-htime)/detTresln<<"\n";
	  hvlm = vlm;
	  hittree->Fill();
	  num_hits++;
	}
	//zeroes
	//zeroes:	
	henergy = 0;
	htime = 0;
	hvlm = 0;
	trig_detid[vlm] = false;
	check_en_sum_detid[vlm] = 0;
	henergy_detid[vlm] = 0;
	triggertime_detid[vlm] = 0;
	startwin_detid[vlm] = 0;
	finaltime_detid[vlm] = 0;
      }
    }
        
    if(time > finaltime_detid[vlm]){
      startwin_detid[vlm] = time;
      finaltime_detid[vlm] = startwin_detid[vlm]+check_sum_window_detid[vlm];
      check_en_sum_detid[vlm] = 0;
      henergy_detid[vlm] = 0;
    }
    check_en_sum_detid[vlm] += de;
    henergy_detid[vlm] += de;

    if((check_en_sum_detid[vlm] > det_energy_thresh_detid[vlm])&&!(trig_detid[vlm])){
      triggertime_detid[vlm] = time;
      trig_detid[vlm] = true;
      //  cout<<"---...--- 1\n";
    }
    numevt_detid[vlm]++;
    if(j == nevt-2){
      break;
    }
  }
 endofthisloop:
  cout<<"\n\n"<<num_hits<<" hits were created"<<endl;
  auto tde = high_resolution_clock::now();

  // save the output tree
  //StepData();
  //  hittree->Write();
 
  TFile *saver = new TFile("OutputTimeline.root","recreate");
  saver->cd();
  TTree *treehits = hittree->CloneTree();
  cout<<"treehits\t";
  treehits->Write();
  cout<<"written\t";
  saver->Close();
  cout<<"closed"<<endl;
  auto tf = high_resolution_clock::now();
  int total_durationmin = duration_cast<minutes>(tf - ti).count();
  int total_durationsec = duration_cast<seconds>(tf - ti).count()%60;
  cout <<"The total time to run FETTAnalysis is: "<<total_durationmin<<" min "<<total_durationsec<<" sec."<<endl;

  outFile->Close();

  remove(outfilename.c_str());
  //  if(remove(outfilename) == 0){
    cout<<"file deleted"<<endl;
    //  }
  
   
return 0;
}

