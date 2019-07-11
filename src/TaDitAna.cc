//**********************************************************************
//
//     HALL A C++/ROOT Parity Analyzer PostPan
//
//           TaDitAna.cc
//    - Dithering Class for ReD Analysis
//
// Author:  K.D. Paschke
//
// Modified by : Tao Ye  -June, 2019
//
////////////////////////////////////////////////////////////////////////

#define NOISY
#define DEBUG

#include "TaDitAna.hh"
#include "TaDitCyc.hh"
#include "TaConfFile.hh"
#include "TaInput.hh"
#include "TLeaf.h"
#include "TTree.h"
#include "TEventList.h"
#include "TaOResultsFile.hh"
#include "TH1D.h"

#include <fstream>


ClassImp(TaDitAna);


TaDitAna::TaDitAna():
  fNumCyc(0),curCyc(0)
{
  // Default Constructor: puposefully kept as barren as possible
  //allocate no memory in the default constructor.
}

TaDitAna::~TaDitAna() {
}


Int_t TaDitAna::Init (TaInput& input)
{
  ///////////////////////////////////////////////////////////////
  // Initialization routine.
  //
#ifdef DEBUG
  cout << "TaDitAna::Init()" << endl;
#endif

  //  Call the Common init.
  if (VaAna::Init(input) != 0)
    return 1;
  
  LoadConfig();
  Bool_t isDataAvailable = LoadModData();
  if(isDataAvailable)
    return 0;
  else
    return 1;  
}

Int_t TaDitAna::LoadConfig(){
  
  //  Call the Common init.

  Int_t nentries = (Int_t) fRawTree->GetEntries();
  cout << "Number of Entries = " << nentries << endl;

  //  if(ncycles>16)   fDitCyc.Expand(ncycles); // Why we need this? 
  fProtoCyc = new TaDitCyc(); // make prototype cycle 

  //  parse list of dithering coils
  DitList = fInput->GetConfig().GetDitList();
  for (Int_t id=0; id<fgNCoils; id++) DitMap[id]=-1;

  for (UInt_t id = 0; id<DitList.size(); id++) {
    // This is complicated by need to match coils in ditlist with
    // indexes from bmwobject, which requires DitMap
    TString ditname = DitList[id];
    Int_t ditindex = -1;
    // Following dither name for Spring19 run 1473-1474 modulation test 
    if (ditname.Contains("trim1",TString::kIgnoreCase)) ditindex = 0;
    else if (ditname.Contains("trim2",TString::kIgnoreCase)) ditindex = 1;
    else if (ditname.Contains("trim3",TString::kIgnoreCase)) ditindex = 2;
    else if (ditname.Contains("trim4",TString::kIgnoreCase)) ditindex = 3;
    else if (ditname.Contains("trim5",TString::kIgnoreCase)) ditindex = 4;
    else if (ditname.Contains("trim6",TString::kIgnoreCase)) ditindex = 5;
    else if (ditname.Contains("trim7",TString::kIgnoreCase)) ditindex = 6;
    else if (ditname.Contains("trim8",TString::kIgnoreCase)) ditindex = 7;
    if (ditindex>=0 && ditindex<fgNCoils) {
      if (DitMap[ditindex]==-1) {
	DitIndex[id]=ditindex;  // for extra safety, raw index for each coil in list
	DitMap[ditindex] = id;  // map from raw coil-index to list of coils
      } else {
	cout << "Coil name is not unique to ID string! " << ditname.Data() << endl;
	DitList.erase(DitList.begin() + id);
	cout << "Duplicate ID-tagged name removed from coil list " << endl;
        id -=1; // redo loop on next element of DitList array
      }
    }else{
      cout << "Coil name matches no ID string: " << ditname.Data() << endl;
      DitList.erase(DitList.begin() + id);
      cout << "Bogus coil name removed from coil list " << endl;
      id -= 1; // redo loop on next element of DitList array
    }
  }
  
  fNDit = DitList.size();
  
  // Parse the list of limits for the various dither objects
  for (Int_t id=0; id<fgNCoils; id++) DitLimit[id] = 1e6;
  vector <pair <TString, Double_t> > LL = fInput->GetConfig().GetDitLimList();
  for (UInt_t idl = 0; idl<LL.size(); idl++) {
    for (UInt_t id=0; id<DitList.size(); id++) {
      if (DitList[id].Contains(LL[idl].first,TString::kIgnoreCase)) {
	DitLimit[id]=LL[idl].second;
	cout << "Limiting input from coil " << DitList[id]
	     << Form(" to less than %.1f",DitLimit[id]) << endl;
      }
    }
  }

#ifdef NOISY

  for (Int_t id=0; id<fNDit; id++) {
    cout << "Dithering object " << DitList[id] << " used" << endl;
    cout << "       with matching index " << DitIndex[id] << endl;
    cout << "       check the map: " << DitMap[DitIndex[id]] << endl;
  }
#endif
  
  // need lists of detectors, monitors, and cuts
  DetList = fInput->GetConfig().GetDetList();
  MonList = fInput->GetConfig().GetMonList();
  if (fInput->GetConfig().UseCurMon()) {
    fUseCurMon=kTRUE;
    CurMon = fInput->GetConfig().GetCurMon();
    cout << " Control file suggests use of Curmon  " << CurMon << endl;
  }
  else
    fUseCurMon=kFALSE;
  // finish prototyping the DitCyc in LoadModData, after
  // lists have been trimmed by availability in the tree  
  return 0;
}


