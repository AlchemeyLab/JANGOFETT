// shiftTimes.cpp
#include <vector>
#include <iostream>
#include <fstream>
#include <TFile.h>
#include <TTree.h>

int main(int argc, char** argv) {
  if (argc != 7) {
    std::cerr << "Usage: shiftTimes <in.root> <out.root> <shifts.txt> "
              << "<tree> <timeBranch> <eventBranch>\n";
    return 1;
  }
  const char* inName   = argv[1];
  const char* outName  = argv[2];
  const char* shiftTxt = argv[3];
  const char* treeName = argv[4];
  const char* timeBr   = argv[5];
  const char* evtBr    = argv[6];

  // 1) load shifts
  std::vector<Double_t> shifts;
  {
    std::ifstream fin(shiftTxt);
    if (!fin) {
      std::cerr<<"ERROR: cannot open "<<shiftTxt<<"\n";
      return 1;
    }
    Double_t v;
    while (fin>>v) shifts.push_back(v);
  }

  // 2) open input file
  TFile *fin = TFile::Open(inName,"READ");
  if (!fin || fin->IsZombie()) {
    std::cerr<<"ERROR: cannot open "<<inName<<"\n";
    return 1;
  }
  TTree *tin = dynamic_cast<TTree*>(fin->Get(treeName));
  if (!tin) {
    std::cerr<<"ERROR: no TTree named "<<treeName<<"\n";
    fin->Close(); 
    return 1;
  }

  // 3) set up branches: **pointer** to vector
  std::vector<Double_t>* times = nullptr;
  Int_t                 evtID = 0;
  tin->SetBranchAddress(timeBr, &times);
  tin->SetBranchAddress(evtBr,  &evtID);

  // 4) create output and clone structure
  TFile *fout = TFile::Open(outName,"RECREATE");
  if (!fout || fout->IsZombie()) {
    std::cerr<<"ERROR: cannot create "<<outName<<"\n";
    fin->Close();
    return 1;
  }
  TTree *tout = tin->CloneTree(0);

  // 5) loop over entries, shift every element of the vector
  Long64_t n = tin->GetEntries();
  for (Long64_t i=0; i<n; ++i) {
    tin->GetEntry(i);
    Double_t shift = 0;
    if (evtID >= 0 && evtID < (int)shifts.size()) 
      shift = shifts[evtID];
    for (auto &x : *times) 
      x += shift;
    tout->Fill();
  }

  // 6) write & clean up
  fout->Write();
  fout->Close();
  fin->Close();
  return 0;
}

