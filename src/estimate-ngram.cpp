////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008, Massachusetts Institute of Technology              //
// All rights reserved.                                                   //
//                                                                        //
// Redistribution and use in source and binary forms, with or without     //
// modification, are permitted provided that the following conditions are //
// met:                                                                   //
//                                                                        //
//     * Redistributions of source code must retain the above copyright   //
//       notice, this list of conditions and the following disclaimer.    //
//                                                                        //
//     * Redistributions in binary form must reproduce the above          //
//       copyright notice, this list of conditions and the following      //
//       disclaimer in the documentation and/or other materials provided  //
//       with the distribution.                                           //
//                                                                        //
//     * Neither the name of the Massachusetts Institute of Technology    //
//       nor the names of its contributors may be used to endorse or      //
//       promote products derived from this software without specific     //
//       prior written permission.                                        //
//                                                                        //
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS    //
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      //
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  //
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT   //
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  //
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT       //
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  //
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY  //
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT    //
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  //
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   //
////////////////////////////////////////////////////////////////////////////



#include <vector>
#include <string>
#include <cstdlib>

#include "util/CommandOptions.h"



#include "util/ZFile.h"



#include "util/Logger.h"
#include "Types.h"
#include "NgramLM.h"


#include "Smoothing.h"
#include "PerplexityOptimizer.h"



#include "WordErrorRateOptimizer.h"


#include "CrossFolder.h"
#include "LiveGuess.h"

#define BUFFERSIZE 4096

using std::vector;
using std::string;



#ifdef F77_DUMMY_MAIN
#  ifdef __cplusplus
extern "C"
#  endif
int F77_DUMMY_MAIN () { return 1; }
#endif


////////////////////////////////////////////////////////////////////////////////

const char *headerDesc =
"Usage: estimate-ngram [Options]\n"
"\n"
"Estimates an n-gram language model by cumulating n-gram count statistics,\n"
"smoothing observed counts, and building a backoff n-gram model.  Parameters\n"
"can be optionally tuned to optimize development set performance.\n"
"\n"
"Filename argument can be an ASCII file, a compressed file (ending in .Z or .gz),\n"
"or '-' to indicate stdin/stdout.\n";

const char *footerDesc =
"---------------------------------------------------------------\n"
"| MIT Language Modeling Toolkit (v0.4)                        |\n"
"| Copyright (C) 2009 Bo-June (Paul) Hsu                       |\n"
"| MIT Computer Science and Artificial Intelligence Laboratory |\n"
"---------------------------------------------------------------\n";

////////////////////////////////////////////////////////////////////////////////

double average(vector<double> & perps) {
  double perp = 0;
  int n = perps.size();
  for (size_t i = 0; i < n; i++) {
    perp += perps[ i ];
  }
  return (perp / n);
}

int evaluatePerplexityWithCrossFolds(int order, int folds, int repeats, CommandOptions & opts) {
  vector<double> perps;  
  vector<double> logs;  
  for (int k = 0 ; k < repeats; k++) {
    Logger::Log(1, "Loading eval set %s...\n", opts["text"]); // [i].c_str());
    CrossFolder cf( opts["text"], folds);
    while ( cf.foldsLeft() ) {
      NgramLM lm(order);
      lm.Initialize(opts["vocab"], AsBoolean(opts["unk"]), 
                    *(cf.trainingSet()), opts["counts"], 
                    opts["smoothing"], opts["weight-features"]);
      Logger::Log(0, "Parameters:\n");
      ParamVector params(lm.defParams());
      
      Logger::Log(0, "Perplexity Evaluations:\n");
      PerplexityOptimizer eval(lm, order);
      Logger::Log(0, "Perplexity load Corpus:\n");
      eval.LoadCorpus( *(cf.testSet()) );
      Logger::Log(0, "Compute Perplexity:\n");
      double perp = eval.ComputePerplexity(params);
      Logger::Log(0, "\t%s\t%.3f\n", cf.getFoldName().c_str(),
                  perp);
      Logger::Log(0, "Compute Cross Entropy:\n");
      double cross = log(perp)/log(2);
      Logger::Log(0, "\t%s\t%.3f\n", cf.getFoldName().c_str(),
                  cross);
      perps.push_back( perp );
      logs.push_back( cross );
      
      cf.nextFold(); /* load the next fold */
    }
  }
  Logger::Log(0, "Perplexity Evaluations:\n");  
  Logger::Log(0, "\t%s\t%.3f\n", opts["text"],
              average(perps));
  Logger::Log(0, "Cross Entropy Evaluations:\n");  
  Logger::Log(0, "\t%s\t%.3f\n", opts["text"],
              average(logs));
  
  return 0;
}



