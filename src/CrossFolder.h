#ifndef CROSSFOLDER_H
#define CROSSFOLDER_H

#include <memory>
#include <vector>


////////////////////////////////////////////////////////////////////////////////

class CrossFolder {
 protected:
  int folds;
  int currentFold;
  std::vector<char *> lines;
  std::vector<char *> testset;
  std::vector<char *> trainingset;
  int * indices;
  char * filename;
 public:
  CrossFolder( const char * filename, int folds );
  ~CrossFolder();
  int getFolds();
  // Iterate to the next fold
  // - testSet() and trainingSet() will return new files
  void nextFold();
  bool foldsLeft();
  string getFoldName();
  std::auto_ptr< ZFile> testSet();
  std::auto_ptr< ZFile> trainingSet();
};



#endif


