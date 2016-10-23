//  (C) Copyright Ryan Pavlik 2012
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version.

//  AVR (embedded, with avr-libc) config options:
//  (Requires a C++ standard library like STLport - https://github.com/vancegroup/stlport-avr/wiki )

#define BOOST_PLATFORM "avr"

// No wchar_t in avr-libc
#define BOOST_NO_INTRINSIC_WCHAR_T

// Asserts don't make sense (or work right) in embedded
#define BOOST_DISABLE_ASSERTS

// Defined by arduino/wiring WString.h
#ifdef F
  #undef F
#endif
