import FWCore.ParameterSet.Config as cms

# Bit Plotting
hltMonMuBits = cms.EDAnalyzer("HLTMonBitSummary",
     #directory = cms.untracked.string('HLT/HLTMonMuon/Summary/'),
     #label = cms.string('myLabel'),
     #out = cms.untracked.string('dqm.root'),
     HLTPaths = cms.vstring('HLT_L1MuOpen','HLT_L1Mu', 'HLT_L1Mu20',
			    'HLT_L2Mu9','HLT_L2Mu11',
                            'HLT_Mu+',
                            'HLT_IsoMu3',
                            'HLT_DoubleMu0','HLT_DoubleMu3'
                            ),
     filterTypes = cms.vstring( "HLTLevel1GTSeed",
                                "HLTPrescaler",
                                "HLTMuonL1Filter",
                                "HLTMuonL2PreFilter",
                                "HLTMuonDimuonL2Filter",
                                "HLTMuonL3PreFilter",
                                "HLTMuonDimuonL3Filter",
                                "HLTMuonIsoFilter"
                               ),
    denominator = cms.untracked.string('HLT_L1MuOpen'),
    
    TriggerResultsTag = cms.InputTag('TriggerResults','','HLT'),

)