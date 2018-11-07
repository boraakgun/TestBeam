import FWCore.ParameterSet.Config as cms
import FWCore.ParameterSet.VarParsing as VarParsing

import os,sys

options = VarParsing.VarParsing('standard') # avoid the options: maxEvents, files, secondaryFiles, output, secondaryOutput because they are already defined in 'standard'
#Change the data folder appropriately to where you wish to access the files from:
options.register('dataFolder',
                 #'/eos/cms/store/group/dpg_hgcal/tb_hgcal/2018/cern_h2_june/HGCalTBSkiroc2CMS',
                 #'/afs/cern.ch/user/b/bora/CMSSW_8_0_1/src/HGCal/Skiroc2CMS',
                 '/eos/cms/store/user/bora/HGCAL/BeamTestAnalysis_2018/Skiroc2CMS',
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'folder containing raw input')

options.register('runNumber',
                 179,
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.int,
                 'Input run to process')

options.register('pedestalFolder',
                 #'/eos/cms/store/group/dpg_hgcal/tb_hgcal/2018/cern_h2_june/pedestals',
                 #'/afs/cern.ch/user/b/bora/CMSSW_8_0_1/src/HGCal/Skiroc2CMS',
                 '/eos/cms/store/user/bora/HGCAL/BeamTestAnalysis_2018/Skiroc2CMS',
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'folder containing raw input')

options.register('edmOutputFolder',
                 #'/afs/cern.ch/user/b/bora/CMSSW_8_0_1/src/HGCal/RawHit',
                 '/eos/cms/store/user/bora/HGCAL/BeamTestAnalysis_2018/RawHit/',
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'Output folder where edm output are stored')

options.register('rawhitOutputFolder',
                 #'/afs/cern.ch/user/b/bora/CMSSW_8_0_1/src/HGCal/RawHit',
                 '/eos/cms/store/user/bora/HGCAL/BeamTestAnalysis_2018/RawHit',
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'Output folder where analysis output are stored')

options.register('electronicMap',
                 "HGCal/CondObjects/data/emap_full_October2018_setup3_v1_promptReco.txt",
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'path to the electronic map')

options.register('hgcalLayout',
                 'HGCal/CondObjects/data/layer_geom_full_October2018_setup3_v1_promptReco.txt',
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'path to hgcal layout file')

options.maxEvents = -1
options.output = "cmsswEvents.root"

options.parseArguments()
if not os.path.isdir(options.dataFolder):
    sys.exit("Error: Data folder not found or inaccessible!")

options.output = options.edmOutputFolder + "run%06d.root"%(options.runNumber)

print options


pedestalHighGain=options.pedestalFolder+"/pedestalHG_"+str(options.runNumber)+".txt"
pedestalLowGain=options.pedestalFolder+"/pedestalLG_"+str(options.runNumber)+".txt"
noisyChannels=options.pedestalFolder+"/noisyChannels_"+str(options.runNumber)+".txt"

print pedestalHighGain
print pedestalLowGain

################################
process = cms.Process("rawhitprod")
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(options.maxEvents)
)

####################################

process.source = cms.Source("PoolSource",
                            fileNames=cms.untracked.vstring("file:%s/run%06d.root"%(options.dataFolder,options.runNumber))
)

filename = options.rawhitOutputFolder+"/HexaOutput_"+str(options.runNumber)+".root"
process.TFileService = cms.Service("TFileService",
                                   fileName = cms.string(filename)
)
process.output = cms.OutputModule("PoolOutputModule",
                                  fileName = cms.untracked.string(options.output),
                                  outputCommands = cms.untracked.vstring('drop *',
                                                                         'keep *_*_HGCALTBRAWHITS_*')
)

process.rawhitproducer = cms.EDProducer("HGCalTBRawHitProducer",
                                        InputCollection=cms.InputTag("source","skiroc2cmsdata"),
                                        OutputCollectionName=cms.string("HGCALTBRAWHITS"),
                                        ElectronicMap=cms.untracked.string(options.electronicMap),
                                        SubtractPedestal=cms.untracked.bool(True),
                                        MaskNoisyChannels=cms.untracked.bool(True),
                                        HighGainPedestalFileName=cms.untracked.string(pedestalHighGain),
                                        LowGainPedestalFileName=cms.untracked.string(pedestalLowGain),
                                        ChannelsToMaskFileName=cms.untracked.string(noisyChannels)
)

process.rawhitplotter = cms.EDAnalyzer("RawHitPlotter",
                                       InputCollection=cms.InputTag("rawhitproducer","HGCALTBRAWHITS"),
                                       ElectronicMap=cms.untracked.string(options.electronicMap),
                                       DetectorLayout=cms.untracked.string(options.hgcalLayout),
                                       SensorSize=cms.untracked.int32(128),
                                       EventPlotter=cms.untracked.bool(False),
                                       SubtractCommonMode=cms.untracked.bool(True)
)

process.pulseshapeplotter = cms.EDAnalyzer("PulseShapePlotter",
                                           InputCollection=cms.InputTag("rawhitproducer","HGCALTBRAWHITS"),
                                           ElectronicMap=cms.untracked.string(options.electronicMap)
)


process.p = cms.Path( process.rawhitproducer*process.rawhitplotter )

process.end = cms.EndPath(process.output)