Bool_t TaDitAna::LoadModData() 
{
  // reads raw tree.  Creates and fills 
  // the containers for each modulation supercycle, in preparation
  // for the calculation of the dithering slopes
  // I need to think about a better way to Load data - Tao
  
  Double_t EvR_evNum;  // Hmm... JAPAN assigns Double_t to event_number
  Double_t EvR_bmwCycle,EvR_bmwObject;
  
  fRawTree->SetBranchAddress("event_number",&(EvR_evNum));
  fRawTree->SetBranchAddress("bmwcycnum",&(EvR_bmwCycle));
  fRawTree->SetBranchAddress("bmwobj",&(EvR_bmwObject));

  // **
  // Grab the Leafs from the Tree.
  // **
  vector <TLeaf*> leaf_trimcard;
  vector <TLeaf*> leaf_mon;
  vector <TLeaf*> leaf_det;
  vector <TLeaf*> leaf_cut;
  TLeaf* leaf_curmon;

  // **
  // Get the trim card leafs and check if they exist in the Tree
  // **

  for (UInt_t id = 0; id<DitList.size(); id++) {
    TBranch* trimcard_branch =(TBranch*)fRawTree->GetBranch(DitList[id]);
    TLeaf* this_leaf =(TLeaf*)trimcard_branch->GetLeaf("value");
    leaf_trimcard.push_back(this_leaf);
  }

  for(UInt_t i=0; i<DitList.size(); i++) {
    if(!leaf_trimcard[i]) {
      cerr << DitList[i] << " does not exist in Raw Tree. " 
	   << "Will not include in analysis." << endl;
      DitList.erase(DitList.begin() + i);
      leaf_trimcard.erase(leaf_trimcard.begin() + i);
      i--;
    }
  }
  
  fNDit = DitList.size();
  fProtoCyc->SetDitNum(fNDit);
  
  // **
  // Get the Det leafs and check if they exist in the Tree
  // **

  for(UInt_t i=0; i<DetList.size(); i++) {
    TBranch *branch_buff = (TBranch*)fRawTree->GetBranch(DetList[i]);
    TLeaf *leaf_buff  = (TLeaf*)branch_buff->GetLeaf("hw_sum");
    leaf_det.push_back(leaf_buff);
  }

  for(UInt_t i=0; i<DetList.size(); i++) {
    if(!leaf_det[i]) {
      cerr << DetList[i] << " does not exist in Raw Tree. " 
	   << "Will not include in analysis." << endl;
      DetList.erase(DetList.begin() + i);
      leaf_det.erase(leaf_det.begin() + i);
      i--;
    }
  }
  
  fNDet = DetList.size();
  fProtoCyc->SetDetNum(fNDet);

#ifdef NOISY
  for(Int_t i=0; i<fNDet; i++) {
    cout << "Detector " << i << ": " << DetList[i] << endl;
  }
#endif

  if (fNDet>fgMxNDet) {  
    cout << "Too Many Detectors defined:"
	 << "Recompile TaDitAna with larger maximum!" <<endl;
    return kFALSE;
  }

  // **
  // Get the Mon leafs and check if they exist in the Tree
  // **
  for(UInt_t i=0; i<MonList.size(); i++) {
    TBranch *branch_buff = (TBranch*)fRawTree->GetBranch(MonList[i]);
    TLeaf *leaf_buff  = (TLeaf*)branch_buff->GetLeaf("hw_sum");
    leaf_mon.push_back(leaf_buff);
  }
  for(UInt_t i=0; i<MonList.size(); i++) {
    if(!leaf_mon[i]) {
      cerr << MonList[i] << " does not exist in Raw Tree. " 
	   << "Will not include in analysis." << endl;
      MonList.erase(MonList.begin() + i);
      leaf_mon.erase(leaf_mon.begin() + i);
      i--;
    }
  }
  fNMon = MonList.size();
  fProtoCyc->SetMonNum(fNMon);
#ifdef NOISY
  for(UInt_t i=0; i<MonList.size(); i++) {
    cout << "Monitor " << i << ": " << MonList[i] << endl;
  }
#endif
  if (fNMon>fgMxNMon) {
    cout << "Too Many Monitors defined:" << 
      "Recompile TaDitAna with larger maximum!" <<endl;
    return kFALSE;
  }

  
  // **
  // Get the Current Monitor leaf
  // **

  if (fUseCurMon) {
    TBranch* branch_buff = (TBranch*)fRawTree->GetBranch(CurMon);
    leaf_curmon =  (TLeaf*)branch_buff->GetLeaf("hw_sum");
    if (!leaf_curmon) {
      cerr << "Current Monitor " << CurMon << " does not exist in Raw Tree. " 
	   << "Analysis aborted." << endl;
      return kFALSE;
    }
  }

  // Form global cuts for beam stability and mask BeamMod Channel Failure
  
  TCut globalcut = "((ErrorFlag)&(0x7bfe6fff))==0";
  
/************** Note for Error bit masking

-- To be masked (though it depends on JAPAN, I agree with myself )
	
   static const UInt_t kBModFFBErrorFlag = 0x1000; 
   static const UInt_t kBModErrorFlag = 0x8000;
   static const UInt_t kEventCutMode3 = 0x10000;
   static const UInt_t kGlobalCut = 0x4000000

-- If I want to mask these three bits at once
   Hex 0x1000 + 0x8000 + 0x10000 + 0x4000000  = 0x4019000
   Decimal 67211264

-- then the mask key
   ~(0x4019000) & (2**31-1) = 0x7bfe6fff

-- Note : it works. -  Tao  June 30th 2019

********************************************/

  // Formulate the device_error_cut
  
  // Get the CustomCuts if they are defined
  TCut customcut;
  TEventList *ditEList=0;
  TCut allcut = globalcut;

  cout << "Getting RawTree Entries satisfying event cut.." << endl;
  customcut = fInput->GetConfig().GetCustomCut().second;
  allcut = allcut + customcut;
  cout << "Event Cuts" << endl;
  cout << allcut.GetTitle() << endl;
    
  fRawTree->Draw(">>ditEList",allcut);
  ditEList = (TEventList*)gDirectory->Get("ditEList");
  if(ditEList){ 
    if(ditEList->GetN()!=0){
      cout << "\tObtained " << ditEList->GetN()
	   << " entries satisfying customcut." << endl;
    } else {
      ditEList = 0;
      cout << "\tNo entries obtained for customcut." << endl;
      cout << "\tAnalysis Aborted." << endl;
      return kFALSE;
    }
  }


  // Start by making the average (or total) cycle

  avgCyc = fProtoCyc->Clone();

  // **
  // loop through tree to sort data in appropriate holders
  // **
  Int_t nentries = (Int_t) fRawTree->GetEntries();
  Bool_t inCycle = kFALSE;
  Double_t DetData[fgMxNDet];
  Double_t MonData[fgMxNMon];
  Int_t icurCyc = -1;
  Int_t last_evNum =-1;
  Bool_t isClean;
  Int_t lastObject;
  Double_t lastValue;
  Int_t lastCycle=-1;

  for (Int_t iEv=0; iEv<nentries; iEv++ ) {
    fRawTree->GetEntry(iEv);
    // **
    // discontinuous event numbers invalidate the clean flag
    // **
    // isClean = (((int) EvR_bmwClean) == 1) && (EvR_evNum-1==last_evNum);
    isClean = (EvR_evNum-1==last_evNum);
    last_evNum=EvR_evNum;
    Int_t evNum = (Int_t) EvR_evNum; // bad rounding, don't care
    lastObject = (int) (EvR_bmwObject-1); // JAPAN bmwobj index starts from 1

    if (EvR_bmwCycle<=0 && lastObject<0) {  // ** not a step in a cycle
      //  this check  may not be neccessary since we have extracted tree. Tao
      if (inCycle) {
#ifdef NOISY
	cout << "TaDitAna:: ending supercycle " << lastCycle 
	     << " at event " << evNum << endl;
#endif
	curCyc->SetLastSCEvent(evNum);
	avgCyc->SetLastSCEvent(evNum);
	inCycle=kFALSE;
      }
    } else if (EvR_bmwCycle<=0 && lastObject>=0) {  // **  error condition
      cout << "*** cycle=0, but object != -1!! ***" << endl;
      inCycle=kFALSE;
    } else if (EvR_bmwCycle>0 && lastObject<0 && isClean) {
      // no longer an error condition
//       cout << "*** cycle>0, but object = -1!!   Supercycle: " << EvR_bmwCycle 
// 	   << " EvNum: " << iEv << endl;
    } else if (EvR_bmwCycle>0 && lastObject>=0) { // ** must be a step in a cycle
      if (inCycle && EvR_bmwCycle != lastCycle) { // check that cycle numbers agree
	cout << "*** error: no break between cycles ***" << endl;
	inCycle = kFALSE;
      }
      if (!inCycle) {// ** start a new cycle, if needed
#ifdef NOISY
	cout << "TaDitAna:: starting new supercycle " << EvR_bmwCycle
	     << " at event " << evNum << endl;
#endif
	if ((avgCyc->GetFirstEvent())>evNum || (avgCyc->GetFirstEvent())<1) 
	  avgCyc->SetFirstEvent(evNum);

	if (curCyc!=0) curCyc->SetLastEvent(evNum);
	curCyc = fProtoCyc->Clone();
	fDitCyc.AddLast(curCyc);
	inCycle=kTRUE;
	icurCyc = fDitCyc.GetLast();
	curCyc->SetFirstEvent(evNum);
	cout << " Current DitCyc is number " << icurCyc << " in TObjArray" << endl;
	lastCycle = (Int_t) EvR_bmwCycle;
	curCyc->SetSuperCycleNumber(lastCycle);
	avgCyc->SetSuperCycleNumber(lastCycle);
      }
    }
    
    if (inCycle && isClean && lastObject>=0) {
      // **
      // check any cuts, as required  (raw tree cuts are all true if cut failed)
      // **
      Bool_t passRawCuts = kTRUE;
      if(ditEList) {
	if(!ditEList->Contains(iEv)) passRawCuts=kFALSE; //customcut failed
      }
      // **
      // if all ok, get data for each detector and monitor into arrays
      // **
      if (passRawCuts) {
	// check that this object is used
	// first, is lastObject in range?
	if (lastObject>-1 && lastObject<fgNCoils) {
	  // then, is DitMap reasonably set?
	  Int_t icoil = DitMap[lastObject];
	  if (icoil>-1 && icoil<fNDit) {
	    for (Int_t idet=0; idet<fNDet; idet++) {
	      DetData[idet]= leaf_det[idet]->GetValue();
	      
	      if (fUseCurMon)
		DetData[idet] = DetData[idet]/(leaf_curmon->GetValue());
	    }
	    for (Int_t imon=0; imon<fNMon; imon++)
	      MonData[imon]= leaf_mon[imon]->GetValue();

	    // Finally, check the limit cut:
	    lastValue = leaf_trimcard[icoil]->GetValue();
	    if ( abs(lastValue)<DitLimit[icoil] ) {
	      // send this data (detector, monitor, coil + value) to the DitCyc
	      ((TaDitCyc*) fDitCyc.UncheckedAt(icurCyc))
		->LoadData(icoil, lastValue,&(DetData[0]),&(MonData[0]));
	      // send this data to the average cycle, also
	      avgCyc->LoadData(icoil, lastValue,
			       &(DetData[0]),&(MonData[0]));
	    } else {
#ifdef DEBUG
	      cout << "Discarded this data because of dit limit: "
		   << DitList[icoil] 
		   << Form(" at %.1f > limit of %.1f",
			   lastValue*1e5,DitLimit[lastObject])
		   << endl;
#endif	      
	    } // end of limit cut check
	  }// end of coil index check
	} // end of bmwobj check
      } // end of if pass raw cut
      else {
	// one could use this space to count the number of failures, for the ditcyc
	((TaDitCyc*) fDitCyc.UncheckedAt(icurCyc))->StepLostEventCounter();	
      }
    } // end of in cycle check
  }  // end loop over all events in tree

  //    if (curCyc!=0) curCyc->SetLastEvent(nentries-1); 
  //    avgCyc->SetLastEvent(nentries-1);
  if (curCyc!=0) curCyc->SetLastEvent((Int_t) last_evNum);
  avgCyc->SetLastEvent((Int_t) last_evNum);
  if (inCycle) {
    curCyc->SetLastSCEvent((Int_t) last_evNum);
    avgCyc->SetLastSCEvent((Int_t) last_evNum);
  }
  fNumCyc = fDitCyc.GetLast() + 1;
  cout << "fNumCyc: " << fNumCyc << endl;
#ifdef NOISY
  cout << "Finished loading data" << endl;
#endif
  // return success flag if task accomplished
  return kTRUE;
}

