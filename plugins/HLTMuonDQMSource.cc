// -*- C++ -*-
//
// Package:    HLTMuonDQMSource
// Class:      HLTMuonDQMSource
// 
/**\class HLTMuonDQMSource 

Description: <one line class summary>

Implementation:
<Notes on implementation>
*/
//
// Original Author:  Muriel VANDER DONCKT *:0
//         Created:  Wed Dec 12 09:55:42 CET 2007
// $Id: HLTMuonDQMSource.cc,v 1.16 2009/02/25 16:51:34 hdyoo Exp $
// Modification:  Hwidong Yoo (Purdue University)
// contact: hdyoo@cern.ch
//
//



#include "DQM/HLTEvF/interface/HLTMuonDQMSource.h"
#include "DQMServices/Core/interface/DQMStore.h"

#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "DataFormats/RecoCandidate/interface/RecoChargedCandidate.h"
#include "DataFormats/RecoCandidate/interface/RecoChargedCandidateFwd.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"
#include "DataFormats/RecoCandidate/interface/IsoDeposit.h"
#include "DataFormats/RecoCandidate/interface/IsoDepositFwd.h"
#include "DataFormats/Common/interface/AssociationMap.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/MuonSeed/interface/L2MuonTrajectorySeed.h"
#include "DataFormats/MuonSeed/interface/L2MuonTrajectorySeedCollection.h"
#include "DataFormats/TrajectorySeed/interface/TrajectorySeed.h"
#include "DataFormats/L1Trigger/interface/L1MuonParticle.h"
#include "DataFormats/L1Trigger/interface/L1MuonParticleFwd.h"
#include "DataFormats/L1GlobalMuonTrigger/interface/L1MuGMTCand.h"

#include "DataFormats/MuonSeed/interface/L3MuonTrajectorySeed.h"
#include "DataFormats/MuonSeed/interface/L3MuonTrajectorySeedCollection.h"
#include "DataFormats/HLTReco/interface/TriggerEventWithRefs.h"
#include "DataFormats/HLTReco/interface/TriggerEvent.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "FWCore/Framework/interface/TriggerNames.h"

#include "TMath.h" 

using namespace std;
using namespace edm;
using namespace reco;
using namespace l1extra;
//
// constructors and destructor
//
HLTMuonDQMSource::HLTMuonDQMSource( const edm::ParameterSet& ps ) :counterEvt_(0), nTrig_(0)
{
  parameters_ = ps;
  verbose_ = parameters_.getUntrackedParameter < bool > ("verbose", false);
  monitorName_ = parameters_.getUntrackedParameter<string>("monitorName","HLT/HLTMonMuon");
  prescaleEvt_ = parameters_.getUntrackedParameter<int>("prescaleEvt", -1);
  coneSize_ = parameters_.getUntrackedParameter<double>("coneSize", 0.24);
  l2seedscollectionTag_ = parameters_.getUntrackedParameter<InputTag>("l2MuonSeedTag",edm::InputTag("hltL2MuonSeeds"));
  l3seedscollectionTag_ = parameters_.getUntrackedParameter<InputTag>("l3MuonSeedTag",edm::InputTag("hltL3TrajectorySeed"));
  l2collectionTag_ = parameters_.getUntrackedParameter<InputTag>("l2MuonTag",edm::InputTag("hltL2MuonCandidates"));
  l3collectionTag_ = parameters_.getUntrackedParameter<InputTag>("l3MuonTag",edm::InputTag("hltL3MuonCandidates"));
  l2isolationTag_ = parameters_.getUntrackedParameter<InputTag>("l2IsolationTag",edm::InputTag("hltL2MuonIsolations"));
  l3isolationTag_ = parameters_.getUntrackedParameter<InputTag>("l3IsolationTag",edm::InputTag("hltL3MuonIsolations"));

  dbe_ = 0 ;
  dbe_ = Service < DQMStore > ().operator->();
  dbe_->setVerbose(0);
 
  outputFile_ =
    parameters_.getUntrackedParameter < std::string > ("outputFile", "");
  if (outputFile_.size() != 0) {
    LogWarning("HLTMuonDQMSource") << "Muon HLT Monitoring histograms will be saved to " 
				   << outputFile_ << std::endl;
  }
  else {
    outputFile_ = "HLTMuonDQM.root";
  }
  
  bool disable =
    parameters_.getUntrackedParameter < bool > ("disableROOToutput", false);
  if (disable) {
    outputFile_ = "";
  }
  
  if (dbe_ != NULL) {
    dbe_->setCurrentFolder("HLT/HLTMonMuon");
  }
  
  std::vector<edm::ParameterSet> filters = parameters_.getParameter<std::vector<edm::ParameterSet> >("filters");
  for(std::vector<edm::ParameterSet>::iterator filterconf = filters.begin() ; filterconf != filters.end() ; filterconf++){
    theHLTCollectionLevel.push_back(filterconf->getParameter<std::string>("HLTCollectionLevel"));
    theHLTCollectionLabels.push_back(filterconf->getParameter<std::string>("HLTCollectionLabels"));
  }
  
  nTrigs = theHLTCollectionLabels.size();
  
}


HLTMuonDQMSource::~HLTMuonDQMSource()
{
  
  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)
  
}


