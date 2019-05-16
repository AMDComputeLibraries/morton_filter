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
#ifndef _BENCHMARK_MF_H
#define _BENCHMARK_MF_H

#include "compressed_cuckoo_filter.h"
#include "benchmark_util.h"
#include "vector_types.h"
#include "benchmark_mf_config.h" // bench_mf namespace parameters defined here
#include "benchmark_common.h" // slot_fraction and TBD other parameters

using namespace Benchmark;
using Morton_Type = CompressedCuckoo::CompressedCuckooFilter<
    bench_mf::slots_per_bucket,
    bench_mf::fingerprint_len_bits,
    bench_mf::ota_len_bits,
    bench_mf::block_size_bits, // block size
    bench_mf::target_compression_ratio_sfp,
    bench_mf::read_counters_method, 
    bench_mf::read_fingerprints_method,
    bench_mf::reduction_method,
    bench_mf::alternate_bucket_selection_method, 
    bench_mf::morton_ota_hashing_method,
    bench_mf::resizing_enabled, // can the table resize?
    bench_mf::remap_enabled, // remapping of items from first bucket enabled
    bench_mf::collision_resolution_enabled, // collision resolution enabled
    bench_mf::morton_filter_functionality_enabled, 
    bench_mf::block_fullness_array_enabled, // Block fullness array enabled
    bench_mf::handle_conflicts,  // Handle conflicts on insertions enabled
    bench_mf::fingerprint_comparison_method  
  >;

template<uint64_t fingerprint_len_bits>
double benchmark_insertions(uint64_t total_slots, double target_lf){
  Morton_Type cf(total_slots);

  // Insert a number of keys equal to 0.1% (when slot_fraction is 0.001)
  // of the total slots in the table (configuration from VLDB'18 paper)
  // after hitting the target load factor
  uint64_t secondary_insert_count = to_multiple_of_batch(total_slots * slot_fraction, 
    batch_size);

  std::vector<keys_t> insert_items(total_slots);
  std::vector<keys_t> benchmark_insert_items(secondary_insert_count);

  populate_with_random_numbers<keys_t>(insert_items, benchmark_insert_items, 
    0.0, 1);

  uint64_t items_to_insert_to_hit_lf_target = 
    to_multiple_of_batch(target_lf * total_slots, batch_size);

  std::vector<bool> status(items_to_insert_to_hit_lf_target, false);
  std::vector<bool> status_benchmark(secondary_insert_count, false);
   
  // Insert up to the target load factor 
  cf.insert_many(insert_items, status, items_to_insert_to_hit_lf_target);

  time_point start = now();
  cf.insert_many(benchmark_insert_items, status_benchmark, 
    secondary_insert_count);
  std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);

  uint64_t success_count = std::accumulate(status_benchmark.begin(), status_benchmark.end(), 0);
  if(success_count != secondary_insert_count){
    std::cerr << "Only " << success_count << " of " << secondary_insert_count << " insertions succeeded.\n";
  }

  return secondary_insert_count / (diff.count() * 1e6);
}

template<uint64_t fingerprint_len_bits>
double benchmark_deletions(uint64_t total_slots, double target_lf){
  Morton_Type cf(total_slots);

  // Delete a number of keys equal to 0.1% (when slot_fraction is 0.001)
  // of the total slots in the table (configuration from VLDB'18 paper)
  // after hitting the target load factor
  uint64_t delete_count = to_multiple_of_batch(total_slots * slot_fraction, 
    batch_size);

  uint64_t items_to_insert_to_hit_lf_target = 
    to_multiple_of_batch(target_lf * total_slots, batch_size);

  std::vector<keys_t> insert_items(items_to_insert_to_hit_lf_target);
  std::vector<keys_t> delete_items(delete_count);

  // Generate items to probe (no duplicates)
  populate_with_random_numbers<keys_t>(insert_items, delete_items, 1.0, 0);

  std::vector<bool> status(items_to_insert_to_hit_lf_target, false);
  std::vector<bool> status_benchmark(delete_count, false);

  cf.insert_many(insert_items, status, items_to_insert_to_hit_lf_target);
  
  time_point start = now();
  cf.delete_many(delete_items, status_benchmark, delete_count);
  std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);

  uint64_t success_count = std::accumulate(status_benchmark.begin(), status_benchmark.end(), 0);
  if(success_count != delete_count){
    std::cerr << "Only " << success_count << " of " << delete_count << " deletions succeeded.\n";
  }

  return delete_count / (diff.count() * 1e6);
}

template<uint64_t fingerprint_len_bits>
double benchmark_lookups(uint64_t total_slots, double target_lf, 
  double overlap){
  Morton_Type cf(total_slots);

  // Look a number of keys equal to 0.1% (when slot_fraction is 0.001)
  // of the total slots in the table (configuration from VLDB'18 paper)
  uint64_t lookup_count = 1024 * 1024; //to_multiple_of_batch(total_slots * slot_fraction, 
    //batch_size);

  uint64_t items_to_insert_to_hit_lf_target = 
    to_multiple_of_batch(target_lf * total_slots, batch_size);

  std::vector<keys_t> insert_items(items_to_insert_to_hit_lf_target);
  std::vector<keys_t> probe_items(lookup_count);

  // Generate items to probe (no duplicates)
  populate_with_random_numbers<keys_t>(insert_items, probe_items, overlap, 1);

  std::vector<bool> status(items_to_insert_to_hit_lf_target, false);
  std::vector<bool> status_benchmark(lookup_count, false);
 
  cf.insert_many(insert_items, status, items_to_insert_to_hit_lf_target); 
 
  time_point start = now();
  cf.likely_contains_many(probe_items, status_benchmark, lookup_count);
  std::chrono::duration<double> diff = 
    std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);

  if(overlap == 1.0){
    uint64_t success_count = std::accumulate(status_benchmark.begin(), status_benchmark.end(), 0);
    if(success_count != lookup_count){
      std::cerr << "Only " << success_count << " of " << lookup_count << " lookups succeeded.\n";
    }
  }

  return lookup_count / (diff.count() * 1e6);
}

#endif
