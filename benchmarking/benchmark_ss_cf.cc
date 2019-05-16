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
#include "benchmark_util.h"
#include "vector_types.h"
#include "benchmark_common.h" // slot_fraction and other TBD parameters
#include "cuckoofilter.h"

using namespace Benchmark;

template<uint64_t fingerprint_len_bits>
double benchmark_insertions(uint64_t total_slots, double target_lf){
  uint64_t total_buckets = total_slots / 4; // 4 slots per bucket
  cuckoofilter::CuckooFilter<uint64_t, fingerprint_len_bits, 
    cuckoofilter::PackedTable> cf(total_buckets, false); // PackedTable is ss-CF, SingleTable CF

  std::vector<keys_t> insert_items(total_slots);
  populate_with_random_numbers<keys_t>(insert_items);

  uint64_t items_to_insert_to_hit_lf_target = target_lf * total_slots;
  for(uint64_t i = 0; i < items_to_insert_to_hit_lf_target; i++){
    cf.Add(insert_items[i]);
  }

  // Add slot_fraction of load to the table beyond the target load factor
  uint64_t secondary_insert_count = total_slots * slot_fraction;

  time_point start = now();
  for(uint64_t i = items_to_insert_to_hit_lf_target; i < items_to_insert_to_hit_lf_target + secondary_insert_count; i++){
    cf.Add(insert_items[i]);
  }
  std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);

  return secondary_insert_count / (diff.count() * 1e6);
}

template<uint64_t fingerprint_len_bits>
double benchmark_deletions(uint64_t total_slots, double target_lf){
  uint64_t total_buckets = total_slots / 4; // 4 slots per bucket
  cuckoofilter::CuckooFilter<uint64_t, fingerprint_len_bits, 
    cuckoofilter::PackedTable> cf(total_buckets, false);

  // Delete slot_fraction of load from the table after hitting the target
  uint64_t delete_count = total_slots * slot_fraction;

  std::vector<keys_t> insert_items(total_slots * target_lf);
  std::vector<keys_t> delete_items(delete_count);

  // Generate items to probe (no duplicates)
  populate_with_random_numbers<keys_t>(insert_items, delete_items, 1.0, 0);

  uint64_t items_to_insert_to_hit_lf_target = target_lf * total_slots;
  for(uint64_t i = 0; i < items_to_insert_to_hit_lf_target; i++){
    cf.Add(insert_items[i]);
  }
  
  time_point start = now();
  for(uint64_t i = 0; i < delete_count; i++){
    cf.Delete(delete_items[i]);
  }
  std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);

  return delete_count / (diff.count() * 1e6);
}

template<uint64_t fingerprint_len_bits>
double benchmark_lookups(uint64_t total_slots, double target_lf, 
  double overlap){
  uint64_t total_buckets = total_slots / 4; // 4 slots per bucket
  cuckoofilter::CuckooFilter<uint64_t, fingerprint_len_bits, 
    cuckoofilter::PackedTable> cf(total_buckets, false);

  // Look up slot_fraction worth of keys (see above comments)
  uint64_t lookup_count = total_slots * slot_fraction;

  std::vector<keys_t> insert_items(total_slots * target_lf);
  std::vector<keys_t> probe_items(lookup_count);

  // Generate items to probe (no duplicates)
  populate_with_random_numbers<keys_t>(insert_items, probe_items, overlap, 0);

  uint64_t items_to_insert_to_hit_lf_target = target_lf * total_slots;
  for(uint64_t i = 0; i < items_to_insert_to_hit_lf_target; i++){
    cf.Add(insert_items[i]);
  }
  
  time_point start = now();
  for(uint64_t i = 0; i < lookup_count; i++){
    cf.Contain(probe_items[i]);
  }
  std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);

  return lookup_count / (diff.count() * 1e6);
}

int main(int argc, char** argv){
  constexpr uint64_t fingerprint_len_bits = 13;
  constexpr uint64_t total_slots = 128 * 1024 * 1024;
  std::vector<double> lfs = {0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.35, 0.40, 
    0.45, 0.50, 0.55, 0.60, 0.65, 0.70, 0.75, 0.80, 0.85, 0.90, 0.95};

  // Deletions
  std::vector<double> delete_throughputs(lfs.size());
  std::cout << "FILTER  LOAD  OPERATION  THROUGHPUT\n";
  for(uint64_t i = 0; i < lfs.size(); i++){
    double lf = lfs[i];
    delete_throughputs[i] = benchmark_deletions<fingerprint_len_bits>(
      total_slots, lf);
    std::cout << "SF  " << lf << " DELETE " << delete_throughputs[i] << std::endl;
  }

  // Lookups
  uint64_t gradations = 1.0; // 0.0 to 1.0 inclusive in gaps of 1.0/gradations
  std::vector<double> lookup_throughputs(lfs.size());
  std::cout << "FILTER  LOAD  OPERATION  THROUGHPUT %%TRUE_POSITIVE\n";
  for(uint64_t g = 0; g <= gradations; g++){
    double true_positive = (1.0 * g) / gradations;
    for(uint64_t i = 0; i < lfs.size(); i++){
      double lf = lfs[i];
      lookup_throughputs[i] = benchmark_lookups<fingerprint_len_bits>(
        total_slots, lf, true_positive);
      std::cout << "SF  " << lf << " LOOKUP " << lookup_throughputs[i] << " " << true_positive << std::endl;
    }
  }

  // Insertions
  std::vector<double> insert_throughputs(lfs.size());
  std::cout << "FILTER  LOAD  OPERATION  THROUGHPUT\n";
  for(uint64_t i = 0; i < lfs.size(); i++){
    double lf = lfs[i];
    insert_throughputs[i] = benchmark_insertions<fingerprint_len_bits>(
      total_slots, lf);
    std::cout << "SF  " << lf << " INSERT " << insert_throughputs[i] << std::endl;
  }

   
  return 0;
}