void TaDitAna::Process() 
{  
//   // make one grandaverage ditcycle
//   if (fDitCyc[0]!=0) {
//     avgCyc = ( (TaDitCyc*) fDitCyc[0])->Copy();
//     for (Int_t ic = 1; ic<fNumCyc; ic++) 
//       avgCyc->Merge((TaDitCyc*) fDitCyc[ic]);
//   }

  ComputeDitCoeffs();
  PrintCycResults(-1);
  PrintCycResults(0);
  OutputResults();
}

void TaDitAna::ComputeDitCoeffs() 
{  
  // Tao Y. May 23rd :
  // Some idea : 
  //   1. At the stage of sensitivity calculation, we need to cut off events when a coil is not active. This should be simple, just a cut on trim card RMS
  //   2. We can calculate sensitivies within each cycle then merge sensitivies weighted by error then compute slopes from that merged cycles. This will help to get rid of 'local-zero' shifts between cycles.
  //   3. Also, if any coil is not activated all the time, it needs to be moved out of coil array.
  //   4. We will need to compute mean of detectors in each cycle .
  //   5. If instead, we want to merge data from each cycles, then "local-zero" needs to be known	      
  //     *. Also, we may want to rename function names with proper 'slopes' 'sensitivities'
  
  avgCyc_sens = fProtoCyc->Clone();
  avgCyc_sens->ClearSensitivities();
  for (Int_t icyc = 0; icyc<fNumCyc; icyc++) {
    ((TaDitCyc*) fDitCyc[icyc])->CalcSlopes();
    avgCyc_sens->UpdateSensitivities((TaDitCyc*) fDitCyc[icyc]);
    // should check returned flag, and do something smart if failure!
  }
  
  for (Int_t ic = 0; ic<fNumCyc; ic++) {
    ((TaDitCyc*) fDitCyc[ic])->CalcSlopes();
    // should check returned flag, and do something smart if failure!
  }
  avgCyc->CalcSlopes();
  avgCyc->CalcCoeffs();
  avgCyc_sens->CalcCoeffs();
}


