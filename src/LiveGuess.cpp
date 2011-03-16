#include "LiveGuess.h"




std::auto_ptr< std::vector<LiveGuessResult> > LiveGuess::Predict( char * str, int predictions ) {
  vector<char *> words(256);
  int len = strlen(str) + 1;
  char strSpace[len];
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
  // words are parsed

  // now make a fake file
  std::vector< char * > zfileContents();
  zfileContents.push_back( str );
  FakeZFile corpusFile( zfileContents );

  /* stolen from Perplexity */
  {
    /* it's like the perplexity optimizer but in a function.. */
    vector<CountVector> _probCountVectors();
    vector<CountVector> _bowCountVectors();
    size_t              _numOOV = 0;
    size_t              _numWords = 0;
    size_t              _numZeroProbs = 0;
    size_t              _numCalls = 0;
    double              _totLogProb = 0;
    
    

    BitVector vocabMask(_lm.vocab().size(), 1);
    _lm._pModel->LoadEvalCorpus(_probCountVectors, _bowCountVectors,
                                vocabMask, corpusFile, _numOOV, _numWords);
    
    vector<BitVector> probMaskVectors(_order + 1);
    vector<BitVector> bowMaskVectors(_order);
    for (size_t o = 0; o <= _order; o++)
      probMaskVectors[o] = (_probCountVectors[o] > 0);
    for (size_t o = 0; o < _order; o++)
      bowMaskVectors[o] = (_bowCountVectors[o] > 0);
    SharedPtr<Mask>     _mask = _lm.GetMask(probMaskVectors, bowMaskVectors);
    
    /* now we're all initialized */
    

  } /* close scope */
}
