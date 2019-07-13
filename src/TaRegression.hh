//***********************************************************
// TaRegression.hh
//    - Regression Class for ReD Analysis
//

#include "TFile.h"
#include "TTree.h"
#include <string>
#include <vector>
#include "VaAna.hh"
#include <iostream>
//#include "TQObject.h"
//#include "RQ_OBJECT.h"
//#include "TaConfFile.hh"

using namespace std;

class TFile;
class TTree;
class TaInput;
class TaStatistics;
class TaRegression: public VaAna {

  //  RQ_OBJECT()

  ClassDef(TaRegression,0)

public:

  // Constructors/destructors/operators
  TaRegression ();

  // No need to copy or assign a TaRegression. So the copy 
  // constructor and assignment operator are private and null.

  virtual ~TaRegression ();

  // Major functions

  Int_t Init (TaInput&);           // Initialization with config file
  void LinearInit ();              // Initialization for Linear Algorithm
  void MatrixInit ();              // Initialization for Matrix Algorithm
  void Process ();                 // Gateway Regression algorithms
  void ProcessLinear ();           // Linear Regression algorithm
  void ProcessMatrix ();           // Matrix Regression algorithm
  void FillTree();
  void FillStats(); // to fRedFile
  void WriteSummary(); // Printing Summary Outputs

private:

  // Private member functions
  TaRegression (const TaRegression&) {}
  TaRegression& operator= (const TaRegression&) { return *this; }

  // Data members
  UInt_t TotEvents;                 // Total number of events in tree
  Int_t LinearCycles;              // Number of linear cycles to do

  // Vectors to store events and statistics
  TaStatistics* IVStats;                     // IV Statistics
  vector < vector<Double_t> > IVEntries;     // IV Entries in pair tree

  TaStatistics* DVStats;                     // DV Statistics
  vector < vector<Double_t> > DVEntries;     // DV Entries  in pair tree
  vector < vector<Double_t> > DVEntriesOrig; // DV Entries in pair tree

  vector < vector<Double_t> > PanCutEntries; // Pan Cut Entries in pair tree
  // vector <Int_t> LEvEntries;              // evt_ev_num (low) for entries
  // vector <Int_t> HEvEntries;              // evt_ev_num (high) for entries
  vector< Double_t> pattern_entries;
  vector <Bool_t> GoodEvents;                // Events that Pass all Cuts
  UInt_t nGoodEvents;                        // number of Good Events

  // Vectors to store the Regression Coefficients (matrix method)
  //  for each DV, for each minirun.
  vector < vector < vector <Double_t> > > mCoeff;
  vector < vector < vector <Double_t> > > mErrCoeff;

  vector<vector<Double_t> > mIV_mean;
  vector<vector<Double_t> > mIV_rms;
  vector<vector<Double_t> > mIV_err;

  vector<vector<Double_t> > mDV_raw_mean;
  vector<vector<Double_t> > mDV_raw_rms;
  vector<vector<Double_t> > mDV_raw_err;

  vector<vector<Double_t> > mDV_reg_mean;
  vector<vector<Double_t> > mDV_reg_rms;
  vector<vector<Double_t> > mDV_reg_err;

  
  // Vectors for IV and DV names
  vector <TString> DVnames;                 // DV Names.
  vector <TString> IVnames;                 // IV Names.
  vector <TString> PanCutnames;                // Pan Cut Names.

  // Vectors for cuts
  vector <TString> PanCuts;                 // Pan Cuts defined from leafs

  // Vector for miniruns intervals
  vector<pair <UInt_t,UInt_t> > minirange;    // First is pair tree entry# low, 
                                              // Second is pair tree entry# high.
  vector<pair <Int_t,Int_t> > minirangeEv;    // First is event low, 
                                              // Second is event high.
  // Vector indicator for minirun
  vector <UInt_t> minirun;

};  
