import FWCore.ParameterSet.Config as cms
import FWCore.ParameterSet.VarParsing as VarParsing

import os,sys

options = VarParsing.VarParsing('standard') # avoid the options: maxEvents, files, secondaryFiles, output, secondaryOutput because they are already defined in 'standard'
#Change the data folder appropriately to where you wish to access the files from:
options.register('dataFolder',
                 '/eos/cms/store/user/bora/HGCAL/BeamTestAnalysis_2018/RawHit/disconnected_ch_study',
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'folder containing ski2cms collection input')

options.register('pedestalFolder',
                 'HGCAL/pedestals',
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'folder containing pedestal files')

options.register('runNumber',
                 722,
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.int,
                 'Input run to process')

options.register('outputFolder',
                 './',
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'Output folder where analysis output are stored')

options.register('electronicMap',
                 "HGCal/CondObjects/data/emap_full_October2018_setup2_v0_promptReco.txt",
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'path to the electronic map')

options.register('hgcalLayout',
                 'HGCal/CondObjects/data/layer_geom_full_October2018_setup2_v0_promptReco.txt',
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.string,
                 'path to hgcal layout file')

options.maxEvents = -1
options.output = "cmsswEvents.root"

options.parseArguments()
print options
if not os.path.isdir(options.dataFolder):
    sys.exit("Error: Data folder not found or inaccessible!")


pedestalHighGain=options.pedestalFolder+"/pedestalsHG_"+str(options.runNumber)+".txt"
pedestalLowGain=options.pedestalFolder+"/pedestalsLG_"+str(options.runNumber)+".txt"
noisyChannels=options.pedestalFolder+"noisyChannels_"+str(options.runNumber)+".txt"

################################
process = cms.Process("correlationAnalysis")
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(options.maxEvents)
)

####################################

process.source = cms.Source("PoolSource",
                            fileNames=cms.untracked.vstring("file:%s/run000%d.root"%(options.dataFolder,options.runNumber))
)

filename = options.outputFolder+"/HexaOutput_"+str(options.runNumber)+".root"
process.TFileService = cms.Service("TFileService",
                                   fileName = cms.string(filename))

process.output = cms.OutputModule("PoolOutputModule",
                                  fileName = cms.untracked.string(options.output),
                                  outputCommands = cms.untracked.vstring('drop *')
                                  )

process.correlationplotter = cms.EDAnalyzer("CorrelationPlotter",
                                         InputCollection=cms.InputTag("rawhitproducer","HGCALTBRAWHITS"),
                                         ElectronicMap=cms.untracked.string(options.electronicMap),
                                         HighGainPedestalFileName=cms.untracked.string(pedestalHighGain),
                                         LowGainPedestalFileName=cms.untracked.string(pedestalLowGain),
)

process.p = cms.Path( process.correlationplotter )

process.end = cms.EndPath(process.output)

