# ZBinaryIO [WIP]
[![Build Status](https://travis-ci.org/pawREP/ZBinaryReader.svg?branch=master)](https://travis-ci.org/pawREP/ZBinaryReader)
[![codecov](https://codecov.io/gh/pawREP/ZBinaryReader/branch/master/graph/badge.svg)](https://codecov.io/gh/pawREP/ZBinaryReader) 
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/882591c88b0b45128142c94b47de7e22)](https://www.codacy.com/manual/pawREP/ZBinaryReader?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=pawREP/ZBinaryReader&amp;utm_campaign=Badge_Grade)

ZBinaryIO is a modern, header-only, cross-platform C++17 binary reader library.

The library provides binary reader and writer interfaces for files and buffers. Additional data sources and sinks can be supported via classes that implement the `ISource` and `ISink` interfaces. 

ZBinaryIO is continuously compiled and tested on:
-  gcc 9.3.0
-  clang 9.0.0
-  MSVC 2019 ver. 16.5 (1925) 