//--------------------------------------------------------
void HLTMuonDQMSource::beginJob(const EventSetup& context)
{
  if (dbe_) {
    dbe_->setCurrentFolder(monitorName_);
    dbe_->rmdir(monitorName_);
  }
  
  if (dbe_) {
    //dbe_->setCurrentFolder("monitorName_");
    if (monitorName_ != "" ) monitorName_ = monitorName_+"/" ;
    LogInfo("HLTMuonDQMSource") << "===>DQM event prescale = " << prescaleEvt_ << " events "<< endl;
    
    
    /// book some histograms here
    int NBINS = 50; XMIN = 0; XMAX = 50;
    
    // create and cd into new folder
    char name[512], title[512];
    double pt_max;
    for( int trig = 0; trig < nTrigs+1; trig++ ) {
      string dirname;
      if( trig < nTrigs ) dirname = theHLTCollectionLabels[trig] + "/";
      else if( trig == nTrigs ) dirname = "Combined/";
      for ( int level = 1; level < 6; ++level ) {
	if( level < 4 ) sprintf(name,"Level%i",level);
	else if (level == 4 ) sprintf(name,"Level%iSeed", level-2);
	else if (level == 5 ) sprintf(name,"Level%iSeed", level-2);
	
	if( level == 1 ) pt_max = 140;
	else pt_max = 200;
	dbe_->setCurrentFolder(monitorName_+dirname+name);
	if( level == 1 ) hl1quality[trig] = dbe_->book1D("h1L1Quality","GMT quality Flag", 8, 0., 8.);
	if( level == 2 ) {
	    hnHits[trig][level-1] = dbe_->book1D(name,title, NBINS, 0., 100.);
	    hnValidHits[trig] = dbe_->book1D("HLTMuonL2_nValidHits", "L2 Number of Valid Hits", NBINS, 0., 100.);
	    hnValidHits[trig]->setAxisTitle("Number of Valid Hits", 1);
	}
	if( level == 3 ) {
	  hnTkValidHits[trig] = dbe_->book1D("HLTMuonL3_nTkValidHits", "L3 Number of Valid Tracker Hits", NBINS, 0., 100.);
	  hnTkValidHits[trig]->setAxisTitle("Number of Valid Tracker Hits", 1);
	  hnMuValidHits[trig] = dbe_->book1D("HLTMuonL3_nMuValidHits", "L3 Number of Valid Muon Hits", NBINS, 0., 100.);
	  hnMuValidHits[trig]->setAxisTitle("Number of Valid Muon Hits", 1);
	}
	if( level < 4 ) {
	  sprintf(name,"HLTMuonL%i_NMu",level);
	  sprintf(title,"L%i number of muons",level);
	  hNMu[trig][level-1] = dbe_->book1D(name,title, 5, -0.5, 4.5);
	  hNMu[trig][level-1]->setAxisTitle("Number of muons", 1);
	  sprintf(name,"HLTMuonL%i_pt",level);
	  sprintf(title,"L%i Pt",level);
	  hpt[trig][level-1] = dbe_->book1D(name,title, NBINS, 0., pt_max);
	  hpt[trig][level-1]->setAxisTitle("Pt", 1);
	  sprintf(name,"HLTMuonL%i_eta",level);
	  sprintf(title,"L%i Muon #eta",level);
	  if( level == 1 ) NBINS = 25; // for l1 muon eta, phi
	  heta[trig][level-1] = dbe_->book1D(name,title, NBINS, -2.5, 2.5);
	  heta[trig][level-1]->setAxisTitle("#eta", 1);
	  sprintf(name,"HLTMuonL%i_phi",level);
	  sprintf(title,"L%i Muon #phi",level);
	  hphi[trig][level-1] = dbe_->book1D(name,title, NBINS, -3.15, 3.15);
	  hphi[trig][level-1]->setAxisTitle("#phi", 1);
	  if( level == 1 ) NBINS = 50; // back to normal
	  sprintf(name,"HLTMuonL%i_etaphi",level);
	  sprintf(title,"L%i Muon #eta vs #phi",level);
	  hetaphi[trig][level-1] = dbe_->book2D(name,title, NBINS, -3.15, 3.15,NBINS,-2.5, 2.5);
	  hetaphi[trig][level-1]->setAxisTitle("#phi", 1);
	  hetaphi[trig][level-1]->setAxisTitle("#eta", 2); 
	  sprintf(name,"HLTMuonL%i_ptphi",level);
	  sprintf(title,"L%i Muon pt vs #phi",level);         
	  hptphi[trig][level-1] = dbe_->book2D(name,title, NBINS, 0., pt_max,NBINS,-3.15, 3.15);
	  hptphi[trig][level-1]->setAxisTitle("pt", 1);
	  hptphi[trig][level-1]->setAxisTitle("#phi", 2);
	  sprintf(name,"HLTMuonL%i_pteta",level);
	  sprintf(title,"L%i Muon pt vs #eta",level);         
	  hpteta[trig][level-1] = dbe_->book2D(name,title, NBINS, 0., pt_max,NBINS,-2.5, 2.5);
	  hpteta[trig][level-1]->setAxisTitle("pt", 1);
	  hpteta[trig][level-1]->setAxisTitle("#eta", 2);
	  if( level > 1 ) {
	    sprintf(name,"HLTMuonL%i_nHits",level);
	    sprintf(title,"L%i Number of Hits",level);         
	    hnHits[trig][level-1] = dbe_->book1D(name,title, NBINS, 0., 100.);
	    hnHits[trig][level-1]->setAxisTitle("Number of Hits", 1);
	  }
	  sprintf(name,"HLTMuonL%i_charge",level);
	  sprintf(title,"L%i Muon Charge",level);         
	  hcharge[trig][level-1]  = dbe_->book1D(name,title, 3, -1.5, 1.5);
	  hcharge[trig][level-1]->setAxisTitle("Charge", 1);
	}
	else if( level >= 4 ) {
	  sprintf(name,"HLTMuonL%iSeed_NMu",level-2);
	  sprintf(title,"L%iSeed number of muons",level-2);
	  hNMu[trig][level-1] = dbe_->book1D(name,title, 5, -0.5, 4.5);
	  hNMu[trig][level-1]->setAxisTitle("Number of muons", 1);
	  sprintf(name,"HLTMuonL%iSeed_pt",level-2);
	  sprintf(title,"L%iSeed Pt",level-2);
	  hpt[trig][level-1] = dbe_->book1D(name,title, NBINS, 0., pt_max);
	  hpt[trig][level-1]->setAxisTitle("Pt", 1);
	  sprintf(name,"HLTMuonL%iSeed_eta",level-2);
	  sprintf(title,"L%iSeed Muon #eta",level-2);
	  heta[trig][level-1] = dbe_->book1D(name,title, NBINS, -2.5, 2.5);
	  heta[trig][level-1]->setAxisTitle("#eta", 1);
	  sprintf(name,"HLTMuonL%iSeed_phi",level-2);
	  sprintf(title,"L%iSeed Muon #phi",level-2);
	  hphi[trig][level-1] = dbe_->book1D(name,title, NBINS, -3.15, 3.15);
	  hphi[trig][level-1]->setAxisTitle("#phi", 1);
	  sprintf(name,"HLTMuonL%iSeed_etaphi",level-2);
	  sprintf(title,"L%iSeed Muon #eta vs #phi",level-2);
	  hetaphi[trig][level-1] = dbe_->book2D(name,title, NBINS, -3.15, 3.15,NBINS,-2.5, 2.5);
	  hetaphi[trig][level-1]->setAxisTitle("#phi", 1);
	  hetaphi[trig][level-1]->setAxisTitle("#eta", 2); 
	  sprintf(name,"HLTMuonL%iSeed_ptphi",level-2);
	  sprintf(title,"L%iSeed Muon pt vs #phi",level-2);         
	  hptphi[trig][level-1] = dbe_->book2D(name,title, NBINS, 0., pt_max,NBINS,-3.15, 3.15);
	  hptphi[trig][level-1]->setAxisTitle("pt", 1);
	  hptphi[trig][level-1]->setAxisTitle("#phi", 2);
	  sprintf(name,"HLTMuonL%iSeed_pteta",level-2);
	  sprintf(title,"L%iSeed Muon pt vs #eta",level-2);         
	  hpteta[trig][level-1] = dbe_->book2D(name,title, NBINS, 0., pt_max,NBINS,-2.5, 2.5);
	  hpteta[trig][level-1]->setAxisTitle("pt", 1);
	  hpteta[trig][level-1]->setAxisTitle("#eta", 2);
	  sprintf(name,"HLTMuonL%iSeed_charge",level-2);
	  sprintf(title,"L%iSeed Muon Charge",level-2);         
	  hcharge[trig][level-1]  = dbe_->book1D(name,title, 3, -1.5, 1.5);
	  hcharge[trig][level-1]->setAxisTitle("Charge", 1);
	  // pt
	  sprintf(name,"HLTMuonL%iSeedtoL%i_ptres",level-2,level-3);
	  sprintf(title,"L%iSeed1/Pt - L%iMuon1/Pt",level-2,level-3);         
	  hseedptres[trig][level-4] = dbe_->book1D(name,title, NBINS, -0.1, 0.1);
	  sprintf(title,"1/PtL%iSeed - 1/PtL%i",level-2,level-3);         
	  hseedptres[trig][level-4]->setAxisTitle(title, 1);
	  sprintf(name,"HLTMuonL%iSeedtoL%i_ptrelres",level-2,level-3);
	  sprintf(title,"(L%iSeed1/Pt - L%iMuon1/Pt)/(L%iMuon1/Pt)",level-2,level-3,level-3);         
	  hseedptrelres[trig][level-4] = dbe_->book1D(name,title, NBINS, -1.0, 1.0);
	  sprintf(title,"(1/PtL%iSeed - 1/PtL%i)/(1/PtL%i)",level-2,level-3,level-3);         
	  hseedptrelres[trig][level-4]->setAxisTitle(title, 1);
	  // eta
	  sprintf(name,"HLTMuonL%iSeedtoL%i_etares",level-2,level-3);
	  sprintf(title,"L%iSeed#eta - L%iMuon#eta",level-2,level-3);         
	  hseedetares[trig][level-4] =dbe_->book1D(name,title, NBINS, -0.1, 0.1);
	  sprintf(title,"L%iSeed #eta - L%i #eta",level-2,level-3);         
	  hseedetares[trig][level-4]->setAxisTitle(title, 1);
	  sprintf(name,"HLTMuonL%iSeedtoL%i_etarelres",level-2,level-3);
	  sprintf(title,"(L%iSeed#eta - L%iMuon#eta)/L%iMuon#eta",level-2,level-3,level-3);         
	  hseedetarelres[trig][level-4] =dbe_->book1D(name,title, NBINS, -1.0, 1.0);
	  sprintf(title,"(L%iSeed #eta - L%i #eta)/L%i #eta",level-2,level-3,level-3);         
	  hseedetarelres[trig][level-4]->setAxisTitle(title, 1);
	  // phi
	  sprintf(name,"HLTMuonL%iSeedtoL%i_phires",level-2,level-3);
	  sprintf(title,"L%iSeed#phi - L%iMuon#phi",level-2,level-3);         
	  hseedphires[trig][level-4] =dbe_->book1D(name,title, NBINS, -0.1, 0.1);
	  sprintf(title,"L%iSeed #phi - L%i #phi",level-2,level-3);         
	  hseedphires[trig][level-4]->setAxisTitle(title, 1);
	  sprintf(name,"HLTMuonL%iSeedtoL%i_phirelres",level-2,level-3);
	  sprintf(title,"(L%iSeed#phi - L%iMuon#phi)/L%iMuon#phi",level-2,level-3,level-3);         
	  hseedphirelres[trig][level-4] =dbe_->book1D(name,title, NBINS, -0.1, 0.1);
	  sprintf(title,"(L%iSeed #phi - L%i #phi)/L%i #phi",level-2,level-3,level-3);         
	  hseedphirelres[trig][level-4]->setAxisTitle(title, 1);

	  sprintf(name,"HLTMuonL%iSeed_NMuperL%i",level-2,level-3);
	  sprintf(title,"L%iSeedNMu per L%i",level-2,level-3);         
	  hseedNMuper[trig][level-4] = dbe_->book1D(name, title, 5, -0.5, 4.5);
	  hseedNMuper[trig][level-4]->setAxisTitle(title, 1);
	}
	
	if (level>1&&level<4){
	  sprintf(name,"HLTMuonL%i_dr",level);
	  sprintf(title,"L%i Muon radial impact vs BeamSpot",level);         
	  hdr[trig][level-2] = dbe_->book1D(name,title, NBINS, -0.3, 0.3);
	  hdr[trig][level-2]->setAxisTitle("R Impact (cm) vs BeamSpot", 1);
	  sprintf(name,"HLTMuonL%i_d0",level);
	  sprintf(title,"L%i Muon radial impact vs (0,0)",level);         
	  hd0[trig][level-2] = dbe_->book1D(name,title, NBINS, -0.3, 0.3);
	  hd0[trig][level-2]->setAxisTitle("R Impact (cm) vs 0,0", 1);
	  sprintf(name,"HLTMuonL%i_dz0",level);
	  sprintf(title,"L%i Muon Z impact vs (0)",level);         
	  hdz0[trig][level-2] = dbe_->book1D(name,title, NBINS, -25., 25.);
	  hdz0[trig][level-2]->setAxisTitle("Z impact (cm) vs 0", 1);
	  sprintf(name,"HLTMuonL%i_dz",level);
	  sprintf(title,"L%i Muon Z impact vs BeamSpot",level);         
	  hdz[trig][level-2] = dbe_->book1D(name,title, NBINS, -25., 25.);
	  hdz[trig][level-2]->setAxisTitle("Z impact (cm) vs BeamSpot", 1);
	  sprintf(name,"HLTMuonL%i_err0",level);
	  sprintf(title,"L%i Muon Error on Pt",level);         
	  herr0[trig][level-2] = dbe_->book1D(name,title,NBINS, 0., 0.03);
	  herr0[trig][level-2]->setAxisTitle("Error on Pt", 1);
	  sprintf(name,"HLTMuonL%i_iso",level);
	  if (level==2)sprintf(title,"L%i Muon Energy in Isolation cone",level);         
	  else if (level==3)sprintf(title,"L%i Muon SumPt in Isolation cone",level);               
	  hiso[trig][level-2]  = dbe_->book1D(name,title, NBINS, 0., 5./(level-1));
	  if ( level==2)hiso[trig][level-2]->setAxisTitle("Calo Energy in Iso Cone (GeV)", 1);
	  else if ( level==3)hiso[trig][level-2]->setAxisTitle("Sum Pt in Iso Cone (GeV)", 1);
	  sprintf(name,"HLTMuonL%i_DiMuMass",level);
	  sprintf(title,"L%i Opposite charge DiMuon invariant Mass",level);         
	  hdimumass[trig][level-2]= dbe_->book1D(name,title, NBINS, 0., 150.);
	  hdimumass[trig][level-2]->setAxisTitle("Di Muon Invariant Mass (GeV)");

	  sprintf(name,"HLTMuonL%i_drphi",level);
	  sprintf(title,"L%i #Deltar vs #phi",level);         
	  hdrphi[trig][level-2] = dbe_->bookProfile(name,title, NBINS, -3.15, 3.15,1,-999.,999.,"s");
	  hdrphi[trig][level-2]->setAxisTitle("#phi", 1);
	  sprintf(title,"L%i Muon radial impact vs BeamSpot",level);         
	  hdrphi[trig][level-2]->setAxisTitle(title, 2);

	  sprintf(name,"HLTMuonL%i_d0phi",level);
	  sprintf(title,"L%i #Delta0 vs #phi",level);         
	  hd0phi[trig][level-2] = dbe_->bookProfile(name,title, NBINS, -3.15, 3.15,1,-999.,999.,"s");
	  hd0phi[trig][level-2]->setAxisTitle("#phi", 1);
	  sprintf(title,"L%i Muon radial impact vs (0,0)",level);         
	  hd0phi[trig][level-2]->setAxisTitle(title, 2);

	  sprintf(name,"HLTMuonL%i_dz0eta",level);
	  sprintf(title,"L%i #Deltaz0 vs #eta",level);         
	  hdz0eta[trig][level-2] = dbe_->bookProfile(name,title, NBINS,-2.5, 2.5,1,-999.,999.,"s");
	  hdz0eta[trig][level-2]->setAxisTitle("#eta", 1);
	  sprintf(title,"L%i Muon Z impact vs (0)",level);         
	  hdz0eta[trig][level-2]->setAxisTitle(title, 2);

	  sprintf(name,"HLTMuonL%i_dzeta",level);
	  sprintf(title,"L%i #Deltaz vs #eta",level);         
	  hdzeta[trig][level-2] = dbe_->bookProfile(name,title, NBINS,-2.5, 2.5,1,-999.,999.,"s");
	  hdzeta[trig][level-2]->setAxisTitle("#eta", 1);
	  sprintf(title,"L%i Muon Z impact vs BeamSpot",level);         
	  hdzeta[trig][level-2]->setAxisTitle(title, 2);
	}
	if(level == 2 ) {
	  sprintf(name,"HLTMuonL%itoL%i_ptpull",level,level+1);
	  sprintf(title,"(L%iMuon1/Pt - L%iMuon1/Pt)/#sigma_{pt}^{L2}",level,level+1);         
	  hptpull[trig] = dbe_->book1D(name,title, NBINS, -10.0, 10.0);
	  sprintf(title,"(1/PtL%i - 1/PtL%i)/#sigma_{pt}^{L2}",level,level+1);         
	  hptpull[trig]->setAxisTitle(title, 1);
	  sprintf(name,"HLTMuonL%itoL%i_etapull",level,level+1);
	  sprintf(title,"(L%iMuon#eta - L%iMuon#eta)/#sigma_{#eta}^{L2}",level,level+1);         
	  hetapull[trig] =dbe_->book1D(name,title, NBINS, -10.0, 10.0);
	  sprintf(title,"(L%i #eta - L%i #eta)/#sigma_{#eta}^{L2}",level,level+1);         
	  hetapull[trig]->setAxisTitle(title, 1);
	  sprintf(name,"HLTMuonL%itoL%i_phipull",level,level+1);
	  sprintf(title,"(L%iMuon#phi - L%iMuon#phi)/#sigma_{#phi}^{L2}",level,level+1);         
	  hphipull[trig] =dbe_->book1D(name,title, NBINS, -10.0, 10.0);
	  sprintf(title,"(L%i #phi - L%i #phi)/#sigma_{#phi}^{L2}",level,level+1);         
	  hphipull[trig]->setAxisTitle(title, 1);

	  sprintf(name,"HLTMuonL%itoL%i_ptpullpt",level,level+1);
	  sprintf(title,"L%i Muon #Delta Pt/#sigma_{pt}^{L2} vs Pt ",level);         
	  hptpullpt[trig] =dbe_->bookProfile(name,title, NBINS, 0, pt_max,1,-999.,999.,"s");
	  sprintf(title,"(1/PtL%i - 1/PtL%i)/#sigma_{pt}^{L2}",level,level+1);         
	  hptpullpt[trig]->setAxisTitle(title, 2);
	  hptpullpt[trig]->setAxisTitle("Pt", 1);
	  sprintf(name,"HLTMuonL%itoL%i_etapulleta",level,level+1);
	  sprintf(title,"L%i Muon #Delta#eta/#sigma_{#eta}^{L2} vs #eta ",level);         
	  hetapulleta[trig] =dbe_->bookProfile(name,title, NBINS,-2.5, 2.5,1,-999.,999.,"s");
	  sprintf(title,"(L%i #eta - L%i #eta)/#sigma_{#eta}^{L2}",level,level+1);         
	  hetapulleta[trig]->setAxisTitle(title, 2);
	  hetapulleta[trig]->setAxisTitle("#eta", 1);
	  sprintf(name,"HLTMuonL%itoL%i_phipullphi",level,level+1);
	  sprintf(title,"L%i Muon #Delta#phi/#sigma_{#phi}^{L2} vs #phi ",level);         
	  hphipullphi[trig] =dbe_->bookProfile(name,title, NBINS, -3.15, 3.15,1,-999.,999.,"s");
	  sprintf(title,"(L%i #phi - L%i #phi)/#sigma_{#phi}^{L2}",level,level+1);         
	  hphipullphi[trig]->setAxisTitle(title, 2);
	  hphipullphi[trig]->setAxisTitle("#phi", 1);
	}
	if (level < 3 ) {
	  // res
	  sprintf(name,"HLTMuonL%itoL%i_ptres",level,level+1);
	  sprintf(title,"L%iMuon1/Pt - L%iMuon1/Pt",level,level+1);         
	  hptres[trig][level-1] = dbe_->book1D(name,title, NBINS, -0.1, 0.1);
	  sprintf(title,"1/PtL%i - 1/PtL%i",level,level+1);         
	  hptres[trig][level-1]->setAxisTitle(title, 1);
	  sprintf(name,"HLTMuonL%itoL%i_etares",level,level+1);
	  sprintf(title,"L%iMuon#eta - L%iMuon#eta",level,level+1);         
	  hetares[trig][level-1] =dbe_->book1D(name,title, NBINS, -0.1, 0.1);
	  sprintf(title,"L%i #eta - L%i #eta",level,level+1);         
	  hetares[trig][level-1]->setAxisTitle(title, 1);
	  sprintf(name,"HLTMuonL%itoL%i_phires",level,level+1);
	  sprintf(title,"L%iMuon#phi - L%iMuon#phi",level,level+1);         
	  hphires[trig][level-1] =dbe_->book1D(name,title, NBINS, -0.1, 0.1);
	  sprintf(title,"L%i #phi - L%i #phi",level,level+1);         
	  hphires[trig][level-1]->setAxisTitle(title, 1);

	  sprintf(name,"HLTMuonL%itoL%i_ptrespt",level,level+1);
	  sprintf(title,"L%i Muon #Delta Pt vs Pt ",level);         
	  hptrespt[trig][level-1] =dbe_->bookProfile(name,title, NBINS, 0, pt_max,1,-999.,999.,"s");
	  sprintf(title,"1/PtL%i - 1/PtL%i",level,level+1);         
	  hptrespt[trig][level-1]->setAxisTitle(title, 2);
	  hptrespt[trig][level-1]->setAxisTitle("Pt", 1);
	  sprintf(name,"HLTMuonL%itoL%i_phiresphi",level,level+1);
	  sprintf(title,"L%i Muon #Delta#phi vs #phi ",level);         
	  hphiresphi[trig][level-1] =dbe_->bookProfile(name,title, NBINS, -3.15, 3.15,1,-999.,999.,"s");
	  sprintf(title,"L%i #phi - L%i #phi",level,level+1);         
	  hphiresphi[trig][level-1]->setAxisTitle(title, 2);
	  hphiresphi[trig][level-1]->setAxisTitle("#phi", 1);
	  sprintf(name,"HLTMuonL%itoL%i_etareseta",level,level+1);
	  sprintf(title,"L%i Muon #Delta#eta vs #eta ",level);         
	  hetareseta[trig][level-1] =dbe_->bookProfile(name,title, NBINS,-2.5, 2.5,1,-999.,999.,"s");
	  sprintf(title,"L%i #eta - L%i #eta",level,level+1);         
	  hetareseta[trig][level-1]->setAxisTitle(title, 2);
	  hetareseta[trig][level-1]->setAxisTitle("#eta", 1);

	  // relres
	  sprintf(name,"HLTMuonL%itoL%i_ptrelres",level,level+1);
	  sprintf(title,"(L%iMuon1/Pt - L%iMuon1/Pt)/(L%iMuon1/Pt)",level,level+1,level+1);         
	  hptrelres[trig][level-1] = dbe_->book1D(name,title, NBINS, -1.0, 1.0);
	  sprintf(title,"(1/PtL%i - 1/PtL%i)/(1/PtL%i)",level,level+1,level+1);         
	  hptrelres[trig][level-1]->setAxisTitle(title, 1);
	  sprintf(name,"HLTMuonL%itoL%i_etarelres",level,level+1);
	  sprintf(title,"(L%iMuon#eta - L%iMuon#eta)/L%iMuon#eta",level,level+1,level+1);         
	  hetarelres[trig][level-1] =dbe_->book1D(name,title, NBINS, -1.0, 1.0);
	  sprintf(title,"(L%i #eta - L%i #eta)/L%i #eta",level,level+1,level+1);         
	  hetarelres[trig][level-1]->setAxisTitle(title, 1);
	  sprintf(name,"HLTMuonL%itoL%i_phirelres",level,level+1);
	  sprintf(title,"(L%iMuon#phi - L%iMuon#phi)/L%iMuon#phi",level,level+1,level+1);         
	  hphirelres[trig][level-1] =dbe_->book1D(name,title, NBINS, -1.0, 1.0);
	  sprintf(title,"(L%i #phi - L%i #phi)/L%i #phi",level,level+1,level+1);         
	  hphirelres[trig][level-1]->setAxisTitle(title, 1);

	  sprintf(name,"HLTMuonL%itoL%i_ptrelrespt",level,level+1);
	  sprintf(title,"L%i Muon #DeltaPt/Pt vs Pt ",level);         
	  hptrelrespt[trig][level-1] =dbe_->bookProfile(name,title, NBINS, 0, pt_max,1,-999.,999.,"s");
	  sprintf(title,"(1/PtL%i - 1/PtL%i)/(1/PtL%i)",level,level+1,level+1);         
	  hptrelrespt[trig][level-1]->setAxisTitle(title, 2);
	  hptrelrespt[trig][level-1]->setAxisTitle("Pt", 1);
	  sprintf(name,"HLTMuonL%itoL%i_phirelresphi",level,level+1);
	  sprintf(title,"L%i Muon #Delta#phi/#phi vs #phi ",level);         
	  hphirelresphi[trig][level-1] =dbe_->bookProfile(name,title, NBINS, -3.15, 3.15,1,-999.,999.,"s");
	  sprintf(title,"(L%i #phi - L%i #phi)/L%i #phi",level,level+1,level+1);         
	  hphirelresphi[trig][level-1]->setAxisTitle(title, 2);
	  hphirelresphi[trig][level-1]->setAxisTitle("#phi", 1);
	  sprintf(name,"HLTMuonL%itoL%i_etarelreseta",level,level+1);
	  sprintf(title,"L%i Muon #Delta#eta/#eta vs #eta ",level);         
	  hetarelreseta[trig][level-1] =dbe_->bookProfile(name,title, NBINS,-2.5, 2.5,1,-999.,999.,"s");
	  sprintf(title,"(L%i #eta - L%i #eta)/L%i #eta",level,level+1,level+1);         
	  hetarelreseta[trig][level-1]->setAxisTitle(title, 2);
	  hetarelreseta[trig][level-1]->setAxisTitle("#eta", 1);
	  // charge conversion
	  sprintf(name,"HLTMuonL%itoL%i_chargeconvers",level,level+1);
	  sprintf(title,"L%i Muon charge #rightarrow L%i Muon charge",level,level+1);         
	  hchargeconv[trig][level-1] =dbe_->book1D(name,title, 4, 0, 4);
	  hchargeconv[trig][level-1]->setAxisTitle(title, 1);
	  hchargeconv[trig][level-1]->setBinLabel(1, "- #rightarrow -", 1);
	  hchargeconv[trig][level-1]->setBinLabel(2, "- #rightarrow +", 1);
	  hchargeconv[trig][level-1]->setBinLabel(3, "+ #rightarrow -", 1);
	  hchargeconv[trig][level-1]->setBinLabel(4, "+ #rightarrow +", 1);
	  // reconstruction fraction with dependence
	  sprintf(name,"HLTMuonL%itoL%i_fracpt",level,level+1);
	  sprintf(title,"#ofL%iMuon/#ofL%iMuon",level+1,level);         
	  hptfrac[trig][level-1] = dbe_->book1D(name,title, 40, 0, pt_max);
	  hptfrac[trig][level-1]->setAxisTitle("Pt", 1);
	  sprintf(name,"HLTMuonL%itoL%i_fraceta",level,level+1);
	  sprintf(title,"#ofL%iMuon/#ofL%iMuon",level+1,level);         
	  hetafrac[trig][level-1] = dbe_->book1D(name,title, 40, -2.5, 2.5);
	  hetafrac[trig][level-1]->setAxisTitle("#eta", 1);
	  sprintf(name,"HLTMuonL%itoL%i_fracphi",level,level+1);
	  sprintf(title,"#ofL%iMuon/#ofL%iMuon",level+1,level);         
	  hphifrac[trig][level-1] = dbe_->book1D(name,title, 40, -3.15, 3.15);
	  hphifrac[trig][level-1]->setAxisTitle("#phi", 1);
	  if (level  == 1 ){
	    // res
	    sprintf(name,"HLTMuonL%itoL3_ptres",level);
	    sprintf(title,"L%iMuon1/Pt - L%iMuon1/Pt",level,level+2);         
	    hptres[trig][level+1] = dbe_->book1D(name,title, NBINS, -0.1, 0.1);
	    sprintf(title,"1/PtL%i - 1/PtL%i",level,level+2);         
	    hptres[trig][level+1]->setAxisTitle(title, 1);
	    sprintf(name,"HLTMuonL%itoL3_etares",level);
	    sprintf(title,"L%iMuon#eta - L3Muon#eta",level);         
	    hetares[trig][level+1] =dbe_->book1D(name,title, NBINS, -0.1, 0.1);
	    sprintf(title,"L%i #eta - L3 #eta",level);         
	    hetares[trig][level+1]->setAxisTitle(title, 1);
	    sprintf(name,"HLTMuonL%itoL3_phires",level);
	    sprintf(title,"L%iMuon#phi - L3Muon#phi",level);         
	    hphires[trig][level+1] =dbe_->book1D(name,title, NBINS, -0.1, 0.1);
	    sprintf(title,"L%i #phi - L3 #phi",level);         
	    hphires[trig][level+1]->setAxisTitle(title, 1);

	    sprintf(name,"HLTMuonL%itoL3_ptrespt",level);
	    sprintf(title,"L%i Muon #Delta Pt vs Pt (wrt L3) ",level);         
	    hptrespt[trig][level+1] =dbe_->bookProfile(name,title, NBINS, 0, pt_max,1,-999.,999.,"s");
	    sprintf(title,"1/PtL%i - 1/PtL3",level);         
	    hptrespt[trig][level+1]->setAxisTitle(title, 2);
	    hptrespt[trig][level+1]->setAxisTitle("Pt", 1);
	    sprintf(name,"HLTMuonL%itoL3_phiresphi",level);
	    sprintf(title,"L%i Muon #Delta#phi vs #phi (wrt L3) ",level);         
	    hphiresphi[trig][level+1] =dbe_->bookProfile(name,title, NBINS, -3.15, 3.15,1,-999.,999.,"s");
	    sprintf(title,"L%i #phi - L3 #phi",level);         
	    hphiresphi[trig][level+1]->setAxisTitle(title, 2);
	    hphiresphi[trig][level+1]->setAxisTitle("#phi", 1);
	    sprintf(name,"HLTMuonL%itoL3_etareseta",level);
	    sprintf(title,"L%i Muon #Delta#eta vs #eta (wrt L3) ",level);         
	    hetareseta[trig][level+1] =dbe_->bookProfile(name,title, NBINS,-2.5, 2.5,1,-999.,999.,"s");
	    sprintf(title,"L%i #eta - L3 #eta",level);         
	    hetareseta[trig][level+1]->setAxisTitle(title, 2);
	    hetareseta[trig][level+1]->setAxisTitle("#eta", 1);

	    // relres
	    sprintf(name,"HLTMuonL%itoL3_ptrelres",level);
	    sprintf(title,"(L%iMuon1/Pt - L%iMuon1/Pt)/(L%iMuon1/Pt)",level,level+2,level+2); 
	    hptrelres[trig][level+1] = dbe_->book1D(name,title, NBINS, -1.0, 1.0);
	    sprintf(title,"(1/PtL%i - 1/PtL3)/(1/PtL3)",level);         
	    hptrelres[trig][level+1]->setAxisTitle(title, 1);
	    sprintf(name,"HLTMuonL%itoL3_etarelres",level);
	    sprintf(title,"(L%iMuon#eta - L3Muon#eta)/L3Muon#eta",level);         
	    hetarelres[trig][level+1] =dbe_->book1D(name,title, NBINS, -1.0, 1.0);
	    sprintf(title,"(L%i #eta - L3 #eta)/L3 #eta",level);         
	    hetarelres[trig][level+1]->setAxisTitle(title, 1);
	    sprintf(name,"HLTMuonL%itoL3_phirelres",level);
	    sprintf(title,"(L%iMuon#phi - L3Muon#phi)/L3Muon#phi",level);         
	    hphirelres[trig][level+1] =dbe_->book1D(name,title, NBINS, -1.0, 1.0);
	    sprintf(title,"(L%i #phi - L3 #phi)/L3 #phi",level);         
	    hphirelres[trig][level+1]->setAxisTitle(title, 1);

	    sprintf(name,"HLTMuonL%itoL3_ptrelrespt",level);
	    sprintf(title,"L%i Muon #DeltaPt/Pt vs Pt (wrt L3) ",level);         
	    hptrelrespt[trig][level+1] =dbe_->bookProfile(name,title, NBINS, 0, pt_max,1,-999.,999.,"s");
	    sprintf(title,"(1/PtL%i - 1/PtL3)/(1/PtL3)",level);         
	    hptrelrespt[trig][level+1]->setAxisTitle(title, 2);
	    hptrelrespt[trig][level+1]->setAxisTitle("Pt", 1);
	    sprintf(name,"HLTMuonL%itoL3_phirelresphi",level);
	    sprintf(title,"L%i Muon #Delta#phi/#phi vs #phi (wrt L3) ",level);         
	    hphirelresphi[trig][level+1] =dbe_->bookProfile(name,title, NBINS, -3.15, 3.15,1,-999.,999.,"s");
	    sprintf(title,"(L%i #phi - L3 #phi)/L3 #phi",level);         
	    hphirelresphi[trig][level+1]->setAxisTitle(title, 2);
	    hphirelresphi[trig][level+1]->setAxisTitle("#phi", 1);
	    sprintf(name,"HLTMuonL%itoL3_etarelreseta",level);
	    sprintf(title,"L%i Muon #Delta#eta/#eta vs #eta (wrt L3) ",level);         
	    hetarelreseta[trig][level+1] =dbe_->bookProfile(name,title, NBINS,-2.5, 2.5,1,-999.,999.,"s");
	    sprintf(title,"(L%i #eta - L3 #eta)/L3 #eta",level);         
	    hetarelreseta[trig][level+1]->setAxisTitle(title, 2);
	    hetarelreseta[trig][level+1]->setAxisTitle("#eta", 1);

	    sprintf(name,"HLTMuonL%itoL3_chargeconvers",level);
	    sprintf(title,"L%i Muon charge #rightarrow L3 Muon charge",level);         
	    hchargeconv[trig][level+1] =dbe_->book1D(name,title, 4, 0, 4);
	    hchargeconv[trig][level+1]->setAxisTitle(title, 1);
	    hchargeconv[trig][level+1]->setBinLabel(1, "- #rightarrow -", 1);
	    hchargeconv[trig][level+1]->setBinLabel(2, "- #rightarrow +", 1);
	    hchargeconv[trig][level+1]->setBinLabel(3, "+ #rightarrow -", 1);
	    hchargeconv[trig][level+1]->setBinLabel(4, "+ #rightarrow +", 1);
	  }
	}
      }
      dbe_->showDirStructure();
    }
    
    // Muon det id is 2 pushed in bits 28:31
    const unsigned int detector_id = 2<<28;
    dbe_->tagContents(monitorName_, detector_id);
  } 
  
  for( int trig = 0; trig < nTrigs+1; trig++ ) {
    for( int level = 1; level < 3; ++level ) {
      char name[512];
      double pt_max;
      if( level == 1 ) pt_max = 140;
      else pt_max = 200;
      sprintf(name, "DenominatorL%iptTrig%i", level, trig);
      _hpt1[trig][level-1] = new TH1D(name, name, 40, 0, pt_max);
      sprintf(name, "NumeratorL%iptTrig%i", level, trig);
      _hpt2[trig][level-1] = new TH1D(name, name, 40, 0, pt_max);
      sprintf(name, "DenominatorL%ietaTrig%i", level, trig);
      _heta1[trig][level-1] = new TH1D(name, name, 40, -2.5, 2.5);
      sprintf(name, "NumeratorL%ietaTrig%i", level, trig);
      _heta2[trig][level-1] = new TH1D(name, name, 40, -2.5, 2.5);
      sprintf(name, "DenominatorL%iphiTrig%i", level, trig);
      _hphi1[trig][level-1] = new TH1D(name, name, 40, -3.15, 3.15);
      sprintf(name, "NumeratorL%iphiTrig%i", level, trig);
      _hphi2[trig][level-1] = new TH1D(name, name, 40, -3.15, 3.15);
    }
  }
}