Bool_t TaDitAna::OutputSlopes() {
  Double_t dat, edat;
  Int_t ev_first, ev_last;
  char rtag[100];
  TString units;
  TString comment;
  Bool_t datStatus;

  std::vector<string> header={"CycleID",
			      "DV_name",
			      "IV_name",
			      "coeff",
			      "error",
			      "status",
			      "unit",
			      "#comment"};
  fTxtOut->WriteHeader(header);

  for (Int_t ic = 0; ic<fNumCyc; ic++) { // for each supercycle
    TaDitCyc *tcyc = (TaDitCyc*) fDitCyc[ic];
    // if (!tcyc->CalcsAreDone()) continue;
    // ev_first = tcyc->GetFirstEvent();
    // ev_last = tcyc->GetLastEvent();
    
    for (Int_t idit = 0; idit<fNDit; idit++) { //  for each coil
      for (Int_t idet =0; idet < fNDet; idet++) { //for each detector
	//       get each slope of detector vs coil
	
	dat = tcyc->GetSlopeDetVDit(idet,idit);
	edat = tcyc->GetErrSlopeDetVDit(idet,idit);
	datStatus = tcyc->StatusSlopeDetVDit(idet,idit);
	
	sprintf(rtag,"%d\t%s\t%s",
		ic,DetList[idet].Data(),DitList[idit].Data());

	units = "unit";
	units += "_per_";
	units += "mA";
	comment= "(arbitrary units)";
	//       write each slope of detector vs coil
	fTxtOut->WriteNextLine(rtag,dat,edat,
			       units.Data(),
			       datStatus,
			       comment.Data());
	
      } // end of Detector loop
      
      for (Int_t imon = 0; imon<fNMon; imon++) { //     for each monitor 
	//       get each slope of monitor vs coil
	dat = tcyc->GetSlopeMonVDit(imon,idit);
	edat = tcyc->GetErrSlopeMonVDit(imon,idit);
	datStatus = tcyc->StatusSlopeMonVDit(imon,idit);
	
	sprintf(rtag,"%d\t%s\t%s",
		ic, MonList[imon].Data(),DitList[idit].Data());
	units = "unit";
	units += "_per_";
	units += "mA";
	comment= "(arbitrary units)";
	//       write each slope of Monitor vs coil
	fTxtOut->WriteNextLine(rtag,dat,edat,
			       units.Data(),
			       datStatus,
			       comment.Data());
      } // end of Monitor loop
    }  // end of Coil Loop
  } // end of SuperCycle loop
  
  TaDitCyc *tcyc =avgCyc_sens;
  //   for each monitor
  for (Int_t imon = 0; imon<fNMon; imon++) {
    //for each detector
    for (Int_t idet =0; idet < fNDet; idet++) {
      //       get each coeff of detector vs monitor
      dat = tcyc->GetCoeffDetVMon(idet,imon);
      edat = tcyc->GetErrCoeffDetVMon(idet,imon);
      datStatus = tcyc->StatusCoeffDetVMon(idet,imon); 
      //       write each coeff of detector vs monitor
      sprintf(rtag,"-1\t%s\t%s",
	      DetList[idet].Data(),MonList[imon].Data());
      units = "ppm";
      units += "_per_";
      units += "um";
      comment= "(arbitrary unit)";
      //       write each slope of Monitor vs coil
      fTxtOut->WriteNextLine(rtag,dat,edat,
			     units.Data(),
			     datStatus,
			     comment.Data());
    }
  }

  //   average slopes, coeffs for all supercycles
  //   write slopes, coeffs for all supercycles

  return kTRUE;
}



