/*
Copyright (c) 2019 Advanced Micro Devices, Inc.
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Author: Alex D. Breslow 
        Advanced Micro Devices, Inc.
        AMD Research

Code Source: https://github.com/AMDComputeLibraries/morton_filter

VLDB 2018 Paper: https://www.vldb.org/pvldb/vol11/p1041-breslow.pdf

How To Cite:
  Alex D. Breslow and Nuwan S. Jayasena. Morton Filters: Faster, Space-Efficient
  Cuckoo Filters Via Biasing, Compression, and Decoupled Logical Sparsity. PVLDB,
  11(9):1041-1055, 2018
  DOI: https://doi.org/10.14778/3213880.3213884

*/
#ifndef _TEST_UTIL_H
#define _TEST_UTIL_H

namespace Test{
  std::string pass(bool success_status){
    return std::string(success_status ? "SUCCESS" : "FAILURE");
  }
};

#endif
