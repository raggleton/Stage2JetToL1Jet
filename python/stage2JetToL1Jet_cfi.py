import FWCore.ParameterSet.Config as cms

stage2JetToL1Jet = cms.EDProducer('Stage2JetToL1Jet',
    stage2JetSource = cms.InputTag('caloStage2Digis:MP'),
    jetLsb = cms.double(0.5)
)
