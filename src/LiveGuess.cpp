////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2011, Abram Hindle, Prem Devanbu and UC Davis            //
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

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
//#include <boost/pending/fibonacci_heap.hpp>
#include <boost/pending/relaxed_heap.hpp>

#include "util/FastIO.h"
#include "util/ZFile.h"
#include "util/FakeZFile.h"
#include "util/CommandOptions.h"
#include "Types.h"
#include "NgramModel.h"
#include "NgramLM.h"
#include "Smoothing.h"
#include "CrossFolder.h"
#include <cstdlib>
#include "util/Logger.h"

using std::string;
using std::vector;
using std::stringstream;


#include "LiveGuess.h"

typedef std::less<VocabProb> VCompare;
typedef boost::typed_identity_property_map<long unsigned int> VID;





std::auto_ptr< std::vector<LiveGuessResult> > LiveGuess::Predict( char * str, int predictions ) {
  vector<const char *> words(0);
  int len = strlen(str) + 1;
  char strSpace[len];
  strcpy( strSpace, str);
  char * p = strSpace;
  // parse words
  while (*p != '\0') {
    while (isspace(*p)) ++p;  // Skip consecutive spaces.
    const char *token = p;
    while (*p != 0 && !isspace(*p))  ++p;
    size_t len = p - token;
    if (*p != 0) *p++ = 0;
    words.push_back( token );
  }
  Logger::Log(0, "We've got these words:\n");
  for (int i = 0 ; i < words.size(); i ++) {
    Logger::Log(0, "\tWord: %d - %s\n",i, words[i]);
  }
  // words are parsed
  const Vocab & vocab = _lm.vocab();
  for (int i = 0 ; i < words.size() - _order + 1; i++ ) {

    // Now we find the index of the n-gram
    NgramIndex index = 0;
    Prob prob = 0;
    const char * sWord = NULL;
    for (size_t j = 0; j < _order; ++j) {
      const ProbVector & probabilities = _lm.probs( j  );
      const NgramVector & ngrams = _lm.model().vectors( j + 1   );
      sWord = words[i + j];
      VocabIndex vWordI = vocab.Find( sWord );
      index = ngrams.Find(index, vWordI);
      prob = probabilities[ index ];
      //Logger::Log(0, "Word:\t%d\t%s\t%d\t\%d\t%e\n", j, sWord, vWordI, index, prob);
    }
    Logger::Log(0, "Final:\t%d\t%e\t%s\n", index, prob, sWord);
    double probpred[predictions];
    const int size = 10;
    const VCompare cmp();
    // const boost::identity_property_map ID();
    const VID id();
    // const boost::typed_identity_property_map<long unsigned int> ID();
    // boost::fibonacci_heap<VocabProb, std::less<VocabProb>, boost::identity_property_map >::fibonacci_heap(size , cmp, ID );
    // boost::fibonacci_heap<VocabProb, std::less<VocabProb>, boost::typed_identity_property_map<long unsigned int> >::fibonacci_heap heap( size, cmp, id);
    //boost::fibonacci_heap<VocabProb, VCompare>::fibonacci_heap heap( size, VCompare());
    boost::relaxed_heap<VocabProb, VCompare>::relaxed_heap heap( (const long unsigned int)10 );//, VCompare());

    //boost::fibonacci_heap<VocabProb> heap( size, cmp );    
    const NgramVector & ngrams = _lm.model().vectors( _order );
    const ProbVector & probabilities = _lm.probs( _order - 1  );
    int count = 0;
    for (int j = 0; j < vocab.size(); j++) {
      NgramIndex newIndex = ngrams.Find( index, j);
      Prob prob = probabilities[ newIndex ];
      const VocabProb v(prob,j);
      if ( count < size ) {
        heap.push( v );
        count++;
      } else if ( heap.top() < v ) {
        heap.pop(); // remove top
        heap.push( v );
        // should we update?
      }
    }
    // VocabProb topN[size];    
    for( int j = 0; !heap.empty() && j < size; j++ ) {
      VocabProb v = heap.top();
      heap.pop();
      Logger::Log(0, "%d\t%e\t%s\n", j, v.prob, vocab[ v.index ]);
    }

  }
  std::auto_ptr< std::vector<LiveGuessResult> > returnValue( new std::vector<LiveGuessResult>() );
  return returnValue;
  ///* stolen from Perplexity */
  //{
  //  // now make a fake file
  //  std::vector< char * > zfileContents();
  //  zfileContents.push_back( str );
  //  FakeZFile corpusFile( zfileContents );
  //
  //  /* it's like the perplexity optimizer but in a function.. */
  //  vector<CountVector> _probCountVectors();
  //  vector<CountVector> _bowCountVectors();
  //  size_t              _numOOV = 0;
  //  size_t              _numWords = 0;
  //  size_t              _numZeroProbs = 0;
  //  size_t              _numCalls = 0;
  //  double              _totLogProb = 0;
  //  
  //  
  //
  //  BitVector vocabMask(_lm.vocab().size(), 1);
  //  _lm._pModel->LoadEvalCorpus(_probCountVectors, _bowCountVectors,
  //                              vocabMask, corpusFile, _numOOV, _numWords);
  //  
  //  vector<BitVector> probMaskVectors(_order + 1);
  //  vector<BitVector> bowMaskVectors(_order);
  //  for (size_t o = 0; o <= _order; o++)
  //    probMaskVectors[o] = (_probCountVectors[o] > 0);
  //  for (size_t o = 0; o < _order; o++)
  //    bowMaskVectors[o] = (_bowCountVectors[o] > 0);
  //  SharedPtr<Mask>     _mask = _lm.GetMask(probMaskVectors, bowMaskVectors);
  //  
  //  /* now we're all initialized */
  //
  //  for (size_t o = 0; o <= _order; o++) {
  //    // assert(alltrue(counts == 0 || probs > 0));
  //    // _totLogProb += dot(log(probs), counts, counts > 0);
  //    // _totLogProb += sum((log(probs) * counts)[counts > 0]);
  //    const CountVector &counts(_probCountVectors[o]);
  //    const ProbVector & probs(_lm.probs(o));
  //    for (size_t i = 0; i < counts.length(); i++) {
  //      if (counts[i] > 0) {
  //        assert(std::isfinite(probs[i]));
  //        if (probs[i] == 0)
  //          _numZeroProbs++;
  //        else
  //          _totLogProb += log(probs[i]) * counts[i];
  //      }
  //    }
  //  }
  //
  //  
  //
  //} /* close scope */
  //
}