Bool_t TaDitAna::FillSlopesTree() {
// New version: uses vector branches
#ifdef DEBUG
  cout << "TaDitAna::FillSlopesTree()" << endl;
#endif
  char str[100];
  TString buff;
  TString buff2;
  Double_t MCout[fNMon][fNDit];
  Double_t DCout[fNDet][fNDit];
  Double_t eMCout[fNMon][fNDit];
  Double_t eDCout[fNDet][fNDit];
  Bool_t statusDCout[fNDet][fNDit];
  Bool_t statusMCout[fNMon][fNDit];
  
  Double_t DMout[fNDet][fNMon];
  Double_t eDMout[fNDet][fNMon];
  
  Int_t SC,firstev,lastev,lastSCev,lostcnt;
  TaDitCyc *tcyc;

  // create tree for dithering slopes for each supercycle
  TTree ditsens("ditsens","dithering sensitivity tree");
  TTree ditslps("ditslps","dithering slopes tree");
  // create supercycle label branch
  ditsens.Branch("supercyc",&SC,"supercyc/I");
  // create event label branchs
  ditsens.Branch("firstev",&firstev,"firstev/I");
  ditsens.Branch("lastev",&lastev,"lastev/I");
  ditsens.Branch("lastSCev",&lastSCev,"lastSCev/I");
  ditsens.Branch("lostCount",&lostcnt,"lostCount/I");
  
  // Sensitivity tree: one number for each run
  // create branches for dB/dC [imon][icoil], dD/dC [idet][icoil], 
  buff = "MonVsCoil";
  sprintf(str,"[%d][%d]/D",fNMon,fNDit);
  buff2 = buff + str;
  ditsens.Branch(buff,&(MCout[0]),buff2);

  buff = "DetVsCoil";
  sprintf(str,"[%d][%d]/D",fNDet,fNDit);
  buff2 = buff + str;
  ditsens.Branch(buff,&(DCout[0][0]),buff2);

  buff = "ErrMonVsCoil";
  sprintf(str,"[%d][%d]/D",fNMon,fNDit);
  buff2 = buff + str;
  ditsens.Branch(buff,&(eMCout[0][0]),buff2);

  buff = "ErrDetVsCoil";
  sprintf(str,"[%d][%d]/D",fNDet,fNDit);
  buff2 = buff + str;
  ditsens.Branch(buff,&(eDCout[0][0]),buff2);
  
  buff = "statusMonVsCoil";
  sprintf(str,"[%d][%d]/O",fNMon,fNDit);
  buff2 = buff + str;
  ditsens.Branch(buff,&(statusMCout[0][0]),buff2);

  buff = "statusDetVsCoil";
  sprintf(str,"[%d][%d]/O",fNDet,fNDit);
  buff2 = buff + str;
  ditsens.Branch(buff,&(statusDCout[0][0]),buff2);

  
  // Slope tree: one number for each run
  // dD/dB [idet][imon]
  buff = "DetVsMon";
  sprintf(str,"[%d][%d]/D",fNDet,fNMon);
  buff2 = buff + str;
  ditslps.Branch(buff,&(DMout[0][0]),buff2);
  
  buff = "ErrDetVsMon";
  sprintf(str,"[%d][%d]/D",fNDet,fNMon);
  buff2 = buff + str;
  ditslps.Branch(buff,&(eDMout[0][0]),buff2);

  // loop over all supercycles
  for (Int_t ic = 0; ic<fNumCyc; ic++) { // for each supercycle
    tcyc = (TaDitCyc*) fDitCyc[ic];

    SC = ic;
    firstev = tcyc->GetFirstEvent();
    lastev = tcyc->GetLastEvent();
    lastSCev = tcyc->GetLastSCEvent(); 
    lostcnt = tcyc->GetLostEventCounter();

    //   fill transfer arrays with slopes
    for (Int_t idit = 0; idit<fNDit; idit++) { //  for each coil
      for (Int_t imon = 0; imon<fNMon; imon++) { //     for each monitor 
	MCout[imon][idit] = tcyc->GetSlopeMonVDit(imon,idit);
	eMCout[imon][idit] = tcyc->GetErrSlopeMonVDit(imon,idit);
	statusMCout[imon][idit] = tcyc->StatusSlopeMonVDit(imon,idit);
      }
      for (Int_t idet =0; idet < fNDet; idet++) { //for each detector
	DCout[idet][idit] = tcyc->GetSlopeDetVDit(idet,idit);
	eDCout[idet][idit] = tcyc->GetErrSlopeDetVDit(idet,idit);
	statusDCout[idet][idit] = tcyc->StatusSlopeDetVDit(idet,idit);
      }
    }
    //   fill leaves
    ditsens.Fill();
  }
  
  tcyc = avgCyc_sens;
  for (Int_t imon = 0; imon<fNMon; imon++) { //     for each monitor 
    for (Int_t idet =0; idet < fNDet; idet++) { //for each detector
      DMout[idet][imon] = tcyc->GetCoeffDetVMon(idet,imon);
      eDMout[idet][imon] = tcyc->GetErrCoeffDetVMon(idet,imon);
    }
  }
  ditslps.Fill();
  // write ditslps
  
  ditsens.Write();
  ditslps.Write();
  
  // print out right now for debugging purposes
  PrintCycResults(-1);
  PrintCycResults(0);
  return kTRUE;
}


