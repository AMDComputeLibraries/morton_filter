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
#ifndef _BENCHMARK_UTIL_H
#define _BENCHMARK_UTIL_H

#include <chrono>
#include <cstdint>
#include <vector>
#include <random>
#include <sstream>

namespace Benchmark{

  void set_region_name(std::string& region_name, const std::string& base_name, 
    double load_factor, double true_positive_ratio){
    std::stringstream ss;
    ss << base_name << "_LF" << load_factor << "_TPR" << true_positive_ratio;
    region_name = ss.str();
  }

  uint64_t to_multiple_of_batch(uint64_t number, uint64_t batch_size){
    return ((number + batch_size - 1) / batch_size) * batch_size;
  }

  // TODO: Use and begin and end iterators?  There may be a good reason why
  // didn't do that, but I can't recall at present.
  template <class T>
  void populate_with_random_numbers(std::vector<T>& items){
    std::default_random_engine rn_gen; // Random number generator
    std::uniform_int_distribution<T> distribution(static_cast<T>(0), 
      ~static_cast<T>(0));
    for(uint64_t i = 0; i < items.capacity(); i++){
      items[i] = distribution(rn_gen);
    }
  }

  // Populates two vectors
  // Allocation of the vectors needs to be done for a specific capacity before 
  // they are passed in.

  // One contains items to insert into a filter
  // The other contains items to probe the filter with (could also be deletions)
  // Overlap specifies what percentage of probe items are elements in 
  // insert items.  1.0 is 100% overlap 0.0 is 0% overlap.

  // duplicates_permitted controls whether the probe_items can have duplicates
  // This is necessary for ensuring that when generating items to delete

  // For deletions, overlap should be 1.0 and 
  // duplicates_permitted should be false
  template <class T>
  void populate_with_random_numbers(std::vector<T>& insert_items, 
    std::vector<T>& probe_items, double overlap, bool duplicates_permitted){
    std::default_random_engine rn_gen;
    // Select numbers from a space of 2 ** 34 when uint64_t or bigger
    uint64_t range_max = std::min<T>((1ULL << 34ULL) - 1, ~static_cast<T>(0));
    std::vector<bool> insert_items_bit_vector(range_max + 1, 0);
    
    // Only track duplicates if duplicated_permitted is false
    uint64_t probe_items_bit_vector_capacity = duplicates_permitted ? 0 : 
      range_max + 1;
    std::vector<bool> probe_items_bit_vector(probe_items_bit_vector_capacity, 0);

    std::uniform_int_distribution<T> distribution(static_cast<T>(0), range_max);
    
    // Generates a random index into insert_items when called on rn_gen
    std::uniform_int_distribution<uint64_t> map_distribution(0ULL, insert_items.capacity() - 1);

    for(uint64_t i = 0; i < insert_items.capacity(); i++){
      insert_items[i] = distribution(rn_gen);
      insert_items_bit_vector[insert_items[i]] = true; 
    }
    for(uint64_t i = 0; i < probe_items.capacity(); i++){
      if(distribution(rn_gen) < overlap * (range_max + 1)){
        T candidate_probe_item;
        do{
          candidate_probe_item = insert_items[map_distribution(rn_gen)];
        // Continue looping if the candidate item is a duplicate and duplicates 
        // are not permitted
        } while(!duplicates_permitted && 
          probe_items_bit_vector[candidate_probe_item]);
        probe_items[i] = candidate_probe_item;
        if(!duplicates_permitted){
          probe_items_bit_vector[candidate_probe_item] = true;
        }
      }
      else{
        T item;
        bool item_in_insert_items;
        do{
          item = distribution(rn_gen);
          item_in_insert_items = insert_items_bit_vector[item]; 
        } while (item_in_insert_items);
        probe_items[i] = item; 
      }
    }
  }

  // Generates numbers in the range [range_start, range_stop] with uniform
  // randomness
  template <class T>
  struct RN_Gen{
    std::default_random_engine m_rn_gen;
    std::uniform_int_distribution<T> m_distribution;
    constexpr RN_Gen(T range_start, T range_stop) : m_rn_gen(), 
      m_distribution(range_start, range_stop){}
    inline T next(){ return m_distribution(m_rn_gen); }
  };
  
  typedef std::chrono::high_resolution_clock::time_point time_point;
  __inline time_point now(){
    return std::chrono::high_resolution_clock::now();
  }



}
#endif
