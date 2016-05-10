= Bloomap

A bloom filter variation, aiming to provide more bitmap-like features, while
keeping most of the bloom filter advantages. Namely:

 - `INSERT` close to *O(1)* (currently aiming for *O(log(n/m))* on average)
 - `QUERY` in *O(1)*
 - `INTERSECT` in *O(m)*
 - `UNION` in *O(m)*
 - `ENUMERATE` in *O(N)*
 - `PRUNE` in *O(N)*
 - Small memory footprint.

Where *n* is the maximum number of elements in filter, *m* is the size of
filter, and *N* is the number of elements ever inserted into a family.

== What Bloomap is not?

 - Bloomap isn't a replacement for bitmaps, vectors, linked lists and so on.
 - Bloomap isn't efficient on itself. It gets interesting if you have one family
   with many bloomaps, and do a lot of set operations over them. How large you
   ask? Nobody knows yet!

== Classes and interface

=== BloomFilter

A low-level implementation of bloom filter itself. This is the place to make
everything run faster, as its currently implemented using standard STL data
structures. This isn't really good for perfomance, but it's great for
development.

=== Bloomap

A bloomap implementation. It uses BloomFilter as it's backbone, but adds all the
extra functionality.

=== BloomapFamily

A family of Bloomaps, sharing the same parameters and hash functions. Set
operations can only be executed on sets in the same family.

== Debugging and benchmarking

There are several way to debug and benchmark the bloomaps and families. Here are
some basics. If you are interested, take a closer look into the headers.

 - `NDEBUG` macro. Classic, used only for asserts.
 - `DEBUG_STATS` macro. This enables some public counters and helper functinons,
   intended to gather statistics about the maps (currently only number of false
   positives and queries). To do this, bloomaps will actually carry the
   real list of items contained in them, in a `std::unordered_set`.

== TODO and ideas

 - *(Free idea)* Extensible and counting variation. This shouldn't be too difficult, but I
   have no plans on doing it myself. Pull requests are welcome!
 - *(Todo)* Take a closer look at the hashing function. Murmur was chosen pretty
   much randomly, and modified to work only on integers. It seems to perfom
   well, but no guarantees!
 - *(Todo)* Add templates. Currently only integers can be hashed, and that's
   fine for testing and even my intended aplication, but probably not for much
   else.
