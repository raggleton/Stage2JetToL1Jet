#Stage2JetToL1Jet

This is a small plugin that converts jets output by the Stage 2 emulator into jets suitable for use in L1ExtraTreeProducer.

Note that it uses the hardware values for et, eta, phi, and thus assumes standard trigger tower granularity (as in Run 1). In addition it require the user to select the jet LSB, assuming a linear et scale. 

##Installation

Inside a CMSSW release, in `$CMSSW_BASE/src`:

```
git clone L1Trigger/Stage2JetToL1Jet
scram b -j9
```

##Usage

Add to your config file:

```
process.stage2JetToL1Jets = cms.EDProducer('Stage2JetToL1Jet',
    stage2JetSource = cms.InputTag("caloStage2Digis:MP"),
    jetLsb = cms.double(0.5)
)
```

then add `process.stage2JetToL1Jets` to your `process` path after `process.caloStage2Digis`.

Note that this uses the jets *before* demuxing - is this what you want?
