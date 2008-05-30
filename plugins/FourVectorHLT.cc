// $Id: FourVectorHLT.cc,v 1.6 2008/05/26 09:31:14 wittich Exp $
// See header file for information. 
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DQM/HLTEvF/interface/FourVectorHLT.h"

#include "DataFormats/HLTReco/interface/TriggerObject.h"
#include "FWCore/Framework/interface/TriggerNames.h"
#include "DataFormats/HLTReco/interface/TriggerEvent.h"

#include "DQMServices/Core/interface/MonitorElement.h"

using namespace edm;

FourVectorHLT::FourVectorHLT(const edm::ParameterSet& iConfig):
  resetMe_(true),  currentRun_(-99)
{
  LogDebug("FourVectorHLT") << "constructor...." ;

  dbe_ = Service < DQMStore > ().operator->();
  if ( ! dbe_ ) {
    LogWarning("FourVectorHLT") << "unabel to get DQMStore service?";
  }
  if (iConfig.getUntrackedParameter < bool > ("DQMStore", false)) {
    dbe_->setVerbose(0);
  }
  
  
  dirname_="HLT/FourVectorHLT" + 
    iConfig.getParameter<std::string>("@module_label");
  
  if (dbe_ != 0 ) {
    dbe_->setCurrentFolder(dirname_);
  }
  
  
  // plotting paramters
  ptMin_ = iConfig.getUntrackedParameter<double>("ptMin",0.);
  ptMax_ = iConfig.getUntrackedParameter<double>("ptMax",1000.);
  nBins_ = iConfig.getUntrackedParameter<unsigned int>("Nbins",40);
  
  plotAll_ = iConfig.getUntrackedParameter<bool>("plotAll", false);

  // this is the list of paths to look at.
  std::vector<edm::ParameterSet> filters = 
    iConfig.getParameter<std::vector<edm::ParameterSet> >("filters");
  for(std::vector<edm::ParameterSet>::iterator 
	filterconf = filters.begin() ; filterconf != filters.end(); 
      filterconf++) {
    std::string me  = filterconf->getParameter<std::string>("name");
    int objectType = filterconf->getParameter<unsigned int>("type");
    float ptMin = filterconf->getUntrackedParameter<double>("ptMin");
    float ptMax = filterconf->getUntrackedParameter<double>("ptMax");
    hltPaths_.push_back(PathInfo(me, objectType, ptMin, ptMax));
  }
  if ( hltPaths_.size() && plotAll_) {
    // these two ought to be mutually exclusive....
    LogWarning("FourVectorHLT") << "Using both plotAll and a list. "
      "list will be ignored." ;
    hltPaths_.clear();
  }
  triggerSummaryLabel_ = 
    iConfig.getParameter<edm::InputTag>("triggerSummaryLabel");
 
  
}