//--------------------------------------------------------
void HLTMuonDQMSource::beginRun(const edm::Run& r, const EventSetup& context) {
  
}

//--------------------------------------------------------
void HLTMuonDQMSource::beginLuminosityBlock(const LuminosityBlock& lumiSeg, 
					    const EventSetup& context) {
  
}

// ----------------------------------------------------------
void HLTMuonDQMSource::analyze(const Event& iEvent, 
			       const EventSetup& iSetup )
{  
  if ( !dbe_) return;
  counterEvt_++;
  if (prescaleEvt_ > 0 && counterEvt_%prescaleEvt_!=0) return;
  LogDebug("HLTMuonDQMSource") << " processing conterEvt_: " << counterEvt_ <<endl;
  
  bool trigFired = false;
  bool FiredTriggers[NTRIG] = {false};
  Handle<TriggerResults> trigResult;
  iEvent.getByLabel(InputTag("TriggerResults"), trigResult);
  if( !trigResult.failedToGet() ) {
    int ntrigs = trigResult->size();
    TriggerNames trigName;
    trigName.init(*trigResult);
    for( int itrig = 0; itrig != ntrigs; ++itrig) {
      for( unsigned int n = 0; n < (unsigned int)nTrigs; n++) { 
	if( trigName.triggerIndex(theHLTCollectionLabels[n]) == (unsigned int)ntrigs ) continue;
        if( trigResult->accept(trigName.triggerIndex(theHLTCollectionLabels[n])) ) {
	  FiredTriggers[n] = true;
	  trigFired = true;
	}
      }
    }
  }
  // trigger fired
  if( !trigFired ) return;
  nTrig_++;
  // combined results
  FiredTriggers[nTrigs] = true;
  
  Handle<RecoChargedCandidateCollection> l2mucands, l3mucands;
  iEvent.getByLabel (l2collectionTag_,l2mucands);
  iEvent.getByLabel (l3collectionTag_,l3mucands);
  RecoChargedCandidateCollection::const_iterator cand, cand2, cand3;
  
  Handle<L2MuonTrajectorySeedCollection> l2seeds; 
  iEvent.getByLabel (l2seedscollectionTag_,l2seeds);
  Handle<L3MuonTrajectorySeedCollection> l3seeds; 
  iEvent.getByLabel (l3seedscollectionTag_,l3seeds);

  
  for( int ntrig = 0; ntrig < nTrigs+1; ntrig++ ) {
    if( !FiredTriggers[ntrig] ) continue;
    if( !l2seeds.failedToGet() ) {
      hNMu[ntrig][3]->Fill(l2seeds->size());
      L2MuonTrajectorySeedCollection::const_iterator l2seed;
      map<L1MuonParticleRef, int> l1map;
      for (l2seed=l2seeds->begin() ; l2seed != l2seeds->end();++l2seed){
	PTrajectoryStateOnDet state=l2seed->startingState();
	float pt=state.parameters().momentum().perp();
	float eta=state.parameters().momentum().phi();
	float phi=state.parameters().momentum().eta();
	hcharge[ntrig][3]->Fill(state.parameters().charge());
	hpt[ntrig][3]->Fill(pt);
	hphi[ntrig][3]->Fill(phi);
	heta[ntrig][3]->Fill(eta);
	hetaphi[ntrig][3]->Fill(phi,eta);
	hptphi[ntrig][3]->Fill(pt,phi);
	hpteta[ntrig][3]->Fill(pt,eta);
	L1MuonParticleRef l1ref = l2seed->l1Particle();
	l1map[l1ref]++;
	hseedptres[ntrig][0]->Fill(1/pt - 1/l1ref->pt());
	hseedetares[ntrig][0]->Fill(eta - l1ref->eta());
	hseedphires[ntrig][0]->Fill(phi - l1ref->phi());
	hseedptrelres[ntrig][0]->Fill((1/pt - 1/l1ref->pt())/(1/l1ref->pt()));
	hseedetarelres[ntrig][0]->Fill((eta - l1ref->eta())/l1ref->eta());
	hseedphirelres[ntrig][0]->Fill((phi - l1ref->phi())/l1ref->phi());

	hcharge[ntrig][0]->Fill(l1ref->charge());
	hpt[ntrig][0]->Fill(l1ref->pt());
	hphi[ntrig][0]->Fill(l1ref->phi());
	heta[ntrig][0]->Fill(l1ref->eta());
	hetaphi[ntrig][0]->Fill(l1ref->phi(),l1ref->eta());
	hptphi[ntrig][0]->Fill(l1ref->pt(),l1ref->phi());
	hpteta[ntrig][0]->Fill(l1ref->pt(),l1ref->eta());
	hl1quality[ntrig]->Fill(l1ref->gmtMuonCand().quality());
	_hpt1[ntrig][0]->Fill(l1ref->pt());
	_heta1[ntrig][0]->Fill(l1ref->eta());
	_hphi1[ntrig][0]->Fill(l1ref->phi());
	if ( !l2mucands.failedToGet()) {
	  for (cand=l2mucands->begin(); cand!=l2mucands->end(); ++cand) {
	    TrackRef tk = cand->get<TrackRef>();
	    RefToBase<TrajectorySeed> seed=tk->seedRef();
	    if ( (l2seed->startingState()).detId() == (seed->startingState()).detId() ) {
	      if(tk->pt()*l1ref->pt() != 0 ) {
		hptres[ntrig][0]->Fill(1/l1ref->pt() - 1/tk->pt());
		hptrespt[ntrig][0]->Fill(tk->pt(), 1/l1ref->pt() - 1/tk->pt());
		hptrelres[ntrig][0]->Fill((1/l1ref->pt() - 1/tk->pt())/(1/tk->pt()));
		hptrelrespt[ntrig][0]->Fill(tk->pt(), (1/l1ref->pt() - 1/tk->pt())/(1/tk->pt()));
	      }
	      _hpt2[ntrig][0]->Fill(l1ref->pt());
	      _heta2[ntrig][0]->Fill(l1ref->eta());
	      _hphi2[ntrig][0]->Fill(l1ref->phi());
	      hetares[ntrig][0]->Fill(l1ref->eta()-tk->eta());
	      hetareseta[ntrig][0]->Fill(tk->eta(),l1ref->eta()-tk->eta());
	      hetarelres[ntrig][0]->Fill((l1ref->eta()-tk->eta())/tk->eta());
	      hetarelreseta[ntrig][0]->Fill(tk->eta(),(l1ref->eta()-tk->eta())/tk->eta());
	      hphires[ntrig][0]->Fill(l1ref->phi()-tk->phi());
	      double dphi=l1ref->phi()-tk->phi();
	      if (dphi>TMath::TwoPi())dphi-=2*TMath::TwoPi();
	      else if (dphi<-TMath::TwoPi()) dphi+=TMath::TwoPi();
	      hphiresphi[ntrig][0]->Fill(tk->phi(),dphi);
	      hphirelres[ntrig][0]->Fill((l1ref->phi()-tk->phi())/tk->phi());
	      hphirelresphi[ntrig][0]->Fill(tk->phi(),dphi/tk->phi());
	      // charge conversion
	      int chargeconv = -1;
	      int l1charge = l1ref->charge();
	      int l2charge = tk->charge();
	      if( l1charge == -1 && l2charge == -1 ) chargeconv = 0;
	      else if( l1charge == -1 && l2charge == 1 ) chargeconv = 1;
	      else if( l1charge == 1 && l2charge == -1 ) chargeconv = 2;
	      else if( l1charge == 1 && l2charge == 1 ) chargeconv = 3;
	      hchargeconv[ntrig][0]->Fill(chargeconv);
	      _hpt1[ntrig][1]->Fill(tk->pt());
	      _heta1[ntrig][1]->Fill(tk->eta());
	      _hphi1[ntrig][1]->Fill(tk->phi());
	      //find the L3 build from this L2
	      if (!l3mucands.failedToGet()) {
		for (cand=l3mucands->begin(); cand!=l3mucands->end(); ++cand) {
		  TrackRef l3tk= cand->get<TrackRef>();
		  if( l3tk->seedRef().castTo<Ref<L3MuonTrajectorySeedCollection> > ().isAvailable() ) {
		    if (l3tk->seedRef().castTo<Ref<L3MuonTrajectorySeedCollection> >()->l2Track() == tk){
		      if(l1ref->pt()*l3tk->pt() != 0 ) {
			hptres[ntrig][2]->Fill(1/l1ref->pt() - 1/l3tk->pt());
			hptrespt[ntrig][2]->Fill(l3tk->pt(), 1/l1ref->pt() - 1/l3tk->pt());
			hptrelres[ntrig][2]->Fill((1/l1ref->pt() - 1/l3tk->pt())/(1/l3tk->pt()));
			hptrelrespt[ntrig][2]->Fill(l3tk->pt(), (1/l1ref->pt() - 1/l3tk->pt())/(1/l3tk->pt()));
		      }
		      hetares[ntrig][2]->Fill(l1ref->eta()-l3tk->eta());
		      hetareseta[ntrig][2]->Fill(l1ref->eta(),l1ref->eta()-l3tk->eta());
		      hetarelres[ntrig][2]->Fill((l1ref->eta()-l3tk->eta())/l3tk->eta());
		      hetarelreseta[ntrig][2]->Fill(l1ref->eta(),(l1ref->eta()-l3tk->eta())/l3tk->eta());
		      hphires[ntrig][2]->Fill(l1ref->phi()-l3tk->phi());
		      double dphi=l1ref->phi()-l3tk->phi();
		      if (dphi>TMath::TwoPi())dphi-=2*TMath::TwoPi();
		      else if (dphi<-TMath::TwoPi()) dphi+=TMath::TwoPi();
		      hphiresphi[ntrig][2]->Fill(l3tk->phi(),dphi);
		      hphirelres[ntrig][2]->Fill((l1ref->phi()-l3tk->phi())/l3tk->phi());
		      hphirelresphi[ntrig][2]->Fill(l3tk->phi(),(dphi)/l3tk->phi());
		      // charge conversion
		      int chargeconv = -1;
		      int l1charge = l1ref->charge();
		      int l3charge = l3tk->charge();
		      if( l1charge == -1 && l3charge == -1 ) chargeconv = 0;
		      else if( l1charge == -1 && l3charge == 1 ) chargeconv = 1;
		      else if( l1charge == 1 && l3charge == -1 ) chargeconv = 2;
		      else if( l1charge == 1 && l3charge == 1 ) chargeconv = 3;
		      hchargeconv[ntrig][2]->Fill(chargeconv);
		      _hpt2[ntrig][1]->Fill(tk->pt());
    		      _heta2[ntrig][1]->Fill(tk->eta());
		      _hphi2[ntrig][1]->Fill(tk->phi());
		      //break; //plot only once per L2?
		    }//if
		  }
		}//for
	      }
	      break;
	    }
	  }
	}
      }
      // mapping
      map<L1MuonParticleRef, int>::iterator it;
      for( it = l1map.begin(); it != l1map.end(); it++ ) {
	  hseedNMuper[ntrig][0]->Fill(it->second);
      }
      hNMu[ntrig][0]->Fill(l1map.size());
    }
    if (!l3seeds.failedToGet()) {
      hNMu[ntrig][4]->Fill(l3seeds->size());
      L3MuonTrajectorySeedCollection::const_iterator l3seed;
      map<TrackRef, int> l2map;
      for (l3seed=l3seeds->begin() ; l3seed != l3seeds->end();++l3seed){
	PTrajectoryStateOnDet state=l3seed->startingState();
	float pt=state.parameters().momentum().perp();
	float eta=state.parameters().momentum().phi();
	float phi=state.parameters().momentum().eta();
	hcharge[ntrig][4]->Fill(state.parameters().charge());
	hpt[ntrig][4]->Fill(pt);
	hphi[ntrig][4]->Fill(phi);
	heta[ntrig][4]->Fill(eta);
	hetaphi[ntrig][4]->Fill(phi,eta);
	hptphi[ntrig][4]->Fill(pt,phi);
	hpteta[ntrig][4]->Fill(pt,eta);

	TrackRef l2tkRef = l3seed->l2Track();
	l2map[l2tkRef]++;
	hseedptres[ntrig][1]->Fill(1/pt - 1/l2tkRef->pt());
	hseedetares[ntrig][1]->Fill(eta - l2tkRef->eta());
	hseedphires[ntrig][1]->Fill(phi - l2tkRef->phi());
	hseedptrelres[ntrig][1]->Fill((1/pt - 1/l2tkRef->pt())/(1/l2tkRef->pt()));
	hseedetarelres[ntrig][1]->Fill((eta - l2tkRef->eta())/l2tkRef->eta());
	hseedphirelres[ntrig][1]->Fill((phi - l2tkRef->phi())/l2tkRef->phi());
      }
      // mapping
      map<TrackRef, int>::iterator it;
      for( it = l2map.begin(); it != l2map.end(); it++ ) {
	  hseedNMuper[ntrig][1]->Fill(it->second);
      }
    }
    
    reco::BeamSpot beamSpot;
    edm::Handle<reco::BeamSpot> recoBeamSpotHandle;
    iEvent.getByLabel("hltOfflineBeamSpot",recoBeamSpotHandle);
    if (!recoBeamSpotHandle.failedToGet())  beamSpot = *recoBeamSpotHandle;
    
    if (!l2mucands.failedToGet()) {
      LogDebug("HLTMuonDQMSource") << " filling L2 stuff " << endl;
      Handle<reco::IsoDepositMap> l2depMap;
      iEvent.getByLabel (l2isolationTag_,l2depMap);
      hNMu[ntrig][1]->Fill(l2mucands->size());
      for (cand=l2mucands->begin(); cand!=l2mucands->end(); ++cand) {
	TrackRef tk = cand->get<TrackRef>();
	if (!l2depMap.failedToGet()) {
	  LogDebug("HLTMuonDQMSource") << " filling L2 Iso stuff " << endl;
	  if ( l2depMap->contains(tk.id()) ){
	    reco::IsoDepositMap::value_type calDeposit = (*l2depMap)[tk];
	    double dephlt = calDeposit.depositWithin(coneSize_);
	    if( dephlt != 0 ) hiso[ntrig][0]->Fill(dephlt);
	  }
	}
	
	// eta cut
	hpt[ntrig][1]->Fill(tk->pt());      
	hcharge[ntrig][1]->Fill(tk->charge()); 
	if ( tk->charge() != 0 ) {
	  heta[ntrig][1]->Fill(tk->eta());      
	  hphi[ntrig][1]->Fill(tk->phi()); 
	  hetaphi[ntrig][1]->Fill(tk->phi(),tk->eta()); 
	  hptphi[ntrig][1]->Fill(tk->pt(),tk->phi()); 
	  hpteta[ntrig][1]->Fill(tk->pt(),tk->eta()); 
	  const reco::HitPattern& hitp = tk->hitPattern();
	  hnHits[ntrig][1]->Fill(hitp.numberOfHits()); 
	  hnValidHits[ntrig]->Fill(hitp.numberOfValidHits()); 
	  hd0[ntrig][0]->Fill(tk->d0()); 
	  if (!recoBeamSpotHandle.failedToGet()){
	    hdr[ntrig][0]->Fill(tk->dxy(beamSpot.position()));	
	    hdrphi[ntrig][0]->Fill(tk->phi(),tk->dxy(beamSpot.position())); 
	  } 
	  hd0phi[ntrig][0]->Fill(tk->phi(),tk->d0()); 
	  hdz0[ntrig][0]->Fill(tk->dz()); 
	  hdz0eta[ntrig][0]->Fill(tk->eta(),tk->dz());
	  hdz[ntrig][0]->Fill(tk->dz(beamSpot.position())); 
	  hdzeta[ntrig][0]->Fill(tk->eta(),tk->dz(beamSpot.position()));
	  herr0[ntrig][0]->Fill(tk->error(0)); 
	  cand2=cand;
	  ++cand2;
	  for (; cand2!=l2mucands->end(); cand2++) {
	    TrackRef tk2=cand2->get<TrackRef>();
	    if ( tk->charge()*tk2->charge() == -1 ){
	      double mass=(cand->p4()+cand2->p4()).M();
	      hdimumass[ntrig][0]->Fill(mass);
	    }
	  }
	} else LogWarning("HLTMonMuon")<<"stop filling candidate with update@Vtx failure";
      }
    }
    if (!l3mucands.failedToGet()) {
      LogDebug("HLTMuonDQMSource") << " filling L3 stuff " << endl;
      hNMu[ntrig][2]->Fill(l3mucands->size());
      Handle<reco::IsoDepositMap> l3depMap;
      iEvent.getByLabel (l3isolationTag_,l3depMap);
      for (cand=l3mucands->begin(); cand!=l3mucands->end(); ++cand) {
	TrackRef tk = cand->get<TrackRef>();
	if (!l3depMap.failedToGet()) {
	  if ( l3depMap->contains(tk.id()) ){
	    reco::IsoDepositMap::value_type calDeposit= (*l3depMap)[tk];
	    double dephlt = calDeposit.depositWithin(coneSize_);
	    if( dephlt != 0 ) hiso[ntrig][1]->Fill(dephlt);
	  }
	}
	// eta cut
	hpt[ntrig][2]->Fill(tk->pt());      
	heta[ntrig][2]->Fill(tk->eta());      
	hphi[ntrig][2]->Fill(tk->phi()); 
	hetaphi[ntrig][2]->Fill(tk->phi(),tk->eta()); 
	hptphi[ntrig][2]->Fill(tk->pt(),tk->phi()); 
	hpteta[ntrig][2]->Fill(tk->pt(),tk->eta()); 
	const reco::HitPattern& hitp = tk->hitPattern();
	hnHits[ntrig][2]->Fill(hitp.numberOfHits()); 
	hnTkValidHits[ntrig]->Fill(hitp.numberOfValidTrackerHits()); 
	hnMuValidHits[ntrig]->Fill(hitp.numberOfValidMuonHits()); 
	hd0[ntrig][1]->Fill(tk->d0()); 
	if (!recoBeamSpotHandle.failedToGet()) {
	  hdr[ntrig][1]->Fill(tk->dxy(beamSpot.position()));
	  hdrphi[ntrig][1]->Fill(tk->phi(),tk->dxy(beamSpot.position())); 
	}
	hd0phi[ntrig][1]->Fill(tk->phi(),tk->d0()); 
	hdz0[ntrig][1]->Fill(tk->dz()); 
	hdz0eta[ntrig][1]->Fill(tk->eta(),tk->dz());
	hdz[ntrig][1]->Fill(tk->dz(beamSpot.position())); 
	hdzeta[ntrig][1]->Fill(tk->eta(),tk->dz(beamSpot.position()));
	herr0[ntrig][1]->Fill(tk->error(0)); 
	hcharge[ntrig][2]->Fill(tk->charge()); 
	cand2=cand;
	++cand2;
	
	for (; cand2!=l3mucands->end(); cand2++) {
	  TrackRef tk2=cand2->get<TrackRef>();
	  if ( tk->charge()*tk2->charge() == -1 ){
	    double mass=(cand->p4()+cand2->p4()).M();
	    hdimumass[ntrig][1]->Fill(mass);
	  }
	}
	if( tk->seedRef().castTo<Ref<L3MuonTrajectorySeedCollection> >().isAvailable() ) {
	  TrackRef l2tk = tk->seedRef().castTo<Ref<L3MuonTrajectorySeedCollection> >()->l2Track();
	  if(tk->pt()*l2tk->pt() != 0 ) {
	    hptres[ntrig][1]->Fill(1/l2tk->pt() - 1/tk->pt());
	    hptrespt[ntrig][1]->Fill(tk->pt(), 1/l2tk->pt() - 1/tk->pt());
	    hptrelres[ntrig][1]->Fill((1/l2tk->pt() - 1/tk->pt())/(1/tk->pt()));
	    hptrelrespt[ntrig][1]->Fill(tk->pt(), (1/l2tk->pt() - 1/tk->pt())/(1/tk->pt()));
	    double pterr = (tk->ptError()/(tk->pt()*tk->pt()));
	    hptpull[ntrig]->Fill((1/l2tk->pt() - 1/tk->pt())/pterr);
	    hptpullpt[ntrig]->Fill(tk->pt(), (1/l2tk->pt() - 1/tk->pt())/pterr);
	  }
	  hphires[ntrig][1]->Fill(l2tk->phi()-tk->phi());
	  double dphi=l2tk->phi()-tk->phi();
	  if (dphi>TMath::TwoPi())dphi-=2*TMath::TwoPi();
	  else if (dphi<-TMath::TwoPi()) dphi+=TMath::TwoPi();
	  hphiresphi[ntrig][1]->Fill(tk->phi(),dphi);
	  hphirelres[ntrig][1]->Fill((l2tk->phi()-tk->phi())/tk->phi());
	  hphirelresphi[ntrig][1]->Fill(tk->phi(),dphi/tk->phi());
	  hphipull[ntrig]->Fill(dphi/tk->phiError());
	  hphipullphi[ntrig]->Fill(tk->phi(), dphi/tk->phiError());
	  hetares[ntrig][1]->Fill(l2tk->eta()-tk->eta());
	  hetareseta[ntrig][1]->Fill(tk->eta(),l2tk->eta()-tk->eta());
	  hetarelres[ntrig][1]->Fill((l2tk->eta()-tk->eta())/tk->eta());
	  hetarelreseta[ntrig][1]->Fill(tk->eta(),(l2tk->eta()-tk->eta())/tk->eta());
	  hetapull[ntrig]->Fill((l2tk->eta()-tk->eta())/tk->etaError());
	  hetapulleta[ntrig]->Fill(tk->eta(),(l2tk->eta()-tk->eta())/tk->etaError());
	  // charge conversion
	  int chargeconv = -1;
	  int l2charge = l2tk->charge();
	  int l3charge = tk->charge();
	  if( l2charge == -1 && l3charge == -1 ) chargeconv = 0;
	  else if( l2charge == -1 && l3charge == 1 ) chargeconv = 1;
	  else if( l2charge == 1 && l3charge == -1 ) chargeconv = 2;
	  else if( l2charge == 1 && l3charge == 1 ) chargeconv = 3;
	  hchargeconv[ntrig][1]->Fill(chargeconv);
	}
      }
    }  
    
    for( int level = 0; level < 2; level++ ) {  
      for( int nbin = 1; nbin < _hpt1[ntrig][level]->GetNbinsX()+1; nbin++ ) {
	if( _hpt1[ntrig][level]->GetBinContent(nbin) != 0 ) {
	  double frac = _hpt2[ntrig][level]->GetBinContent(nbin)/_hpt1[ntrig][level]->GetBinContent(nbin);
	  double err = sqrt(frac*fabs(1 - frac)/_hpt1[ntrig][level]->GetBinContent(nbin));
	  hptfrac[ntrig][level]->setBinContent(nbin, frac);
	  hptfrac[ntrig][level]->setBinError(nbin, err);
	}
	if( _heta1[ntrig][level]->GetBinContent(nbin) != 0 ) {
	  double frac = _heta2[ntrig][level]->GetBinContent(nbin)/_heta1[ntrig][level]->GetBinContent(nbin);
	  double err = sqrt(frac*fabs(1 - frac)/_heta1[ntrig][level]->GetBinContent(nbin));
	  hetafrac[ntrig][level]->setBinContent(nbin, frac);
	  hetafrac[ntrig][level]->setBinError(nbin, err);
	}
	if( _hphi1[ntrig][level]->GetBinContent(nbin) != 0 ) {
	  double frac = _hphi2[ntrig][level]->GetBinContent(nbin)/_hphi1[ntrig][level]->GetBinContent(nbin);
	  double err = sqrt(frac*fabs(1 - frac)/_hphi1[ntrig][level]->GetBinContent(nbin));
	  hphifrac[ntrig][level]->setBinContent(nbin, frac);
	  hphifrac[ntrig][level]->setBinError(nbin, err);
	}
	else {
	  hptfrac[ntrig][level]->setBinContent(nbin, 0.0);
	  hetafrac[ntrig][level]->setBinContent(nbin, 0.0);
	  hphifrac[ntrig][level]->setBinContent(nbin, 0.0);
	  hptfrac[ntrig][level]->setBinError(nbin, 0.0);
	  hetafrac[ntrig][level]->setBinError(nbin, 0.0);
	  hphifrac[ntrig][level]->setBinError(nbin, 0.0);
	}
      }
    }
  }
}




//--------------------------------------------------------
void HLTMuonDQMSource::endLuminosityBlock(const LuminosityBlock& lumiSeg, 
					  const EventSetup& context) {
}
//--------------------------------------------------------
void HLTMuonDQMSource::endRun(const Run& r, const EventSetup& context){
}
//--------------------------------------------------------
void HLTMuonDQMSource::endJob(){
  LogInfo("HLTMonMuon") << "analyzed " << counterEvt_ << " events";
  //cout << "analyzed = " << counterEvt_ << " , triggered = " << nTrig_ << endl;
  
  
  //if (outputFile_.size() != 0 && dbe_)
  //dbe_->save(outputFile_);
  
  return;
}
