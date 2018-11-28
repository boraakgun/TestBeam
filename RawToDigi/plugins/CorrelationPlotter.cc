#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <cmath>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "HGCal/DataFormats/interface/HGCalTBRawHitCollection.h"
#include "HGCal/DataFormats/interface/HGCalTBDetId.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "HGCal/CondObjects/interface/HGCalElectronicsMap.h"
#include "HGCal/CondObjects/interface/HGCalCondObjectTextIO.h"
#include "HGCal/DataFormats/interface/HGCalTBElectronicsId.h"
#include "HGCal/CondObjects/interface/HGCalTBDetectorLayout.h"
#include "HGCal/DataFormats/interface/HGCalTBLayer.h"
#include "HGCal/DataFormats/interface/HGCalTBModule.h"

#include "TH2F.h"

class CorrelationPlotter : public edm::one::EDAnalyzer<edm::one::SharedResources>
{
public:
  explicit CorrelationPlotter(const edm::ParameterSet&);
  ~CorrelationPlotter();
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
private:
  virtual void beginJob() override;
  void analyze(const edm::Event& , const edm::EventSetup&) override;
  virtual void endJob() override;
  void createCorrelationHistograms(int module, int board);
  
  std::string m_electronicMap;
  std::string m_detectorLayoutFile;
  HGCalElectronicsMap m_emap;
  HGCalTBDetectorLayout m_layout;

  int m_evtID;
  uint16_t m_numberOfBoards;

  edm::EDGetTokenT<HGCalTBRawHitCollection> m_HGCalTBRawHitCollection;

  struct hgcal_channel{
    hgcal_channel() : key(0),
		      module(0),
		      meanHG(0.),
		      meanLG(0.),
		      rmsHG(0.),
		      rmsLG(0.){;}
    int key;
    int module;
    float meanHG;
    float meanLG;
    float rmsHG;
    float rmsLG;
    std::vector<float> highGain;
    std::vector<float> lowGain;
  };
  std::map<int,hgcal_channel> m_channelMap;

  //histo maps filled and used in endJob() method
  std::map<int,TH2F*> hgCorrMap;
  std::map<int,TH2F*> lgCorrMap;
  std::map<int,TH2F*> hgCorrChipMap;
  std::map<int,TH2F*> lgCorrChipMap;
  
};

CorrelationPlotter::CorrelationPlotter(const edm::ParameterSet& iConfig) :
  m_electronicMap(iConfig.getUntrackedParameter<std::string>("ElectronicMap","HGCal/CondObjects/data/emap.txt")),
  m_detectorLayoutFile(iConfig.getUntrackedParameter<std::string>("DetectorLayout","HGCal/CondObjects/data/layer_geom_full_October2018_setup2_v0_promptReco.txt"))
{
  m_HGCalTBRawHitCollection = consumes<HGCalTBRawHitCollection>(iConfig.getParameter<edm::InputTag>("InputCollection"));
  m_evtID=0;
  std::cout << iConfig.dump() << std::endl;
}


CorrelationPlotter::~CorrelationPlotter()
{

}

void CorrelationPlotter::beginJob()
{
  HGCalCondObjectTextIO io(0);
  edm::FileInPath fip(m_electronicMap);
  if (!io.load(fip.fullPath(), m_emap)) {
    throw cms::Exception("Unable to load electronics map");
  };
  fip=edm::FileInPath(m_detectorLayoutFile);
  if (!io.load(fip.fullPath(), m_layout)) {
    throw cms::Exception("Unable to load detector layout file");
  };
  for( auto layer : m_layout.layers() ){
    layer.print();
  }
}

