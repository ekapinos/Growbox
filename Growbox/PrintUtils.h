#ifndef PrintUtils_h
#define PrintUtils_h

#include "StringUtils.h"

namespace PrintUtils {

  void printHEX(const String &input);

  void printWithoutCRLF(const String &input);

  void printRAM(void *ptr, byte sizeOf);

};

#endif