int liveMode(int order,  CommandOptions & opts) {
  vector<double> perps;  
  vector<double> logs;  
  char buffer[BUFFERSIZE];
  Logger::Log(1, "Loading eval set %s...\n", opts["text"]); // [i].c_str());
  NgramLM lm(order);
  lm.Initialize(opts["vocab"], AsBoolean(opts["unk"]), 
                opts["text"], opts["counts"], 
                opts["smoothing"], opts["weight-features"]);
  Logger::Log(0, "Parameters:\n");
  ParamVector params(lm.defParams());

  Logger::Log(0, "Live Guess:\n");
  LiveGuess eval(lm, order);
  
  // issue: how many entries to predict?
  
  while( getline( stdin, buffer, BUFFERSIZE ) ) {    
    std::auto_ptr< std::vector<LiveGuessResult> > results = eval.Predict( buffer , 50 );
    int n = results->size();
    for (size_t i = 0; i < n; i++) {
      LiveGuessResult res = (*results)[ i ];
      Logger::Log(0, "\t%f\t%s\n", res.probability, res.str);
    }    
  }
  
  // get command
  // if command is add corpus
  //  Make an N-Gram model based on the read in text 
  //  extend the original n-gram model (extend) with the new model
  // eval.addToCorpus( str );
  // if command is estimate
  // eval.estimate( str, nestimates);
  // if command is exit
  
  return 0;
}


