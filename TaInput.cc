//****************************************************************
//
//  TaInput.C
//     -  Handles the input files for ReD Analysis
//
  //#define DEBUG
//#define NOISY
#include "TFile.h"
#include <iostream>
#include <stdlib.h>
#include "TTree.h"

#include "TaFileName.hh"
#include "PanTypes.hh"
#include "TString.h"
#include "TaConfFile.hh"
#include "TaInput.hh"
#include "TaFileName.hh"

ClassImp(TaInput); 

TaInput::TaInput():
  fPanFile(0),
  fRawTree(0),
  fPairTree(0),
  fConfig(0)
{}

TaInput::TaInput(TString usrConfFileName):
  fPanFile(0),
  fRawTree(0),
  fPairTree(0),
  fConfig(0)
{
  fConfFileName = usrConfFileName;
}

TaInput::~TaInput() {}

Int_t TaInput::Init(Int_t runnumber) {
#ifdef DEBUG
  cout << "TaInput::Init()" << endl;
#endif

  TaFileName::Setup (runnumber, "");
  // Open and Read config file.  This is before root file in case
  // options are to be read from conf file
  fConfig = new TaConfFile;
  if(fConfFileName.IsNull()) {
    fConfig->Read();
  } else {
    fConfig->Read(fConfFileName);
  }

  fRunNumber = runnumber;
  // Get the filename from the runnumber.  Very generic for now.
  // Should eventually check to see if it's there.

  // Open the file
  if (fConfig->GetRootAnaType().IsNull()) {
    TaFileName::Setup(runnumber,"");
  } else{
    cout << "setup for " << fConfig->GetRootAnaType().Data() << endl;
    TaFileName::Setup(runnumber,fConfig->GetRootAnaType().Data());
  }
  TaFileName fn = TaFileName("root");
  cout << "JAPAN rootfile: " << fn.String().c_str() << endl;
  fPanFile = new TFile(fn.String().c_str(),"READ");
  if (fPanFile->IsOpen() == kFALSE) {
    cerr << "Could not open JAPAN rootfile: " << fn.String().c_str() << endl;
    return 1;
  }

  return CommonInit();

}

Int_t TaInput::Init(string runfile_str) {
#ifdef DEBUG
  cout << "TaInput::Init()" << endl;
#endif
  
  TString runfile(runfile_str);

  Ssiz_t first_t = runfile.Last('_')+1;
  Ssiz_t last_t = runfile.Last('.');
  Int_t length_t = last_t -first_t;
  TString run_num = runfile(first_t,length_t);
  fRunNumber = atoi(run_num);

  TaFileName::Setup (fRunNumber, "");

  // Open and Read config file.  This is before root file in case
  // options are to be read from conf file
  fConfig = new TaConfFile;
  if(fConfFileName.IsNull()) {
    fConfig->Read();
  } else {
    fConfig->Read(fConfFileName);
  }

  cout << "JAPAN rootfile: " << runfile << endl;
  fPanFile = new TFile(runfile,"READ");
  if (fPanFile->IsOpen() == kFALSE) {
    cerr << "Could not open JAPAN rootfile: " << runfile << endl;
    return 1;
  }

  return CommonInit();

}

Int_t TaInput::CommonInit() {
  // Get the Raw and Pair Trees
  fRawTree = (TTree*)fPanFile->Get("evt");

  //It should be named as 'mul' or 'pr' if it is input from JAPAN
  TString treeName = fConfig->GetTreeName();
  fPairTree = (TTree*)fPanFile->Get(treeName);

  if(treeName=="mul"){
    cout << "-- mul tree is in use " << endl;
    cout << "-- Adding mulc as friend Tree " << endl;
    TTree *mulc_tree = (TTree*)fPanFile->Get("mulc");
    fPairTree->AddFriend(mulc_tree);
  }
#ifdef NOISY
  fRawTree->Print();
  fPairTree->Print();
#endif
    
  return 0;

}

