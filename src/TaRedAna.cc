///////////////////////////////////////////////////////////////
// TaRedAna.C
//    - Class that handles all analyses
//      
//  
//#define NOISY
//#define DEBUG

#include "Rtypes.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include <iostream>

#include "TaRedAna.hh"
#include "TaRegression.hh"
#include "TaDitAna.hh"

ClassImp(TaRedAna);

// Constructors/destructors/operators                                         
TaRedAna::TaRedAna():
  fAnalysis(0),
  fConfig(0),
  fInput(0),
  kStatus(1)
{}

TaRedAna::TaRedAna(TString usrConfFileName):
  fAnalysis(0),
  fConfig(0),
  fInput(0),
  kStatus(1)
{
  fConfFileName = usrConfFileName;
}

TaRedAna::~TaRedAna() {}

Int_t TaRedAna::Init (Int_t runnumber)
{
#ifdef DEBUG
  cout << "TaRedAna::Init()" << endl;
#endif

  if(fConfFileName.IsNull()) {
    fInput = new TaInput();
  } else {
    fInput = new TaInput(fConfFileName);
  }
  if (fInput->Init(runnumber) != 0)
    return 1;

  return CommonInit();
}

Int_t TaRedAna::Init (string runfile)
{
#ifdef DEBUG
  cout << "TaRedAna::Init()" << endl;
#endif

  if(fConfFileName.IsNull()) {
    fInput = new TaInput();
  } else {
    fInput = new TaInput(fConfFileName);
  }
  if (fInput->Init(runfile) != 0) 
    return 1;

  return CommonInit();
}

Int_t TaRedAna::CommonInit ()
{
  TString theAnaType = fInput->GetConfig().GetAnaType();

  if(theAnaType == "regress") {
    fAnalysis = new TaRegression;
    kStatus=fAnalysis->Init(*fInput);

  } else if(theAnaType == "dither") {
    fAnalysis = new TaDitAna();
    kStatus=fAnalysis->Init(*fInput);

  } else {
    cout << "TaRedAna::Init() ERROR: " << theAnaType << "not recognized."
	 << endl;
    return 1;
  }
 
  return 0;
}

void TaRedAna::Process () 
{
#ifdef DEBUG
  cout << "TaRedAna::Process()" << endl;
#endif
  if(!kStatus)
    fAnalysis->Process();
}

void TaRedAna::End ()
{
#ifdef DEBUG
  cout << "TaRedAna::End()" << endl;
#endif
  if(!kStatus)
    fAnalysis->End();
  else
    cout << "TaRedAna Aborted. " << endl;
  //  delete fPanFile;
  //  delete fConfig;
  delete fAnalysis;

}

