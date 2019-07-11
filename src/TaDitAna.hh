//**********************************************************************
//
//     HALL A C++/ROOT Parity Analyzer PostPan
//
//           TaDitAna.hh
//
// Author:  K.D. Paschke
//
////////////////////////////////////////////////////////////////////////
//
//    Central object in the correction of asymmetry using the
//    dithering technique.  
//
////////////////////////////////////////////////////////////////////////

#ifndef TADITANA_HH
#define TADITANA_HH

#include "TFile.h"
#include "TObjArray.h"
#include "TString.h"
#include "TTree.h"
#include "VaAna.hh"
#include <vector>

#include "TaDitCyc.hh"

using namespace std;

class TaDitAna : public VaAna{
      
  ClassDef(TaDitAna,0) // dithering analysis class

  private:

  // Data members
  static const Int_t fgNCoils=8;
  static const Int_t fgMxNDet=20;
  static const Int_t fgMxNMon=10;

 public:
  Int_t fNumCyc;       // number of supercycles in input data file.
  vector <Int_t> CycleID; // Added by Tao Y. for JAPAN : A container for 'bmwcycnum'
  TaDitCyc* fProtoCyc; // prototype supercycle object
  TObjArray fDitCyc;   // array of TaDitCyc, one for each supercycle
  TaDitCyc* avgCyc;    // grand average TaDitCyc, merged from all others
  TaDitCyc* avgCyc_sens;    // TaDitCyc accumulate sensitivities from each cycle
  // Vectors for Monitor, Detector, and coil names
  vector <TString> DetList;  // Det Names.
  vector <TString> MonList;  // Mon Names.
  vector <TString> DitList;  // Dit Names.
  TString CurMon;            // current monitor name
  Bool_t fUseCurMon;            // Bool to flag use of current monitor
  Int_t DitIndex[fgNCoils];  // Index used in raw data to represent each coil
  Int_t DitMap[fgNCoils];    // pointer to used-coil list, 
                             // indexed by raw-data representation
  Double_t DitLimit[fgNCoils]; // limits for each coil, indexed by used coil

  // Vectors for cuts
  vector <TString> RawCuts;                 // cuts defined from Raw leafs

  Int_t fNDet;       // number of detectors used
  Int_t fNDit;       // number of coils used
  Int_t fNMon;       // number of monitors used
  Int_t fNRwCuts;    // number of cuts applied to raw tree leaves
  Int_t fNPairCuts;    // number of cuts applied to pair tree leaves


  // Vectors for Pair Tree variables
  vector <TString> DetVarList;  // Det asym Names.
  vector <TString> MonVarList;  // Mon diff Names.

  vector <TString> PrPanCuts; // Pan Cuts defined from Pair leafs

  TaDitCyc * curCyc;

  TFile *fSlopeFile;
  // Private methods

  //public:

  // Constructors/destructors/operators
  TaDitAna();
  virtual ~TaDitAna();

  // Major methods
  Int_t Init (TaInput&);         // Initialization with config file
  void Process ();               //  sets this mutha in motion
  void ComputeDitCoeffs ();      // calc dit coeffs and slopes from each S-C
  void OutputResults();         // generate text and rootfile output

  Int_t LoadConfig();
  Bool_t LoadModData();
  Bool_t OutputSlopes();    // output slopes for each minirun
  Bool_t FillSlopesTree();  // put slopes for each minirun into tree
  void PrintCycResults(Int_t);   // print slopes/coeffs from indicated cycle

  // Modifier Methods

  // Data access methods

  // Constants

  // Static members


};

#endif
