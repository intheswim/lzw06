### Background 

<pre>

This is classic implementation of LZW  algorithm based on  Mark Nelson's book on
data compression from 1995. It  uses fixed  12-bit dictionary codes.  Originally
written as a  work and  study  related  project  in 1996, it has been updated to
support  more  arguments  and be  a single executable as  opposed  to individual 
packer and un-packer programs. I also  kept it in  pure ANSI C (see Makefile) it 
was originally written in. 

The program has  been tested on Ubuntu 18.04 (with gcc and clang) and on Windows 
10 (with Visual Studio 2019).

This is mostly  a  reference  for variable-width  codes implementations of LZW I
plan to publish next  and for my own variant of LZMW I came up  around 1996, not 
quite being aware then that it has already been suggested. 

</pre>

### Usage and Examples 

<pre> 

Running  make  produces  (1) executable `lzw06` and  (2) library `liblzw06` with 
two exported functions Compress and Decompress (see export.h).   

Type `./lzw06` to see all syntax options. 

Examples: 

`./lzw06 -p sample.txt sample.lzw`  (pack sample.txt)

`./lzw06 -u sample.lzw sample_copy.txt` (unpackd sample.lzw)

`./lzw06 -t sample.txt` (test compression/decompression)

`./lzw06 -large 50` (test synthetic data)

</pre>

### Limitations

<pre>

1. It supports input files up to 2GB in size. 

2. It is currently supported on little-endian machines only. 

Both of these limitations can be easily lifted. 

</pre>

### Possible improvements 

<pre>

Both are related to limitations described above. 

1. Add big endian support.  Currently  we  are relying on "little  endian"  byte
order when  writing 12-bit codes and reading them. Search for "little endian" in
code. 

2. Input file size  limit can be fixed by replacing  4-byte file  size header in
compressed file with 48- or 64-bit value. 

</pre>

### License 

[MIT](https://choosealicense.com/licenses/mit/) 
