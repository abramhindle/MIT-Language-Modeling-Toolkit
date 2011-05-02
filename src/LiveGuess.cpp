
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
//#include <boost/pending/relaxed_heap.hpp>
#include <algorithm>

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

void mkHeap(std::vector<VocabProb> & heap) {
  // I hate the STL heaps
  make_heap (heap.begin(),heap.end());  
}
void popHeap( std::vector<VocabProb> & heap ) {
  pop_heap (heap.begin(),heap.end());  
  heap.pop_back();
}
void pushHeap( std::vector<VocabProb> & heap, const VocabProb & v) {
  heap.push_back( v ); 
  push_heap( heap.begin(), heap.end() );
}
void sortHeap( std::vector<VocabProb> & heap ) { 
  sort_heap( heap.begin(), heap.end() ); // heap now is internally sorted 
}

// WARNING THIS ALLOCATES MEMORY //
char * joinVectorOfCStrings( std::vector<const char*> & words ) {
  int len = 1; // null term
  for (int i = 0; i < words.size(); i++) {
    len += strlen( words[i] );
  }
  if (words.size() > 0) {
    len += words.size() - 1;
  }
  char * v = new char[ len ];
  char * o = v;
  int lastIndex = words.size() - 1;
  for (int i = 0; i <= lastIndex; i++) {
    const char * word = words[i];
    int wlen = strlen( word );
    CopyString( o, word );
    o += wlen; // jump to end of word
    if (i != lastIndex) {
      *o++ = ' '; //add space and skip it
    } else {
      *o++ = '\0';
    }
  }
  return v;
}

// Returns a vector of LiveGuessResults
// warning: words is mutated temporarily
std::auto_ptr< std::vector<LiveGuessResult> > 
forwardish(std::vector<const char *> & words, // the current words can be empty
           const double currentProb, // log prob
           const int size, // how many to grab
           const int depthLeft,
           const NgramLM & _lm, 
           const int _order,  
           const Vocab & vocab ) {
  
  // Index contains the last ngram word 



  //Logger::Log(0, "Forwardish [%d] [%d]\n", depthLeft, index);

  VocabIndex vwords[ _order ];
  //int n = (words.size() < (_order - 1))?words.size():_order;

  //for (int i = words.size() - _order - 1; i < words.size(); i++) {
  //  if ( i >= 0) {
  //    Logger::Log(0,"Word: %d %s\n",i,words[i]);
  //  }
  //}
  
  for (int i = 1; i < _order; i++) {
    int j = words.size() - _order + i;
    if (j < 0) {
      vwords[i - 1] = Vocab::EndOfSentence;
    } else {
      vwords[i - 1] = vocab.Find( words[ j ] );
    }
  }

  

  vector<VocabProb> heap(0);

  mkHeap(heap);

  const ProbVector & probabilities = _lm.probs(  _order ) ;// _order - 2  );
  const CountVector & counts = _lm.counts( _order );
  
  int count = 0;
  //Logger::Log(0, "Find probabilities %d\n",vocab.size());

  for (int j = 0; j < vocab.size(); j++) {
    VocabIndex vWordI = j;//vocab[j];
    vwords[ _order - 1 ] = j;
    NgramIndex newIndex = _lm.model()._Find( vwords, _order );
    
    if (newIndex == -1) { // not legit :(
      continue;
    }
    Prob probRaw = probabilities[ newIndex ];
    if (probRaw == 0.0) {
      continue;
    }
    Prob prob = -1 * log( probRaw ); //biggest is smallest

    //Prob prob = (probRaw == 0.0)?10000:(-1 * log( probRaw )); //biggest is smallest
    //Prob probRaw = (counts[newIndex]==0)?1.0:counts[newIndex]/vocab.size()
    //Prob prob = -1 * log(probRaw);
    //Prob prob = -1 * counts[newIndex];
    //Logger::Log(0, "Prob %e\n",prob);

    const VocabProb v( prob,j, newIndex);
    if ( count < size ) {
      heap.push_back( v );
      count++;
      if (count == size) {
        mkHeap( heap );
      }
      // this is irritating, basically it means the highest rank stuff
      // will be in the list and we only kick out the lowest ranked stuff
      // (which will be the GREATEST of what is already there)
      // 
    } else if (  heap.front().prob >  prob ) {
      // this is dumb        
      // remove the least element
      popHeap( heap );
      pushHeap( heap, v );
      // should we update?
    }
  }
  sortHeap( heap );

  std::vector<LiveGuessResult> * resVector = new std::vector<LiveGuessResult>();
  
  for( int j = 0; j < heap.size(); j++) {
    VocabProb v = heap[ j ];
    Prob prob = v.prob;
    prob += currentProb;
    const char * word = vocab[ v.index ];
    vector<const char *> ourWords(words);
    ourWords.push_back( word ); // add 
    char * str = joinVectorOfCStrings( ourWords ); // Remember to deallocate later :(
    resVector->push_back( LiveGuessResult( prob , str  )); 
  }
  
  if ( depthLeft <= 0 ) {

  } else {
    //Let's recurse!
    for( int j = 0; j < heap.size(); j++) {
      VocabProb v = heap[ j ];
      Prob prob = v.prob;
      prob += currentProb;
      words.push_back( vocab[ v.index ] );
      std::auto_ptr< std::vector<LiveGuessResult> > r = 
        forwardish( words, 
                    prob,
                    size,
                    depthLeft - 1,
                    _lm, 
                    _order,  
                    vocab );
      words.pop_back(); // and restore
      for (int i = 0; i < r->size(); i++) {
        resVector->push_back( (*r)[i] );
      }
    }
  }


  std::auto_ptr< std::vector<LiveGuessResult> > returnValues( resVector );
  return returnValues;
}
bool mySortLiveGuessFunction (LiveGuessResult a, LiveGuessResult b) {
  return (a.probability > b.probability);
}
void sortLiveGuesses( std::vector<LiveGuessResult> & v ) {
  sort (v.begin(), v.end(), mySortLiveGuessFunction); 
}

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
  const Vocab & vocab = _lm.vocab();

  vector<const char *> ourWords(words); // clone it
  std::auto_ptr< std::vector<LiveGuessResult> > returnValues = forwardish(
                                                                          ourWords,
                                                                          0.0,
                                                                          _order, // 4 words deep
                                                                          _order, // 4 words deep?
                                                                          _lm,
                                                                          _order,
                                                                          vocab);
  sortLiveGuesses( *returnValues );
  return returnValues;
                                                                          
}
