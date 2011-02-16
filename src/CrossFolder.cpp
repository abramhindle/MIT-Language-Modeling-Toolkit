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

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

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

using std::string;
using std::vector;
using std::stringstream;

#define MAXLINE 4096

////////////////////////////////////////////////////////////////////////////////

void shuffle( int * arr , int n ) {
  for ( int i = n - 1 ; i >= 1 ; i-- ) {
    int r = int( i * ((double)rand()) / (double)(RAND_MAX + 1.0) );
    // Logger::Log(0, "Shuffle: %d -- %d\n", i, r);

    int tmp = arr[ i ];
    arr[ i ] = arr[ r ];
    arr[ r ] = tmp;
  }
}


CrossFolder::CrossFolder( const char * ourFileName, int ourFolds ): lines(), testset(), trainingset() {
  folds = ourFolds;
  filename = NULL;
  ZFile file( ourFileName );
  char line[MAXLINE];
  {
    int len = strnlen( ourFileName, MAXLINE ) + 1;
    filename = new char[ len ];
    filename[ len - 1 ] = '\0';
    strncpy( filename, ourFileName, len - 1 );
  }


  // Read the file
  while (file.getLine( line, MAXLINE )) {

    int len = strnlen( line, MAXLINE ) + 1;
    char * str = new char[ len ];
    str[len - 1] = '\0'; // is this needed?
    strncpy( str, line, len );
    lines.push_back( str );
  }

  // Generate the folds 
  int n = lines.size();
  indices = new int[ n ];
  for (int i = 0; i < n ; i++ ) {
    indices[ i ] = folds * i / n;
    // Logger::Log(0, "Have: %d -- %s\n", indices[ i ] , lines[ i ]);
  }
  shuffle( indices, n );
  // for (int i = 0; i < n ; i++ ) {
  //  Logger::Log(0, "Have: %d -- %s\n", indices[ i ] , lines[ i ]);
  // }
  currentFold = -1; // HACK
  nextFold();

}

CrossFolder::~CrossFolder() {
  delete indices;
  for (int i = 0; i < lines.size() ; i++ ) {
    delete lines[ i ];
  }
  delete filename;  
}

void CrossFolder::nextFold() {
  currentFold++;
  testset.clear();
  trainingset.clear();
  for(int i = 0 ; i < lines.size(); i++ ) {
    if ( indices[ i ] == currentFold ) {
      testset.push_back( lines[ i ] );
    } else {
      trainingset.push_back( lines[ i ] );
    }
  }
}

std::auto_ptr< ZFile> CrossFolder::testSet() {
  std::auto_ptr< ZFile> zfile( new FakeZFile( testset  ) );
  return zfile;
}

std::auto_ptr< ZFile> CrossFolder::trainingSet() {
  std::auto_ptr< ZFile> zfile( new FakeZFile( trainingset  ) );
  return zfile;
}

string CrossFolder::getFoldName() {
  stringstream out( stringstream::in );
  out << currentFold;
  return (((string)filename) + ":" + out.str());
}

int CrossFolder::getFolds() { return folds; }
bool CrossFolder::foldsLeft() { return (currentFold < folds); };
