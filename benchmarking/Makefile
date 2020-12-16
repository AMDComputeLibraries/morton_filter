# Copyright (c) 2019 Advanced Micro Devices, Inc.
 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Author: Alex D. Breslow 
#         Advanced Micro Devices, Inc.
#         AMD Research
#
# Code Source: https://github.com/AMDComputeLibraries/morton_filter
#
# VLDB 2018 Paper: https://www.vldb.org/pvldb/vol11/p1041-breslow.pdf
#
# How To Cite:
#  Alex D. Breslow and Nuwan S. Jayasena. Morton Filters: Faster, Space-Efficient
#  Cuckoo Filters Via Biasing, Compression, and Decoupled Logical Sparsity. PVLDB,
#  11(9):1041-1055, 2018
#  DOI: https://doi.org/10.14778/3213880.3213884


ifndef CXX
  CXX=g++ # Code uses GCC builtins.  I recommend using either g++ or clang++.  
          # GCC's g++ is probably preferred since it appears to yield better 
	  # throughput numbers.  However, the results are roughly comparable.
endif

# Comment the line below back in if you want to instrument the code with the 
# sanitizer functionality
#SANITIZE:=-fsanitize=address,undefined,leak

OPT=-Ofast -march=native -mpopcnt 

FLAGS:=-Wall -Winline -g -std=c++11 $(OPT) $(SANITIZE)

# Need to routinely check for bugs with -fsanitize=address -fsanitize=undefined

TARGETS=benchmark \
  benchmark_cf benchmark_mf \
  benchmark_ss_cf measure_bucket_accesses

CLEAN=rm -f *.o *.a *.so *.lo *.s $(TARGETS)

HEADERS=$(wildcard *.h)
SRCS=$(wildcard *.cc)

# Relative path to the original cuckoo filter source code
CUCKOO_REFERENCE_DIR=./cuckoofilter

INCLUDE=-I$(CUCKOO_REFERENCE_DIR)/src -I../
LDFLAGS="$(CUCKOO_REFERENCE_DIR)"/src/hashutil.o -lcrypto -lm

# Patches the baseline cuckoo filter implementation and then builds the 
# performance benchmarking programs
default: $(HEADERS) $(SRCS)
	cd .; \
  if [ ! -d ./cuckoofilter ]; then \
    git clone https://github.com/efficient/cuckoofilter.git; \
    cd ./cuckoofilter; \
    git checkout 4593377e64cb37342b44bfcee0655104c7dfaa64; \
    git apply ../cf_benchmarking.patch; \
  fi

	$(CXX) $(INCLUDE) $(FLAGS) benchmark.cc -o benchmark
	cp benchmark_mf_config.h.template benchmark_mf_config.h && $(CXX) $(INCLUDE) $(FLAGS) benchmark_mf.cc -o benchmark_mf 
	$(CXX) $(INCLUDE) $(FLAGS) -DNDEBUG benchmark_cf.cc -o benchmark_cf
	$(CXX) $(INCLUDE) $(FLAGS) -DNDEBUG benchmark_ss_cf.cc -o benchmark_ss_cf
	$(CXX) $(INCLUDE) $(FLAGS) measure_bucket_accesses.cc -o measure_bucket_accesses

clean:
	$(CLEAN)
