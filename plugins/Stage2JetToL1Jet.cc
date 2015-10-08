// -*- C++ -*-
//
// Package:    L1Trigger/Stage2JetToL1Jet
// Class:      Stage2JetToL1Jet
// 
/**\class Stage2JetToL1Jet Stage2JetToL1Jet.cc L1Trigger/Stage2JetToL1Jet/plugins/Stage2JetToL1Jet.cc

 Description: EDProducer to convert HW values in Stage 2 jets to physical values.
 It also converts the Stage 2 Jets into L1JetParticles.
 Output can then be passed into l1UpgradeTreeProducer, or l1ExtraTreeProducer.

 Implementation:
     This is specific to Stage 2, since eta and phi have trigger tower granularity,
     unlike Legacy/Stage 1, which only have region granularity.
*/
//
// Original Author:  Robin Aggleton
//         Created:  Tue, 06 Oct 2015 15:36:08 GMT
//
//


// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include "DataFormats/L1Trigger/interface/L1JetParticle.h"
#include "DataFormats/L1Trigger/interface/L1JetParticleFwd.h"
#include "DataFormats/L1Trigger/interface/Jet.h"

//
// class declaration
//

class Stage2JetToL1Jet : public edm::EDProducer {
  public:
    explicit Stage2JetToL1Jet(const edm::ParameterSet&);
    ~Stage2JetToL1Jet();

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

  private:
    virtual void beginJob() override;
    virtual void produce(edm::Event&, const edm::EventSetup&) override;
    virtual void endJob() override;

    //virtual void beginRun(edm::Run const&, edm::EventSetup const&) override;
    //virtual void endRun(edm::Run const&, edm::EventSetup const&) override;
    //virtual void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;
    //virtual void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;

    virtual math::PtEtaPhiMLorentzVector hwToLorentzVector(int hwPt, int hwEta, int hwPhi, double jetLsb);
    virtual double trigTowerToEta(int hwEta);
    virtual double trigTowerToPhi(int hwPhi, bool forward);

    // ----------member data ---------------------------
    const edm::InputTag stage2JetSource_;
    edm::EDGetTokenT<l1t::JetBxCollection> stage2JetToken_;
    const double jetLsb_;
};

//
// constants, enums and typedefs
//


//
// static data member definitions
//

//
// constructors and destructor
//
Stage2JetToL1Jet::Stage2JetToL1Jet(const edm::ParameterSet& iConfig):
  stage2JetSource_(iConfig.getParameter<edm::InputTag>("stage2JetSource")),
  jetLsb_(iConfig.getParameter<double>("jetLsb"))
{
  using namespace l1extra;

  //register your products
  produces<L1JetParticleCollection>("Central");
  produces<L1JetParticleCollection>("Forward");
  produces<l1t::JetBxCollection>();

  //now do what ever other initialization is needed
  stage2JetToken_ = consumes<l1t::JetBxCollection>(stage2JetSource_);
  
}


Stage2JetToL1Jet::~Stage2JetToL1Jet()
{
 
  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called to produce the data  ------------
void
Stage2JetToL1Jet::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace edm;
  using namespace l1extra;
  using namespace std;

  // Get the JetBxCollection
  Handle<l1t::JetBxCollection> stage2JetCollection;
  iEvent.getByToken(stage2JetToken_, stage2JetCollection);
  if (!stage2JetCollection.isValid()) {
    throw cms::Exception("ProductNotValid") << "stage2JetSource not valid";
  }

  auto_ptr<L1JetParticleCollection> cenJetColl(new L1JetParticleCollection);
  auto_ptr<L1JetParticleCollection> fwdJetColl(new L1JetParticleCollection);
  auto_ptr<l1t::JetBxCollection> newJetColl(new l1t::JetBxCollection);
  // Loop over BX
  int firstBX = stage2JetCollection->getFirstBX();
  int lastBX = stage2JetCollection->getLastBX();
  newJetColl->setBXRange(firstBX, lastBX);

  for (int itBX = firstBX; itBX!=lastBX+1; ++itBX) {
    // Loop over each obj, make a l1extra::L1JetParticle obj from it
    l1t::JetBxCollection::const_iterator jetItr = stage2JetCollection->begin(itBX);
    l1t::JetBxCollection::const_iterator jetEnd = stage2JetCollection->end(itBX);
    for( ; jetItr != jetEnd ; ++jetItr) {
      // get pt, eta, phi from HW values, convert to 4 vec.
      math::PtEtaPhiMLorentzVector p4 = hwToLorentzVector(jetItr->hwPt(), jetItr->hwEta(), jetItr->hwPhi(), jetLsb_);
      l1t::Jet jet(p4, jetItr->hwPt(), jetItr->hwEta(), jetItr->hwPhi(), jetItr->hwQual());
      newJetColl->push_back(itBX, jet);

      if (abs(p4.eta()) < 3) {
        cenJetColl->push_back(L1JetParticle(p4, L1JetParticle::JetType::kCentral, itBX));
      } else {
        fwdJetColl->push_back(L1JetParticle(p4, L1JetParticle::JetType::kForward, itBX));
      }
    }
  }

  OrphanHandle< L1JetParticleCollection > cenJetHandle = iEvent.put(cenJetColl, "Central");
  OrphanHandle< L1JetParticleCollection > fwdJetHandle = iEvent.put(fwdJetColl, "Forward");
  OrphanHandle< l1t::JetBxCollection > newJetHandle = iEvent.put(newJetColl);
}


