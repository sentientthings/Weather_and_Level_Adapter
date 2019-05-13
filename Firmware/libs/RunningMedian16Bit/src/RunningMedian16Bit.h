//
//    FILE: RunningMedian.h
//  AUTHOR: Rob dot Tillaart at gmail dot com
// PURPOSE: RunningMedian library for Arduino
// VERSION: 0.1.15
//     URL: http://arduino.cc/playground/Main/RunningMedian
// HISTORY: See RunningMedian.cpp
//
// Released to the public domain
//
// Modified by Robert Mawrey for uint16_t values
// Not really necessary but saves a little space
// The size of 701 is likely more important for this application
// and the original library does not make it dynamic

#ifndef RunningMedian16Bit_h
#define RunningMedian16Bit_h
/*
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
*/

#include "application.h"

#include <inttypes.h>

#define RUNNING_MEDIAN_VERSION "0.1.15"

// prepare for dynamic version
// not tested use at own risk :)
//#define RUNNING_MEDIAN_USE_MALLOC

// conditional compile to minimize lib
// by removeing a lot of functions.
#ifndef RUNNING_MEDIAN_ALL
#define RUNNING_MEDIAN_ALL
#endif


// should at least be 5 to be practical
// odd size results in a 'real' middle element.
// even size takes the lower of the two middle elements
#ifndef MEDIAN_MIN_SIZE
#define MEDIAN_MIN_SIZE     1
#endif
#ifndef MEDIAN_MAX_SIZE
#define MEDIAN_MAX_SIZE     701          // adjust if needed
#endif

#define NAN NULL

// Robert Mawrey changes
// Changed medians values from float to uint16_t for Maxbotix devices
// Changed uint8_t indexes to uint16_t to accommodate larger arrays

class RunningMedian
{
public:
  explicit RunningMedian(const uint16_t size);  // # elements in the internal buffer
  ~RunningMedian();                            // destructor

  void clear();                        // resets internal buffer and var
//  void add(const float value);        // adds a new value to internal buffer, optionally replacing the oldest element.
void add(const uint16_t value);
  uint16_t getMedian();                  // returns the median == middle element

#ifdef RUNNING_MEDIAN_ALL
  float getAverage();                 // returns average of the values in the internal buffer
  float getAverage(uint16_t nMedian);  // returns average of the middle nMedian values, removes noise from outliers
  uint16_t getHighest() { return getSortedElement(_cnt - 1); };
  uint16_t getLowest()  { return getSortedElement(0); };

  uint16_t getElement(const uint16_t n);        // get n'th element from the values in time order
  uint16_t getSortedElement(const uint16_t n);  // get n'th element from the values in size order
  float predict(const uint16_t n);           // predict the max change of median after n additions

  uint16_t getSize() { return _size; };  // returns size of internal buffer
  uint16_t getCount() { return _cnt; };  // returns current used elements, getCount() <= getSize()
#endif

protected:
  boolean _sorted;
  uint16_t _size;
  uint16_t _cnt;
  uint16_t _idx;

#ifdef RUNNING_MEDIAN_USE_MALLOC
//  float * _ar;
uint16_t * _ar;
  uint16_t * _p;
#else
//  float _ar[MEDIAN_MAX_SIZE];
uint16_t _ar[MEDIAN_MAX_SIZE];
  uint16_t _p[MEDIAN_MAX_SIZE];
#endif
  void sort();
};

#endif
// END OF FILE
