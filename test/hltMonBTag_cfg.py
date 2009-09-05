import FWCore.ParameterSet.Config as cms

process = cms.Process("DQM")
process.load("DQMServices.Core.DQM_cfg")

process.load("DQM.HLTEvF.hltMonBTagIPSource_cfi")
process.hltMonBTagIPSource.storeROOT  = True

process.load("DQM.HLTEvF.hltMonBTagMuSource_cfi")
process.hltMonBTagMuSource.storeROOT = True

process.load("DQM.HLTEvF.hltMonBTagIPClient_cfi")
process.hltMonBTagIPClient.storeROOT = True

process.load("DQM.HLTEvF.hltMonBTagMuClient_cfi")
process.hltMonBTagMuClient.storeROOT = True

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1)
)

# /RelValTTbar/CMSSW_3_3_0_pre1-STARTUP31X_V4-v1/GEN-SIM-DIGI-RAW-HLTDEBUG
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/E02BDBA9-F895-DE11-A1F9-000423D944DC.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/DCD057C2-F995-DE11-BED4-000423D9A2AE.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/D08F3FED-FC95-DE11-9F86-001D09F23A61.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/CE323561-F795-DE11-874B-001D09F2841C.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/CC48A901-F795-DE11-AEFF-003048D2C0F4.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/C4BE345D-F795-DE11-AF23-003048D2BE08.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/C44E12A2-0896-DE11-9AE4-003048D3756A.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/B0802351-F895-DE11-9123-001617C3B6E2.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/A85DFEAA-F895-DE11-A5B7-000423D99AA2.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/9A74F054-FE95-DE11-B84D-001D09F2B30B.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/9A039A85-F995-DE11-B022-003048D2BE12.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/8616D8B3-F795-DE11-A4E4-000423D98B28.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/7AEC6A1F-F895-DE11-89B3-000423D99614.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/5E1B188A-F595-DE11-A97E-001617DC1F70.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/4C5AD325-FE95-DE11-90C4-000423D6C8EE.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/44C68985-F795-DE11-BAD0-001D09F24353.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/2C655F5B-F795-DE11-B3C7-001617C3B6FE.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/22F4297C-F595-DE11-A025-003048D37538.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/20F3EF0F-FE95-DE11-AE5B-000423D985B0.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/1AC02FD2-F895-DE11-8AF8-000423D9989E.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/1AA4582E-FE95-DE11-BEE0-000423D98B08.root',
        '/store/relval/CMSSW_3_3_0_pre1/RelValTTbar/GEN-SIM-DIGI-RAW-HLTDEBUG/STARTUP31X_V4-v1/0012/06DFB5CD-F895-DE11-ABE5-003048D375AA.root'
    )
)

process.dqm = cms.Path( process.hltMonBTagIPSource + process.hltMonBTagMuSource  + process.hltMonBTagIPClient + process.hltMonBTagMuClient )