/**
 * @brief Convert trigger tower (TT) number (aka iEta) to physical eta
 * @details Returns physical eta corresponding to centre of each trigger tower.
 * So tower 1 => 0.087 / 2 = 0.0435.
 *
 * @param hwEta Hardware eta (iEta)
 * @return Physical eta, at the centre of each trigger tower.
 */
double
Stage2JetToL1Jet::trigTowerToEta(int hwEta)
{
  int absHwEta = abs(hwEta);

  if (absHwEta > 32) {
    throw cms::Exception("Value not valid") << "hwEta out of bounds, abs(hwEta) > 32";
  }

  double eta = 0.;
  if (absHwEta <= 20) {
    // Up to the split HE, TT: 1 - 20, eta: 0 - 1.74. Each TT has dEta = 0.087.
    eta = (absHwEta - .5) * 0.087;
  } else if (absHwEta <= 28) {
    // Split HE, TT: 21 - 28, eta: 1.74 - 3. Towers have non-uniform segmentation.
    double etaEdges [] = {1.74, 1.83, 1.93, 2.043, 2.172, 2.322, 2.5, 2.65, 3};
    eta = 0.5 * (etaEdges[absHwEta-20] + etaEdges[absHwEta-21]);
  } else {
    // HF, TT: 29 - 32, Eta: 3 - 5. Each tower has dEta = 0.5
    eta = ((absHwEta - 29) * 0.5) + 3.25;
  }
  int sign = (hwEta > 0) - (hwEta < 0);
  return eta * sign;
}


/**
 * @brief Convert trigger tower number (iPhi) to physical phi
 * @details Returns physical phi corresponding to centre of trigger tower.
 *
 * @param hwPhi Hardware phi (iPhi)
 * @param forward True if trigger tower in forward region as phi granularity different.
 */
double
Stage2JetToL1Jet::trigTowerToPhi(int hwPhi, bool forward)
{
  if (forward) {
    return (hwPhi - 1) * 2 * M_PI / 72;
  } else {
    return (hwPhi - 0.5) * 2 * M_PI / 72.;
  }
}

/**
 * @brief Convert hardware pt, eta, phi to physical values in a LorentzVector.
 * @details Assumes 72 towers in phi, each equally sized.
 *
 * @param hwPt Hardware pt
 * @param hwEta Hardware eta (iEta)
 * @param hwPhi Hardware phi (iPhi)
 * @param jetLsb LSB for jet et scale, assumes linear et scale starting at 0.
 * @return [description]
 */
math::PtEtaPhiMLorentzVector
Stage2JetToL1Jet::hwToLorentzVector(int hwPt, int hwEta, int hwPhi, double jetLsb)
{
  double pt = hwPt * jetLsb;
  double eta = trigTowerToEta(hwEta);
  bool fwd = abs(eta) > 3;
  double phi = trigTowerToPhi(hwPhi, fwd);
  return math::PtEtaPhiMLorentzVector(pt, eta, phi, 0.);
}

// ------------ method called once each job just before starting event loop  ------------
void 
Stage2JetToL1Jet::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
Stage2JetToL1Jet::endJob() {
}

// ------------ method called when starting to processes a run  ------------
/*
void
Stage2JetToL1Jet::beginRun(edm::Run const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method called when ending the processing of a run  ------------
/*
void
Stage2JetToL1Jet::endRun(edm::Run const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method called when starting to processes a luminosity block  ------------
/*
void
Stage2JetToL1Jet::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method called when ending the processing of a luminosity block  ------------
/*
void
Stage2JetToL1Jet::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
Stage2JetToL1Jet::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("stage2JetSource", edm::InputTag(""))->setComment("Jet collection from Stage 2 emulator");
  desc.add<double>("jetLsb")->setComment("LSB for jet et scale.");
  descriptions.add("Stage2JetToL1Jet", desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(Stage2JetToL1Jet);
