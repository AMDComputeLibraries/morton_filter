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
#include "morton_filter.h"

// Fan et al.'s cuckoo filter implementation
#include "cuckoofilter.h"

using namespace Benchmark;

// Round up to multiple of desired_size
constexpr uint64_t round_to_appropriate_size(uint64_t desired_size){
  return desired_size + (batch_size - desired_size % batch_size);
}

// A variant of equation (5) in the VLDB'18 paper
template<class MF_CLASS>
double calculate_mf_false_positive_ratio(MF_CLASS& mf, double ota_occupancy, 
  double logical_load_factor){
  double buckets_accessed_per_negative_lookup = 1 + ota_occupancy;
  double epsilon = 1.0 - pow(1.0 - 1.0/(1ULL << (mf._fingerprint_len_bits - 
    mf._resize_count)), 
    logical_load_factor * buckets_accessed_per_negative_lookup * 
    mf._slots_per_bucket);
  return epsilon;
}

void benchmark(){
  constexpr bool use_item_at_a_time_insertion = false;
  constexpr bool use_item_at_a_time_deletion = false;
  constexpr bool skip_cuckoo_filter = true;
  constexpr bool skip_morton_filter = false;
  constexpr bool resize_filter = false; // Set t_resizing_enabled to true when true
  // When set to true, the distribution of load across buckets and blocks is 
  // printed
  constexpr bool print_load_histogram = false;

  // Load factor ($\alpha_C$) See http://www.vldb.org/pvldb/vol11/p1041-breslow.pdf
  constexpr double block_saturation = 0.95; // Actual saturation will be less if
                                            // resize_filter is set to true
                                            // E.g., 1/2 * 0.95 if capacity is doubled
  
  // Trying to get close to 95% of 128 * 1024 * 1024 for comparing against 
  // Fan et al.'s cuckoo filter, which requires that the table be a power of 2
  constexpr uint64_t total_phys_slots = 128ULL * 1024ULL * 1024ULL; // Physical
   
  constexpr uint64_t total_items_to_probe = round_to_appropriate_size(total_phys_slots * block_saturation);
  constexpr uint64_t total_items_to_insert = round_to_appropriate_size(total_phys_slots * block_saturation);

  // 1.0 for 100% overlap, 0.0 for no overlap (intersection between insert and 
  // probe items is the empty set)
  // Set to 1.0 for measuring the positive lookup throughput
  // Set to 0.0 for measuring the negative lookup throughput and false positive rate
  constexpr double insert_and_probe_item_overlap = 1.0;

  std::vector<keys_t> insert_items(total_items_to_insert);
  std::vector<keys_t> probe_items(total_items_to_probe);
  
  bool duplicates_permitted_in_probe_vector = true;

  populate_with_random_numbers<keys_t>(insert_items, probe_items, 
    insert_and_probe_item_overlap, duplicates_permitted_in_probe_vector);

  time_point start;
  std::chrono::duration<double> diff;
  using MortonFilter = CompressedCuckoo::Morton3_8; // UPDATE ON CHANGE!!!
  constexpr uint64_t total_slots = total_phys_slots; // Used to be logical slots
  start = now();
  MortonFilter ccf(total_slots);
  diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
  std::cout << "Morton filter constructor time: " << diff.count() << " seconds" << std::endl;

  if(! skip_cuckoo_filter){
    static_assert(__builtin_popcountll(total_phys_slots / 4) == 1, 
      "Cuckoo filter requires that the number of buckets be a power of two.");
    
    constexpr uint16_t fingerprint_len_bits = 8;
    // Fan et al.'s cuckoo filter implementation
    start = now();
    cuckoofilter::CuckooFilter<uint64_t, fingerprint_len_bits, 
      cuckoofilter::SingleTable> cf(total_phys_slots / 4, false);
    diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
    std::cout << "Cuckoo filter constructor time: " << diff.count() << " seconds" << std::endl;

    std::cout << ccf << std::endl;

    uint64_t cuckoo_insert_count = total_items_to_insert;
    start = now();
    for(uint64_t i = 0; i < cuckoo_insert_count; i++){
      cf.Add(insert_items[i]);
    }
    diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
    std::cout << "Vanilla cuckoo filter millions of insertions per second: " 
      << (cuckoo_insert_count / ((1e6)*diff.count())) << std::endl;
 
    start = now();
    for(uint64_t i = 0; i < cuckoo_insert_count; i++){
      cf.Delete(insert_items[i]);
    }
    diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
    std::cout << "Vanilla cuckoo filter millions of deletions per second: " 
      << (cuckoo_insert_count / ((1e6)*diff.count())) << std::endl;
  }
  
  if(skip_morton_filter){
    return;
  }

  uint64_t successful_inserts = 0;
  if(use_item_at_a_time_insertion){
    std::cout << "ITEM AT A TIME INSERTIONS\n" << std::flush;
    start = now();
    for(uint64_t i = 0; i < total_items_to_insert; i++){
      successful_inserts += ccf.insert(insert_items[i]);
    } 
    diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
  }
  else{
    std::vector<bool> insert_status(total_items_to_insert, false);
    std::cout << "MANY ITEMS AT A TIME INSERTION ALGORITHM\n" << std::flush;
    start = now();
    ccf.insert_many(insert_items, insert_status, total_items_to_insert);
    diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
    for(uint64_t i = 0; i < total_items_to_insert; i++){
      successful_inserts += insert_status[i];
    }
  }
  std::cout << "Millions of insertions per second: " << (total_items_to_insert / ((1e6)*diff.count())) << std::endl;
	std::cout << successful_inserts << " of " << total_items_to_insert << 
	  " were successful insertions." << std::endl;
	std::cout << (100.0 * successful_inserts) / total_items_to_insert << "% successfully inserted\n" << std::flush;

  if(resize_filter){ 
    start = now();
    ccf.template resize<1>(); // Double capacity
    //ccf.template resize<2>(); // Increase capacity by a factor of 4 
    //ccf.template resize<3>(); // Octuple capacity
    diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
    std::cout << "Millions of items relocated per second: " << (total_items_to_insert / ((1e6)*diff.count())) << std::endl;
  }

  // Item at a time lookups
  uint64_t net_success = 0;
  start = now();
  for(uint64_t i = 0; i < total_items_to_probe; i++){
    net_success += ccf.likely_contains(probe_items[i]);
  }
  diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
  std::cout << "Millions of lookups per second: " << (total_items_to_probe / ((1e6)*diff.count())) << std::endl;
  std::cout << (100.0 * net_success) / total_items_to_probe << "% successfully retrieved\n";
  std::cout << net_success << " of " << total_items_to_probe << " were successful lookups\n";

  // Batch at a time lookups
  std::vector<bool> successes2(total_items_to_probe, false);
  start = now();
  ccf.likely_contains_many(probe_items, successes2, total_items_to_probe);
  diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
  std::cout << "Millions of lookups per second: " << (total_items_to_probe / ((1e6)*diff.count())) << std::endl;
  net_success = 0;
  for(uint64_t i = 0; i < total_items_to_probe; i++){
    net_success += successes2[i];
  } 
  std::cout << (100.0 * net_success) / total_items_to_probe << "% successfully retrieved\n";
  std::cout << net_success << " of " << total_items_to_probe << " were successful lookups\n";

  double ota_occupancy = ccf.report_ota_occupancy();
  std::cout << "OTA Occupancy Ratio: " << ota_occupancy << std::endl;
  std::cout << "Mean OTA Bits Set: " << ota_occupancy * ccf._ota_len_bits << 
    std::endl;
  double block_occupancy = ccf.report_block_occupancy();
  std::cout << "Projected False Positive Ratio: " << 
    calculate_mf_false_positive_ratio<MortonFilter>(ccf, ota_occupancy, 
    block_occupancy * ccf.report_compression_ratio()) << std::endl;
  std::cout << "Block occupancy: " << block_occupancy << std::endl;
  if(print_load_histogram) ccf.print_bucket_and_block_load_histograms();
  
  // Deletions
  if(use_item_at_a_time_deletion){
    start = now();
    net_success = 0;
    for(uint64_t i = 0; i < total_items_to_insert; i++){
      bool success = ccf.delete_item(insert_items[i]);
      net_success += success;
      if(!success){ 
        std::cerr << "Failed to delete " << i << "th item\n";
      }
    }
    diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
    std::cout << "Millions of deletions per second: " << (total_items_to_insert / ((1e6)*diff.count())) << std::endl;
    std::cout << net_success << " of " << total_items_to_insert << 
      " were successful deletions.\n";
    std::cout << (100.0 * net_success) / total_items_to_insert << "% successfully deleted\n";
  }
  else{
    // Bulk deletions
    std::vector<bool> successes3(total_items_to_insert, false);
    start = now();
    ccf.delete_many(insert_items, successes3, total_items_to_insert);
    diff = std::chrono::duration_cast<std::chrono::duration<double>>(now() - start);
    std::cout << "Millions of deletions per second: " << (total_items_to_insert / ((1e6)*diff.count())) << std::endl;
    net_success = 0;
    for(uint64_t i = 0; i < total_items_to_insert; i++){
      net_success += successes3[i];
    }
    std::cout << net_success << " of " << total_items_to_insert << 
      " were successful deletions.\n";
 
    std::cout << (100.0 * net_success) / total_items_to_insert << "% successfully deleted\n";
    //std::cout << ccf.as_string() << std::endl;
  }
  ota_occupancy = ccf.report_ota_occupancy();
  std::cout << "OTA Occupancy Ratio: " << ota_occupancy << std::endl;
  std::cout << "Mean OTA Bits Set: " << ota_occupancy * ccf._ota_len_bits << 
    std::endl;
  std::cout << "Block occupancy: " << ccf.report_block_occupancy() << std::endl;
}

int main(int argc, char** argv){
  benchmark();
}