void CorrelationPlotter::analyze(const edm::Event& event, const edm::EventSetup& setup)
{
  edm::Handle<HGCalTBRawHitCollection> hits;
  event.getByToken(m_HGCalTBRawHitCollection, hits);
  
  m_evtID++;
  if( !hits->size() ) return;
  
  for( auto hit : *hits ){
    HGCalTBElectronicsId eid( m_emap.detId2eid(hit.detid().rawId()) );
    //////if( !m_emap.existsEId(eid) ) continue;
    
    int iskiroc=hit.skiroc();//from 0 to numberOfHexaboard*4-1
    int iboard=iskiroc/HGCAL_TB_GEOMETRY::N_SKIROC_PER_HEXA;
    int ichip=iskiroc%HGCAL_TB_GEOMETRY::N_SKIROC_PER_HEXA;//from 0 to 3
    uint32_t key=iboard*1000+ichip*100+hit.channel();
    std::map<int,hgcal_channel>::iterator iter=m_channelMap.find(key);
    if( iter==m_channelMap.end() ){
      hgcal_channel tmp;
      tmp.key=key;
      
      //std::cout << " hit.detid().layer() = " << hit.detid().layer() << std::endl;
      ///HGCalTBLayer alayer = m_layout.at( hit.detid().layer()-1 );
      ///HGCalTBModule amodule = alayer.at( hit.detid().sensorIU(), hit.detid().sensorIV() );
      ///tmp.module=amodule.moduleID();
      
      /*if ( iboard == 0 && ichip ==0  ){
	std::cout << " hit.channel() = " << hit.channel() <<  " hit.highGainADC(0) =  " << hit.highGainADC(0) << " hit.lowGainADC(0) = " << hit.lowGainADC(0) << std::endl;
	}*/
      
      std::vector<float> vecH,vecL;
      vecH.push_back(hit.highGainADC(0));
      vecL.push_back(hit.lowGainADC(0));
      tmp.highGain=vecH;
      tmp.lowGain=vecL;
      std::pair<int,hgcal_channel> p(key,tmp);
      m_channelMap.insert( p );
    }
    else{
      iter->second.highGain.push_back(hit.highGainADC(0));
      iter->second.lowGain.push_back(hit.lowGainADC(0));
    }
  }
}

void CorrelationPlotter::endJob()
{
  for( std::map<int,hgcal_channel>::iterator it=m_channelMap.begin(); it!=m_channelMap.end(); ++it ){
    it->second.meanHG = 0.;
    it->second.meanLG = 0.;
    it->second.rmsHG = 0.;
    it->second.rmsLG = 0.;
    unsigned int size = it->second.highGain.size();
    for( unsigned int ihit=0; ihit<size; ihit++ ){
      it->second.meanHG+=it->second.highGain.at(ihit);
      it->second.meanLG+=it->second.lowGain.at(ihit);
      it->second.rmsHG+=it->second.highGain.at(ihit)*it->second.highGain.at(ihit);
      it->second.rmsLG+=it->second.lowGain.at(ihit)*it->second.lowGain.at(ihit);
    }
    it->second.meanHG/=size;
    it->second.meanLG/=size;
    it->second.rmsHG=std::sqrt( it->second.rmsHG/size-it->second.meanHG*it->second.meanHG );
    it->second.rmsLG=std::sqrt( it->second.rmsLG/size-it->second.meanLG*it->second.meanLG );
  }  

  std::map<int,hgcal_channel>::iterator it=m_channelMap.begin();
  while( it!=m_channelMap.end() ){
    int iboard=it->first/1000;
    int iski=(it->first%1000)/100;
    int ichan=it->first%100;
    int module=it->second.module;
    //if ( iboard == 0 && iski ==0  ) {std::cout << " ichan = " << ichan << " it->second.meanHG =  " << it->second.meanHG << " it->second.meanLG =  " << it->second.meanLG << " it->second.rmsHG = " << it->second.rmsHG << " it->second.rmsLG = " << it->second.rmsLG << std::endl; }
    if( hgCorrMap.find(iboard)==hgCorrMap.end() )
      createCorrelationHistograms(module, iboard);
    int binx = ichan + iski*HGCAL_TB_GEOMETRY::N_CHANNELS_PER_SKIROC;
    for( std::map<int,hgcal_channel>::iterator jt=++m_channelMap.begin(); jt!=m_channelMap.end(); ++jt ){
      if( jt->first/1000 != iboard ) continue; //ie not the same board
      float meanhghg(0.),meanlglg(0.);
      for( unsigned int ievt=0; ievt<it->second.highGain.size(); ievt++ ){
	meanhghg+=it->second.highGain.at(ievt)*jt->second.highGain.at(ievt);
	meanlglg+=it->second.lowGain.at(ievt)*jt->second.lowGain.at(ievt);
      }
      meanhghg/=it->second.highGain.size();
      meanlglg/=it->second.lowGain.size();
      float corrHGHG = it->second.rmsHG>0&&jt->second.rmsHG>0 ? (meanhghg-it->second.meanHG*jt->second.meanHG)/(it->second.rmsHG*jt->second.rmsHG) : 0;
      float corrLGLG = it->second.rmsLG>0&&jt->second.rmsLG>0 ? (meanlglg-it->second.meanLG*jt->second.meanLG)/(it->second.rmsLG*jt->second.rmsLG) : 0;
      
      int jski=(jt->first%1000)/100;
      int jchan=jt->first%100;
      int biny = jchan + jski*HGCAL_TB_GEOMETRY::N_CHANNELS_PER_SKIROC;
      //if ( iboard == 0 && iski == 0 && jski == 0  ) {std::cout << " ichan = " << ichan << " jchan = " << jchan << " corrHGHG = " << corrHGHG << " corrLGLG = " << corrLGLG << " binx = "<< binx << " biny = "<< biny <<  std::endl; }
      
      hgCorrMap[ iboard ]->Fill( binx,biny,corrHGHG );
      hgCorrMap[ iboard ]->Fill( biny,binx,corrHGHG );
      lgCorrMap[ iboard ]->Fill( binx,biny,corrLGLG );
      lgCorrMap[ iboard ]->Fill( biny,binx,corrLGLG );

      if(iski!=jski) continue;
      int binxNew=ichan;
      int binyNew=jchan;
      
      hgCorrChipMap[ iboard*10+iski ]->Fill( binxNew,binyNew,corrHGHG );
      hgCorrChipMap[ iboard*10+iski ]->Fill( binyNew,binxNew,corrHGHG );
      lgCorrChipMap[ iboard*10+iski ]->Fill( binxNew,binyNew,corrLGLG );
      lgCorrChipMap[ iboard*10+iski ]->Fill( binyNew,binxNew,corrLGLG );
    }
    m_channelMap.erase(it);
    it=m_channelMap.begin();
  }
}

