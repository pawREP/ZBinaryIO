# ZBinaryReader
[![Build Status](https://travis-ci.org/pawREP/ZBinaryReader.svg?branch=master)](https://travis-ci.org/pawREP/ZBinaryReader)
[![codecov](https://codecov.io/gh/pawREP/ZBinaryReader/branch/master/graph/badge.svg)](https://codecov.io/gh/pawREP/ZBinaryReader)


ZBinaryReader is a modern, single header, cross-platform C++17 binary reader library.

The library provides a reader interface for file and buffer data sources. Additional data sources can supported through classes that implement the `ISource` interface. Existing sources can also be extended using the mixin pattern. See `CoverageTrackingSource` for an example of such an extension. 

ZBinaryReader is continuously compiled and tested on:
 - gcc 9.3.0
 - clang 9.0.0
 - MSVC 2019 ver. 16.5 (1925) 