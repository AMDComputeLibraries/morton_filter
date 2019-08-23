Morton Filter Description
================

This repo contains an implementation of a 
**Morton filter** (https://www.vldb.org/pvldb/vol11/p1041-breslow.pdf), 
a new approximate set membership data structure (ASMDS) developed at 
Advanced Micro Devices, Inc. in the AMD Research group by 
Alex Breslow and Nuwan Jayasena.  

A Morton filter is a modified 
cuckoo filter (see Fan et al. https://dl.acm.org/citation.cfm?id=2674005.2674994) 
that is optimized for bandwidth-constrained systems.  Morton filters use additional 
computation in order to reduce their off-chip memory traffic.  Thus, they particularly 
excel when the latency of this additional computation can be hidden behind long-latency 
loads from DRAM (e.g., 100 ns).

Like a cuckoo filter, a Morton filter supports 
insertions, deletions, and lookup operations.  It additionally adds 
high-throughput self-resizing, a feature of quotient filters, which allows a 
Morton filter to 
increase its capacity solely by leveraging its internal 
representation (albeit with an increase in the false positive rate after 
multiple resizings).  This capability is in contrast to existing 
vanilla cuckoo filter implementations, which are static and thus require 
using a backing data structure that contains the full set of items to resize the 
filter.

Morton filters can also be configured to use less memory than a cuckoo filter 
for the same error rate while simultaneously delivering insertion, deletion, 
and lookup throughputs that are, respectively, up to 15.5x, 
1.3x, and 2.5x higher than a cuckoo filter.  Morton filters 
in contrast to vanilla cuckoo filters do not require a power of two number of 
buckets but rather only a number 
that is a multiple of two.  They also use fewer bits per item
than a Bloom filter when the target false positive rate is less than around 
1% to 3%.

For details, ***please read our 
VLDB paper: https://www.vldb.org/pvldb/vol11/p1041-breslow.pdf.***  Note 
that ***this code has not been hardened for industrial use but is an 
academic-style prototype***.  

A forthcoming extended version that describes additional features like the filter self-resizing 
operation (released here in this repository) has been accepted to the VLDB Journal in a special issue with the best papers from VLDB'18.  An official preprint is available from Springer's website (https://link.springer.com/article/10.1007/s00778-019-00561-0).  A 
subsequent paper that extends self-resizing to cuckoo filters will also shortly be submitted for peer review.

Using the Code
=================
***When using the code, you are required to comply with the terms of the license (LICENSE.txt).***  Please cite our paper 
if you use the code
base.  The Very Large Data Bases Endowment has requested that the ***paper be cited in this fashion:***

Alex D. Breslow and Nuwan S. Jayasena. Morton Filters: Faster,
Space-Efficient Cuckoo Filters via Biasing, Compression, and Decoupled
Logical Sparsity. *PVLDB*, 11(9): 1041-1055, 2018.
DOI: https://doi.org/10.14778/3213880.3213884

ACM provides the following bibtex, which is slightly different:
```bibtex
@article{Breslow:2018:MFF:3213880.3232248,
 author = {Breslow, Alex D. and Jayasena, Nuwan S.},
 title = {Morton Filters: Faster, Space-efficient Cuckoo Filters via Biasing, Compression, and Decoupled Logical Sparsity},
 journal = {Proc. VLDB Endow.},
 issue_date = {May 2018},
 volume = {11},
 number = {9},
 month = may,
 year = {2018},
 issn = {2150-8097},
 pages = {1041--1055},
 numpages = {15},
 url = {https://doi.org/10.14778/3213880.3213884},
 doi = {10.14778/3213880.3213884},
 acmid = {3232248},
 publisher = {VLDB Endowment},
} 
```


***Examples of use that merits citing our work are listed below.***  Note that these examples are not exhaustive.

1. You use part of the code in a publication or subsequent artefact (e.g., code or text)

2. You create a new artefact (e.g., code or text) using ideas or code that appears in our VLDB'18 paper or in this repository

3. You're feeling generous of heart and want to spread the word about Morton filters (thank you)

Key files
===================
**benchmark_mf.cc** and **benchmark_mf.h** contain code for benchmarking the Morton filter implementation on your system.

**benchmark.cc** contains some code for benchmarking an MF.

**compressed_cuckoo_filter.h** contains the bulk of the Morton Filter class implementation (CompressedCuckooFilter).  You may want to include this directly if you are an advanced user. 

**morton_filter.h** is the file you want to include in your code if you want to use preconfigured defaults and a simpler, saner interface.

**compressed_cuckoo_config.h** contains some additional configuration parameters and enum definitions with comments.

**vector_types.h** defines certain array types and the batch size.

How to Use
===================
The code is implemented as a template header library, which means that you just need to point your C++ preprocessor to the Morton filter top-level directory
and include *morton_filter.h*.  **At present, we only support GNU's C++ compiler (g++).**  Below is an example of how to use our Makefile 
to compile the example programs on Linux systems using g++:
```bash
cd morton_filter/benchmarking
make 
```

Please note that this will also clone Fan et al.'s cuckoo filter implementation (https://github.com/efficient/cuckoofilter) and patch it to add another constructor that enables comparative benchmarking 
with Morton filters.  Fan et al.'s code will be cloned to ./benchmarking/cuckoofilter.  Please cite their code and paper if you end up using it.  Instructions for how to cite it are at the preceding link.

For compiling the code, we recommend using g++ compiler version 5.4.  Later versions of GCC (e.g., 8.x) as of March, 2019 still appear to have a regression that 
reduces the performance of the compiled code.

The implementation makes heavy use of templating for compile-time configuration of the Morton filter.  To maintain sanity, we have provided 
some sample Morton filter configurations in *morton_sample_configs.h* that you can use out of the box.  

For example, the following code will construct a Morton filter with 3-slot buckets and 8-bit fingerprints that matches the primary configuration used in our VLDB'18 paper:
```C++ 
#include "morton_sample_configs.h"
using namespace CompressedCuckoo; // Morton filter namespace

int main(int argc, char** argv){
  // Construct a Morton filter with at least 128 * 1024 * 1024 slots for fingerprints
  Morton3_8 mf(128 * 1024 * 1024);
  return 0;
}
``` 

Simple use is provided in this README.  For more advanced use, please check the comments in the code and the FAQ section below.

Simple Use
-------------------
For performance reasons, the main interface for this implementation uses batched lookups, insertions, and deletions rather than one-at-a-time data 
processing.  Hence, the interface is different than what one might typically expect.  The API includes the following calls:
```C++
bool insert_many(const std::vector<keys_t>& keys, std::vector<bool>& status, const uint64_t num_keys); // Calculate the keys' fingerprints and insert them into the filter, populates status vector with success/failure
void likely_contains_many(const std::vector<keys_t>& keys, std::vector<bool>& status, const uint64_t num_keys); // Checks for the existence of "keys" in the filter, populates status vector with success/failure
void delete_many(const std::vector<keys_t>& keys, std::vector<bool>& status, const uint64_t num_keys); // Deletes the fingerprints corresponding to "keys" in the filter, populates a status vector with success/failure
```

Each of these APIs execute a bulk operation using an input vector of keys whose membership we wish to, respectively, insert, query, or delete from the filter.  The success or failure of each operation is 
stored in the bit vector status, which can be subsequently queried.  At present, keys_t is a uint64_t.  We may change the interfaces to support templated forward iterators so 
that the APIs are not specific to instances of std::vector.

**Note that you should only call delete_many on items whose fingerprints are actually in the filter.  Otherwise, you can expect false negatives (i.e., the filter may incorrectly
report that an item e is not an element of the set because an earlier delete operation for an item not encoded by the filter caused e's fingerprint to be deleted).**

We implement additional methods for item-at-a-time data processing, but we discourage users from using these because they are typically much slower than the bulk data processing APIs that we list above, at least for large filters.

Please see benchmark.cc and benchmark_mf.h for examples of how to use the APIs.

Running the Sample Programs
---------------------

The following programs should run out of the box like so:

```bash
./benchmark
./benchmark_mf
./benchmark_cf
./benchmark_ss_cf
```


Known Issues
===================
The batching interface currently only works correctly when the input is a multiple of the batch size (presently 128).  We aim to fix 
this limitation sometime soon.

There is a performance regression in deletion throughput due to a correctness bug that I fixed in the batched deletion algorithm.  Peak deletion throughput drops from about 38 MOPS down to 28 MOPS for the 3-slot bucket configuration used in the VLDB'18 paper.

Reporting Use
===================
We are interested in learning how Morton filters are being used in the wild.  If you find this code helpful, please let us 
know your use case.  Please also let us know benchmarking numbers for the implementation on your system and if there are 
any performance anomalies.

Contributing
===================
Contributions are welcome as pull requests.  We may enforce a particular style at a later date.

Author and Correspondence
===================
Alex D. Breslow 

Please raise an issue on Github if you have issues with using the code or suggestions.

Frequently Asked Questions
===================
**TBD**