void CorrelationPlotter::createCorrelationHistograms(int module, int board)
{
  usesResource("TFileService");
  edm::Service<TFileService> fs;
  std::ostringstream os( std::ostringstream::ate );
  TH2F *h;
  os.str("");
  os << "Board" << board << "_M" << module;
  TFileDirectory dir = fs->mkdir( os.str().c_str() );
  os.str("");
  os<<"HighGain";
  //int nbin=HGCAL_TB_GEOMETRY::N_SKIROC_PER_HEXA*HGCAL_TB_GEOMETRY::N_CHANNELS_PER_SKIROC/2;
  int nbin=HGCAL_TB_GEOMETRY::N_SKIROC_PER_HEXA*HGCAL_TB_GEOMETRY::N_CHANNELS_PER_SKIROC;
  //h=dir.make<TH2F>(os.str().c_str(),os.str().c_str(),nbin,0,nbin*2,nbin,0,nbin*2);
  h=dir.make<TH2F>(os.str().c_str(),os.str().c_str(),nbin,0,nbin,nbin,0,nbin);
  h->SetName(os.str().c_str());
  h->SetTitle(os.str().c_str());
  h->SetOption("colz");
  h->SetMinimum(-1);
  h->SetMaximum(1);
  hgCorrMap.insert( std::pair<int,TH2F*>(board,h) );
  os.str("");
  os<<"LowGain";
  //h=dir.make<TH2F>(os.str().c_str(),os.str().c_str(),nbin,0,nbin*2,nbin,0,nbin*2);
  h=dir.make<TH2F>(os.str().c_str(),os.str().c_str(),nbin,0,nbin,nbin,0,nbin);
  h->SetName(os.str().c_str());
  h->SetTitle(os.str().c_str());
  h->SetOption("colz");
  h->SetMinimum(-1);
  h->SetMaximum(1);
  lgCorrMap.insert( std::pair<int,TH2F*>(board,h) );
  //nbin=HGCAL_TB_GEOMETRY::N_CHANNELS_PER_SKIROC/2;
  nbin=HGCAL_TB_GEOMETRY::N_CHANNELS_PER_SKIROC;
  for( int iski=0; iski<HGCAL_TB_GEOMETRY::N_SKIROC_PER_HEXA; iski++ ){
    os.str("");
    os<<"HighGain_CHIP"<<iski;
    //h=dir.make<TH2F>(os.str().c_str(),os.str().c_str(),nbin,0,nbin*2,nbin,0,nbin*2);
    h=dir.make<TH2F>(os.str().c_str(),os.str().c_str(),nbin,0,nbin,nbin,0,nbin);
    h->SetName(os.str().c_str());
    h->SetTitle(os.str().c_str());
    h->SetOption("colz");
    h->SetMinimum(-1);
    h->SetMaximum(1);
    hgCorrChipMap.insert( std::pair<int,TH2F*>(board*10+iski,h) );
    os.str("");
    os<<"LowGain_CHIP"<<iski;
    //h=dir.make<TH2F>(os.str().c_str(),os.str().c_str(),nbin,0,nbin*2,nbin,0,nbin*2);
    h=dir.make<TH2F>(os.str().c_str(),os.str().c_str(),nbin,0,nbin,nbin,0,nbin); 
    h->SetName(os.str().c_str());
    h->SetTitle(os.str().c_str());
    h->SetOption("colz");
    h->SetMinimum(-1);
    h->SetMaximum(1);
    lgCorrChipMap.insert( std::pair<int,TH2F*>(board*10+iski,h) );
  }
}

void CorrelationPlotter::fillDescriptions(edm::ConfigurationDescriptions& descriptions)
{
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

DEFINE_FWK_MODULE(CorrelationPlotter);
