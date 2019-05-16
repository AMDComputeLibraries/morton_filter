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
// This file contains code to measure the bucket accesses per insertion when 
// using a random kickout insertion algorithm.  To make it properly work,
// you need to set _print_access_counts to true in the main classes.

#include "benchmark_util.h"
#include "morton_filter.h"

// Fan et al.'s cuckoo filter implementation
#include "cuckoofilter.h"

using namespace Benchmark;

int main(int argc, char** argv){
  // Fan et al.'s cuckoo filter implementation requires this to be a power of 2
  constexpr uint64_t total_slots = 128ULL * 1024ULL * 1024ULL; 
  constexpr double load_factor = 0.95;
  constexpr uint64_t total_items_to_insert = total_slots * load_factor;
  constexpr uint64_t total_items_to_probe = total_items_to_insert;

  constexpr double insert_and_probe_item_overlap = 1.0;

  std::vector<keys_t> insert_items(total_items_to_insert);
    std::vector<keys_t> probe_items(total_items_to_probe);

  bool duplicates_permitted_in_probe_vector = true;

  populate_with_random_numbers<keys_t>(insert_items, probe_items,
    insert_and_probe_item_overlap, duplicates_permitted_in_probe_vector);

  CompressedCuckoo::Morton3_8 ccf(total_slots);

  #if 1
  std::cout << "Primary,BlockOverflow,BucketOverflow,HybridOverflow\n";
  for(uint64_t i = 0; i < total_items_to_insert; i++){
    hash_t raw_hash = ccf.raw_primary_hash(insert_items[i]);
    ccf.random_kickout_cuckoo(ccf.map_to_bucket(raw_hash, ccf._total_buckets), 
      ccf.fingerprint_function(raw_hash));
    //ccf.table_store(ccf.map_to_bucket(raw_hash, ccf._total_buckets), 
    //  ccf.fingerprint_function(raw_hash));
  }
  #endif
  #if 0
  constexpr uint64_t fingerprint_len_bits = 12;
  constexpr uint64_t buckets = total_slots / 4;
  cuckoofilter::CuckooFilter<uint64_t, fingerprint_len_bits, cuckoofilter::SingleTable> 
    cf(buckets, false);
  std::cout << "Count\n";
  for(uint64_t i = 0; i < total_items_to_insert; i++){
    cf.Add(insert_items[i]);
  }
  #endif
  return 0;
}
