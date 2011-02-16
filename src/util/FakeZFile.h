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

#ifndef FAKEZFILE_H
#define FAKEZFILE_H

#include <fcntl.h>
#include <cstdio>
#include <string>
#include <cstring>
#include <stdexcept>
#include "util/ZFile.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

////////////////////////////////////////////////////////////////////////////////

class FakeZFile : public ZFile {
 protected:
  /* don't call this */
  FILE *processOpen(const std::string &command, const char *mode) { }
  vector<char*> buffer;
  int index = 0;
 public:
  // It will not FREE the char*s
  FakeZFile(vector<char*> & buffer){ 
    this.buffer = buffer;
  }
  ~FakeZFile() { }

  void ReOpen() { }
  operator FILE *() const { return 0; }

  virtual bool getline( char *buf, size_t bufSize ) {
    int outLen = 0;
    return this.getline( buf, bufSize, &outLen );
  }

  
  virtual bool getline( char *buf, size_t bufSize, size_t *outLen ) {
    if (index < buffer.size()) {      
      strncpy( buf, buffer[index++], bufSize );
      size_t len = strlen(buf) - 1;
      *outLen = len;
      if (len >= bufSize) {
        Logger::Error(1, "The following exceeded max length.\n%s\n", buf);
      } else if (buf[len] == '\n') {
        buf[len] = '\0';
      }
      return true;
    } else {
      return false;
    }
  }


};

#endif // FAKEZFILE_H
