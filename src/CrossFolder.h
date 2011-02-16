#ifndef NGRAMLM_H
#define NGRAMLM_H

#include <vector>
#include "util/SharedPtr.h"
#include "Types.h"
#include "Vocab.h"
#include "NgramModel.h"
#include "Smoothing.h"
#include "Mask.h"

using std::vector;

////////////////////////////////////////////////////////////////////////////////

class CrossFolder {
 protected:
  int folds;
  int currentFold;

 public:
  CrossFolder( char * filename, int folds );
  int getFolds() { return this.folds; }
  void nextFold();
  void foldsLeft() { return currentFold < folds; }
  