FourVectorHLT::~FourVectorHLT()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called to for each event  ------------
void
FourVectorHLT::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace edm;
  using namespace trigger;
  ++nev_;
  LogDebug("FourVectorHLT")<< "FourVectorHLT: analyze...." ;
  
  edm::Handle<TriggerEvent> triggerObj;
  iEvent.getByLabel(triggerSummaryLabel_,triggerObj); 
  if(!triggerObj.isValid()) { 
    edm::LogWarning("FourVectorHLT") << "Summary HLT objects not found, "
      "skipping event"; 
    return;
  }
  

  

  const trigger::TriggerObjectCollection & toc(triggerObj->getObjects());

  if ( plotAll_ ) {
    for ( size_t ia = 0; ia < triggerObj->sizeFilters(); ++ ia) {
      std::string name = triggerObj->filterTag(ia).encode();
      PathInfoCollection::iterator pic =  hltPaths_.find(name);
      if ( pic == hltPaths_.end() ) {
	// doesn't exist - add it
	MonitorElement *et(0), *eta(0), *phi(0), *etavsphi(0);
	std::string histoname(name+"_et");
	std::string title(name+" E_t");
	et =  dbe_->book1D(histoname.c_str(),
			  title.c_str(),nBins_, 0, 100);
      
	histoname = name+"_eta";
	title = name+" #eta";
	eta =  dbe_->book1D(histoname.c_str(),
			   title.c_str(),nBins_,-2.7,2.7);
      
	histoname = name+"_phi";
	title = name+" #phi";
	phi =  dbe_->book1D(histoname.c_str(),
			   histoname.c_str(),nBins_,-3.14,3.14);
      
      
	histoname = name+"_etaphi";
	title = name+" #eta vs #phi";
	etavsphi =  dbe_->book2D(histoname.c_str(),
				title.c_str(),
				nBins_,-2.7,2.7,
				nBins_,-3.14, 3.14);
      
	// no idea how to get the bin boundries in this mode
	PathInfo e(name,0, et, eta, phi, etavsphi, 0,100);
	hltPaths_.push_back(e);  
	pic = hltPaths_.begin() + hltPaths_.size()-1;
      }
      const trigger::Keys & k = triggerObj->filterKeys(ia);
      for (trigger::Keys::const_iterator ki = k.begin(); ki !=k.end(); ++ki ) {
	pic->getEtHisto()->Fill(toc[*ki].pt());
	pic->getEtaHisto()->Fill(toc[*ki].eta());
	pic->getPhiHisto()->Fill(toc[*ki].phi());
	pic->getEtaVsPhiHisto()->Fill(toc[*ki].eta(), toc[*ki].phi());
      }  

    }

  }
  else { // not plotAll_
    for(PathInfoCollection::iterator v = hltPaths_.begin();
	v!= hltPaths_.end(); ++v ) {
      const int index = triggerObj->filterIndex(v->getName());
      if ( index >= triggerObj->sizeFilters() ) {
	continue; // not in this event
      }
      LogDebug("FourVectorHLT") << "filling ... " ;
      const trigger::Keys & k = triggerObj->filterKeys(index);
      for (trigger::Keys::const_iterator ki = k.begin(); ki !=k.end(); ++ki ) {
	v->getEtHisto()->Fill(toc[*ki].pt());
	v->getEtaHisto()->Fill(toc[*ki].eta());
	v->getPhiHisto()->Fill(toc[*ki].phi());
	v->getEtaVsPhiHisto()->Fill(toc[*ki].eta(), toc[*ki].phi());
      }  
    }
  }
}


// -- method called once each job just before starting event loop  --------
void 
FourVectorHLT::beginJob(const edm::EventSetup&)
{
  nev_ = 0;
  DQMStore *dbe = 0;
  dbe = Service<DQMStore>().operator->();
  
  if (dbe) {
    dbe->setCurrentFolder(dirname_);
    dbe->rmdir(dirname_);
  }
  
  
  if (dbe) {
    dbe->setCurrentFolder(dirname_);

    if ( ! plotAll_ ) {
      for(PathInfoCollection::iterator v = hltPaths_.begin();
	  v!= hltPaths_.end(); ++v ) {
	MonitorElement *et, *eta, *phi, *etavsphi=0;
	std::string histoname(v->getName()+"_et");
	std::string title(v->getName()+" E_t");
	et =  dbe->book1D(histoname.c_str(),
			  title.c_str(),nBins_,
			  v->getPtMin(),
			  v->getPtMax());
      
	histoname = v->getName()+"_eta";
	title = v->getName()+" #eta";
	eta =  dbe->book1D(histoname.c_str(),
			   title.c_str(),nBins_,-2.7,2.7);

	histoname = v->getName()+"_phi";
	title = v->getName()+" #phi";
	phi =  dbe->book1D(histoname.c_str(),
			   histoname.c_str(),nBins_,-3.14,3.14);
 

	histoname = v->getName()+"_etaphi";
	title = v->getName()+" #eta vs #phi";
	etavsphi =  dbe->book2D(histoname.c_str(),
				title.c_str(),
				nBins_,-2.7,2.7,
				nBins_,-3.14, 3.14);
      
	v->setHistos( et, eta, phi, etavsphi);
      } 
    } // ! plotAll_ - for plotAll we discover it during the event
  }
}

// - method called once each job just after ending the event loop  ------------
void 
FourVectorHLT::endJob() 
{
   LogInfo("FourVectorHLT") << "analyzed " << nev_ << " events";
   return;
}


// BeginRun
void FourVectorHLT::beginRun(const edm::Run& run, const edm::EventSetup& c)
{
  LogDebug("FourVectorHLT") << "beginRun, run " << run.id();
}

/// EndRun
void FourVectorHLT::endRun(const edm::Run& run, const edm::EventSetup& c)
{
  LogDebug("FourVectorHLT") << "endRun, run " << run.id();
}