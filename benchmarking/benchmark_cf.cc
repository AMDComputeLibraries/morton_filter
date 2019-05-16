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
#include "benchmark_cf.h"

int main(int argc, char** argv){
  constexpr uint64_t fingerprint_len_bits = 12;
  constexpr uint64_t total_slots = 128 * 1024 * 1024;
  std::vector<double> lfs = {0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.35, 0.40, 
    0.45, 0.50, 0.55, 0.60, 0.65, 0.70, 0.75, 0.80, 0.85, 0.90, 0.95};

  
  uint64_t gradations = 1.0; // 0.0 to 1.0 inclusive in gaps of 1.0/gradations
  constexpr uint64_t lookup_trials = 5; // Multiple trials also achieved with benchmark.sh
  constexpr uint64_t modify_trials = lookup_trials;

  double trial_outputs[lookup_trials];

  // Lookups
  std::vector<double> lookup_throughputs(lfs.size(), 0.0);
  std::cout << "FILTER  LOAD  OPERATION  THROUGHPUT %%TRUE_POSITIVE\n";
  for(uint64_t g = 0; g <= gradations; g++){
    std::vector<double> lookup_throughputs(lfs.size(), 0.0);
    double true_positive = (1.0 * g) / gradations;
    for(uint64_t i = 0; i < lfs.size(); i++){
      double lf = lfs[i];
      for(uint64_t t = 0; t < lookup_trials; t++){
        trial_outputs[t] = benchmark_lookups<fingerprint_len_bits>(
          total_slots, lf, true_positive);
        lookup_throughputs[i] += trial_outputs[t];
      }
      lookup_throughputs[i] /= lookup_trials;
      std::cout << "CF  " << lf << " LOOKUP " << lookup_throughputs[i] << " " << true_positive;
      for(uint64_t t = 0; t < lookup_trials; t++){
        std::cout << " " << trial_outputs[t];
      }
      std::cout << std::endl;
      lookup_throughputs[i] = 0;
    }
  }
  
  
  // Insertions
  std::vector<double> insert_throughputs(lfs.size(), 0.0);
  std::cout << "FILTER  LOAD  OPERATION  THROUGHPUT\n";
  for(uint64_t i = 0; i < lfs.size(); i++){
    double lf = lfs[i];
    for(uint64_t t = 0; t < modify_trials; t++){
      trial_outputs[t] = benchmark_insertions<fingerprint_len_bits>(
        total_slots, lf);
      insert_throughputs[i] += trial_outputs[t];
    }
    insert_throughputs[i] /= modify_trials;
    std::cout << "CF  " << lf << " INSERT " << insert_throughputs[i];
    for(uint64_t t = 0; t < modify_trials; t++){
        std::cout << " " << trial_outputs[t];
    }
    std::cout << std::endl;
  }

  // Deletions
  std::vector<double> delete_throughputs(lfs.size());
  std::cout << "FILTER  LOAD  OPERATION  THROUGHPUT\n";
  for(uint64_t i = 0; i < lfs.size(); i++){
    double lf = lfs[i];
    for(uint64_t t = 0; t < modify_trials; t++){
      trial_outputs[t] = benchmark_deletions<fingerprint_len_bits>(
        total_slots, lf);
      delete_throughputs[i] += trial_outputs[t];
    }
    delete_throughputs[i] /= modify_trials;
    std::cout << "CF  " << lf << " DELETE " << delete_throughputs[i];
    for(uint64_t t = 0; t < modify_trials; t++){
        std::cout << " " << trial_outputs[t];
    }
    std::cout << std::endl;
  }

  return 0;
}