int main(int argc, char* argv[]) {
    // Parse command line options.
    CommandOptions opts(headerDesc, footerDesc);
    opts.AddOption("h,help", "Print this message.");
    opts.AddOption("verbose", "Set verbosity level.", "1");
    opts.AddOption("o,order", "Set the n-gram order of the estimated LM.", "3");
    opts.AddOption("v,vocab", "Fix the vocab to only words from the specified file.");
    opts.AddOption("u,unk", "Replace all out of vocab words with <unk>.");
    opts.AddOption("t,text", "Add counts from text files.");
    opts.AddOption("c,counts", "Add counts from counts files.");
    opts.AddOption("s,smoothing", "Specify smoothing algorithms.  "
                   "(ML, FixKN, FixModKN, FixKN#, KN, ModKN, KN#)", "ModKN");
    opts.AddOption("wf,weight-features", "Specify n-gram weighting features.");
    opts.AddOption("p,params", "Set initial model params.");
    opts.AddOption("oa,opt-alg", "Specify optimization algorithm.  "
                   "(Powell, LBFGS, LBFGSB)", "Powell");
    opts.AddOption("op,opt-perp", "Tune params to minimize dev set perplexity.");
    opts.AddOption("ow,opt-wer", "Tune params to minimize lattice word error rate.");
    opts.AddOption("om,opt-margin", "Tune params to minimize lattice margin.");
    opts.AddOption("wb,write-binary", "Write LM/counts files in binary format.");
    opts.AddOption("wp,write-params", "Write tuned model params to file.");
    opts.AddOption("wv,write-vocab", "Write LM vocab to file.");
    opts.AddOption("wc,write-counts", "Write n-gram counts to file.");
    opts.AddOption("wec,write-eff-counts", "Write effective n-gram counts to file.");
    opts.AddOption("wlc,write-left-counts", "Write left-branching n-gram counts to file.");
    opts.AddOption("wrc,write-right-counts", "Write right-branching n-gram counts to file.");
    opts.AddOption("wl,write-lm", "Write ARPA backoff LM to file.");
    opts.AddOption("ep,eval-perp", "Compute test set perplexity.");
    opts.AddOption("ew,eval-wer", "Compute test set lattice word error rate.");
    opts.AddOption("em,eval-margin", "Compute test set lattice margin.");
    opts.AddOption("f,fold", "Evaluate Perplexity with N-fold cross validation","0");
    opts.AddOption("rs,seed", "The random seed for srand", "0");
    opts.AddOption("repeat,repeat", "Repeat the calculation", "1");

    if (!opts.ParseArguments(argc, (const char **)argv) ||
        opts["help"] != NULL) {
        std::cout << std::endl;
        opts.PrintHelp();
        return 1;
    }

    // Process basic command line arguments.
    size_t order = atoi(opts["order"]);
    int repeats = atoi(opts["repeat"]);
    if (repeats < 1) {
      Logger::Error(1, "Too few repetitions!\n");
    }
    int     seed  = atoi(opts["seed"]);
    srand( ((seed==0)?time( NULL ):seed) );

    bool writeBinary = AsBoolean(opts["write-binary"]);
    Logger::SetVerbosity(atoi(opts["verbose"]));

    if (!opts["text"] && !opts["counts"]) {
        Logger::Error(1, "Specify training data using -text or -counts.\n");
        exit(1);
    }

    size_t folds = atoi(opts["fold"]);

    if (folds != 0) {
      if (folds < 2) {
        Logger::Error(1, "Too few folds!\n");
      }
      return evaluatePerplexityWithCrossFolds( order, folds, repeats, opts );
    }

    // Build language model.
    NgramLM lm(order);
    lm.Initialize(opts["vocab"], AsBoolean(opts["unk"]), 
                  opts["text"], opts["counts"], 
                  opts["smoothing"], opts["weight-features"]);

    // Estimate LM.
    ParamVector params(lm.defParams());
    if (opts["params"]) {
        Logger::Log(1, "Loading parameters from %s...\n", opts["params"]);
        ZFile f(opts["params"], "r");
        VerifyHeader(f, "Param");
        ReadVector(f, params);
        if (params.length() != lm.defParams().length()) {
            Logger::Error(1, "Number of parameters mismatched.\n");
            exit(1);
        }
    }

    Optimization optAlg = ToOptimization(opts["opt-alg"]);
    if (optAlg == UnknownOptimization) {
        Logger::Error(1, "Unknown optimization algorithms '%s'.\n",
                      opts["opt-alg"]);
        exit(1);
    }
        
    if (opts["opt-perp"]) {
        if (params.length() == 0) {
            Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            Logger::Log(1, "Loading development set %s...\n", opts["opt-perp"]);
            ZFile devZFile(opts["opt-perp"]);
            PerplexityOptimizer dev(lm, order);
            dev.LoadCorpus(devZFile);

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optEntropy = dev.Optimize(params, optAlg);
            Logger::Log(2, " Best perplexity = %f\n", exp(optEntropy));
        }
    }
    if (opts["opt-margin"]) {
        if (params.length() == 0) {
            Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            Logger::Log(1, "Loading development lattices %s...\n", 
                        opts["opt-margin"]);
            ZFile devZFile(opts["opt-margin"]);
            WordErrorRateOptimizer dev(lm, order);
            dev.LoadLattices(devZFile);

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optMargin = dev.OptimizeMargin(params, optAlg);
            Logger::Log(2, " Best margin = %f\n", optMargin);
        }
    }
    if (opts["opt-wer"]) {
        if (params.length() == 0) {
            Logger::Warn(1, "No parameters to optimize.\n");
        } else {
            Logger::Log(1, "Loading development lattices %s...\n", 
                        opts["opt-wer"]);
            ZFile devZFile(opts["opt-wer"]);
            WordErrorRateOptimizer dev(lm, order);
            dev.LoadLattices(devZFile);

            Logger::Log(1, "Optimizing %lu parameters...\n", params.length());
            double optWER = dev.OptimizeWER(params, optAlg);
            Logger::Log(2, " Best WER = %f%%\n", optWER);
        }
    }

    // Estimate full model.
    if (opts["write-lm"] || opts["eval-perp"] || 
        opts["eval-margin"] || opts["eval-wer"]) {
        Logger::Log(1, "Estimating full n-gram model...\n");
        lm.Estimate(params);
    }

    // Save results.
    if (opts["write-params"]) {
        Logger::Log(1, "Saving parameters to %s...\n", opts["write-params"]);
        ZFile f(opts["write-params"], "w");
        WriteHeader(f, "Param");
        WriteVector(f, params);
    }
    if (opts["write-vocab"]) {
        Logger::Log(1, "Saving vocabulary to %s...\n", opts["write-vocab"]);
        ZFile vocabZFile(opts["write-vocab"], "w");
        lm.SaveVocab(vocabZFile);
    }
    if (opts["write-counts"]) {
        Logger::Log(1, "Saving counts to %s...\n", opts["write-counts"]);
        ZFile countZFile(opts["write-counts"], "w");
        lm.SaveCounts(countZFile, writeBinary);
    }
    if (opts["write-eff-counts"]) {
        Logger::Log(1, "Saving effective counts to %s...\n", 
                    opts["write-eff-counts"]);
        ZFile countZFile(opts["write-eff-counts"], "w");
        lm.SaveEffCounts(countZFile, writeBinary);
    }
    if (opts["write-left-counts"]) {
        Logger::Log(1, "Saving left-branching counts to %s...\n", 
                    opts["write-left-counts"]);
        ZFile countZFile(opts["write-left-counts"], "w");

        vector<CountVector> countVectors(order + 1);
        for (size_t o = 0; o < order; ++o) {
            countVectors[o].reset(lm.sizes(o), 0);
            BinCount(lm.backoffs(o+1), countVectors[o]);
        }
        lm.model().SaveCounts(countVectors, countZFile, true);
    }
    if (opts["write-right-counts"]) {
        Logger::Log(1, "Saving right-branching counts to %s...\n", 
                    opts["write-right-counts"]);
        ZFile countZFile(opts["write-right-counts"], "w");

        vector<CountVector> countVectors(order + 1);
        for (size_t o = 0; o < order; ++o) {
            countVectors[o].reset(lm.sizes(o), 0);
            BinCount(lm.hists(o+1), countVectors[o]);
        }
        lm.model().SaveCounts(countVectors, countZFile, true);
    }
    if (opts["write-lm"]) {
        Logger::Log(1, "Saving LM to %s...\n", opts["write-lm"]);
        ZFile lmZFile(opts["write-lm"], "w");
        lm.SaveLM(lmZFile, writeBinary);
    }

    // Evaluate LM.
    if (opts["eval-perp"]) {
        Logger::Log(0, "Perplexity Evaluations:\n");
        vector<string> evalFiles;
        trim_split(evalFiles, opts["eval-perp"], ',');
        for (size_t i = 0; i < evalFiles.size(); i++) {
            Logger::Log(1, "Loading eval set %s...\n", evalFiles[i].c_str());
            ZFile evalZFile(evalFiles[i].c_str());
            PerplexityOptimizer eval(lm, order);
            eval.LoadCorpus(evalZFile);

            Logger::Log(0, "\t%s\t%.3f\n", evalFiles[i].c_str(),
                        eval.ComputePerplexity(params));
        }
    }
    if (opts["eval-margin"]) {
        Logger::Log(0, "Margin Evaluations:\n");
        vector<string> evalFiles;
        trim_split(evalFiles, opts["eval-margin"], ',');
        for (size_t i = 0; i < evalFiles.size(); i++) {
            Logger::Log(1, "Loading eval lattices %s...\n", 
                        evalFiles[i].c_str());
            ZFile evalZFile(evalFiles[i].c_str());
            WordErrorRateOptimizer eval(lm, order);
            eval.LoadLattices(evalZFile);

            Logger::Log(0, "\t%s\t%.3f\n", evalFiles[i].c_str(),
                        eval.ComputeMargin(params));
        }
    }
    if (opts["eval-wer"]) {
        Logger::Log(0, "WER Evaluations:\n");
        vector<string> evalFiles;
        trim_split(evalFiles, opts["eval-wer"], ',');
        for (size_t i = 0; i < evalFiles.size(); i++) {
            Logger::Log(1, "Loading eval lattices %s...\n", 
                        evalFiles[i].c_str());
            ZFile evalZFile(evalFiles[i].c_str());
            WordErrorRateOptimizer eval(lm, order);
            eval.LoadLattices(evalZFile);

            Logger::Log(0, "\t%s\t%.2f%%\n", evalFiles[i].c_str(),
                        eval.ComputeWER(params));
        }
    }

    return 0;
}

