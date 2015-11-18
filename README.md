#Stage2JetToL1Jet

This is a small plugin that converts jets output by the Stage 2 emulator into jets suitable for use in `L1UpgradeTreeProducer` or `L1ExtraTreeProducer`.

Note that it uses the hardware values for et, eta, phi, and thus assumes **LEGACY** trigger tower granularity (as in Run 1). In addition it require the user to select the jet LSB, assuming a linear et scale.

**TODO**:

- switch between legacy and upgrade hardware

- rename that `gtJets` option.

##Installation

Inside a CMSSW release, in `$CMSSW_BASE/src`:

```
git clone https://github.com/raggleton/Stage2JetToL1Jet.git L1Trigger/Stage2JetToL1Jet
scram b -j9
```

##Usage

Add to your config file:

```
process.stage2JetToL1Jet = cms.EDProducer('Stage2JetToL1Jet',
    stage2JetSource = cms.InputTag('caloStage2Digis:MP'),
    jetLsb = cms.double(0.5),
    gtJets = cms.bool(False)  # True if using jets from GlobalTrigger i.e. after demux
)
```

then add `process.stage2JetToL1Jet` to your `process` path after `process.caloStage2Digis`.

You can then use the output in `L1UpgradeTreeProducer`:

```
process.l1UpgradeTreeProducer.jetLabel = cms.untracked.InputTag('stage2JetToL1Jet')
```

or if you are using the `L1ExtraTreeProducer`, you can also use it:

```
process.l1ExtraTreeProducer.cenJetLabel = cms.untracked.InputTag('stage2JetToL1Jet:central')
process.l1ExtraTreeProducer.fwdJetLabel = cms.untracked.InputTag('stage2JetToL1Jet:forward')
```
