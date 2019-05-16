#!/usr/bin/env python2

"""
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
"""

"""
Author: Alex Breslow
Date: Jan. 9, 2018
Description: This file contains a utility for computing the block compression 
 ratio (the parameter C in the VLDB paper) given other target parameters 
 such as the buckets per block and the slots per bucket.

Usage: ./calculate_compression_ratio.py
"""

import math
import itertools as it

def int_ceil(n):
  return int(math.ceil(n))

def log2(n):
  return math.log(n, 2)

def log2ceil(n):
  return int(math.ceil(log2(n)))

def calculate_slot_compression_ratio_and_other_params(buckets_per_block, 
  slots_per_bucket, fingerprint_len_bits, target_ota_len, block_size_bits = 512):
  fullness_counter_width = log2ceil(slots_per_bucket + 1)
  fullness_counter_array_len = fullness_counter_width * buckets_per_block
  available_bits = block_size_bits - target_ota_len - fullness_counter_array_len
  max_fingerprints_per_block = available_bits / fingerprint_len_bits
  # Now figure out how many actual bits we have for the OTA by padding the 
  # block full the rest of the way
  available_bits_for_ota = block_size_bits - fullness_counter_array_len - \
    max_fingerprints_per_block * fingerprint_len_bits
  slot_compression_ratio = (max_fingerprints_per_block)\
     / float(buckets_per_block * slots_per_bucket)
  return {"slot_compression_ratio" : slot_compression_ratio, 
          "max_fingerprints_per_block" : max_fingerprints_per_block,
          "fullness_counter_width" : fullness_counter_width,
          "fullness_counter_array_len" : fullness_counter_array_len,
          "target_ota_len" : target_ota_len,
          "available_bits_for_ota" : available_bits_for_ota}


if __name__ == "__main__":
  buckets_per_block = 64
  slots_per_bucket = 2
  fingerprint_len_bits = 8
  target_ota_len = 16#buckets_per_block / 2 # Intentional integer division
  d = calculate_slot_compression_ratio_and_other_params(buckets_per_block, slots_per_bucket,
    fingerprint_len_bits, target_ota_len)
  for k in d:
    print(k, d[k])