void TaDitAna::OutputResults() {
  OutputSlopes();
  FillSlopesTree();
}

void TaDitAna::PrintCycResults(Int_t icyc = -1) {

  TaDitCyc* pcyc;

  if (icyc==-1) {
    pcyc = avgCyc;
    cout << " Print Grand Average Cycle results: " << endl;
  } 
  else if(icyc==0){
    pcyc = avgCyc_sens;
    cout << " Print Sensitivities Averaged Cycle from results: " << endl;
  }else if (icyc<fNumCyc) {
    pcyc = (TaDitCyc*) fDitCyc[icyc];
  } else {
    cout << "nonesuch cycle to print" << endl;
    return;
  }

  cout << endl << "  Monitors vs. Coils: [um / (counts/1000)] " << endl;
  for (Int_t imon = 0; imon<fNMon; imon++) { //     for each monitor 
    cout << "--------------------------------" << endl;
    for (Int_t idit = 0; idit<fNDit; idit++) { //  for each coil
      cout << MonList[imon] << " vs. " << DitList[idit] << " : ";
      cout << pcyc->GetSlopeMonVDit(imon,idit) << "+/-" 
	   << pcyc->GetErrSlopeMonVDit(imon,idit) << endl;
    }
  }

  cout << endl << "  Detectors vs. Coils: [ ppm / (counts/1000)]" << endl;
  for (Int_t idet = 0; idet<fNDet; idet++) { //     for each detitor 
    cout << "--------------------------------" << endl;
    for (Int_t idit = 0; idit<fNDit; idit++) { //  for each coil
      cout << DetList[idet] << " vs. " << DitList[idit] << " : ";
      cout << pcyc->GetSlopeDetVDit(idet,idit) << "+/-" 
	   << pcyc->GetErrSlopeDetVDit(idet,idit) << endl;
    }
  }

  cout << endl << "  Detectors vs. Monitors: [ ppm / um ] " << endl;
  for (Int_t idet = 0; idet<fNDet; idet++) { //     for each detitor
    cout << "--------------------------------" << endl;
    for (Int_t imon = 0; imon<fNMon; imon++) { //  for each coil
      cout << DetList[idet] << " vs. " << MonList[imon] << " : ";
      cout << pcyc->GetCoeffDetVMon(idet,imon) << "+/-" 
	   << pcyc->GetErrCoeffDetVMon(idet,imon) << endl;
    }
  }
  cout<< endl;

}



