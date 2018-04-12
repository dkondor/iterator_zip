# iterator_zip
Iterator adapter to "zip" together two iterators, iterating in parallel with the aim to be usable in STL algorithms.

The main motivation is to be able to use STL algorithms (e.g. std::sort) on pairs of containers (e.g. two std::vectors)
without having to create a container of std::pairs or similar. This is very similar to
[zip_iterator in Boost](https://www.boost.org/doc/libs/1_66_0/libs/iterator/doc/zip_iterator.html),
but with the addition that elements referenced by the iterator are modifiable, so algorithms that modify them will work.
This is still experimental / work in progress. Note that this is probably not possible while satisfying the requirements
of iterator concepts (mainly that reference types are actually a reference to value types), in practice it seems to work
and it does not involve undefined behavior AFAIK.

### Usage
This is a header-only library, include iterator_zip.h in your project and use the functionality. Note that it requires C++14.
Main usage is to create a "zipped" iterator:
```
auto zit = zi::make_zip_it(it1,it2);
```
After this, zit works mostly like a standard iterator supporting the operations it1 and it2 do. Dereferencing it will
result in a zi::refpair<T1,T2>, which is basically an std::pair<T1&,T2&> -- note that it is actually a pair of references,
so it can modify to elements pointed to by the original iterators (it1,it2). So e.g.
```
(*zit).first = somevalue;
```
will work. Also, there is a convenience function
```
zit.first()
```
which does the same (unfortunately, zit->first will not work as operator ->() requires an actual lvalue object to work on).

These iterators should work with STL algorithms, so the following code will sort a pair of vectors and erase duplicate elements:
```
std::vector<int> v1;
std::vector<size_t> v2;
... /* code to fill the vectors */
std::sort( zi::make_zip_it(v1.begin(),v2.begin()), zi::make_zip_it(v1.end(),v2.end()) );
auto it = std::unique( zi::make_zip_it(v1.begin(),v2.begin()), zi::make_zip_it(v1.end(),v2.end()) );
v1.erase( it.get_it1(), v1.end() );
v2.erase( it.get_it2(), v2.end() );
```

Also, there is a test program, which contains some examples for usage. It can be compiled and run:
```
g++ -o it_zip_test it_zip_test.cpp -g -std=gnu++14 -Wall
./it_zip_test
```

### What works
So far I've tested the following STL algorithms, using std::vectors and C-style arrays of PODs:
```
std::copy
std::mismatch
std::sort
std::make_heap
std::sort_heap
std::unique
std::unique_copy
```

### Caveats / gotchas
- zip_it::reference is NOT a reference to zip_it::value_type. This is non standard behavior, as it is
	only allowed for InputIterators, but not ForwardIterators or anything above that. But in this case,
	it is required so that zip_it::reference can contain references to the actual values, while
	zip_it::value_type can be safely used to copy (/ move) values. While this eliminates the main
	concern why ForwardIterators require to return actual references (so that they can be used to
	access / modify the referenced elements after incrementing the iterator, which the current setup
	allows), this is not what most people would expect; see the next point as well.
- auto x = *zit -- this will NOT copy the values, but create references; don't use this, explicitely declare
	x as zip_it::value_type (or std::pair<T1,T2>) or zip_it::reference (or the corresponding types obtained
	with std::iterator_traits) according to your intentions. This is the consequence of the previous and
	probably the largest potential issue that might result in unexpected behavior. When writing algorithms
	accepting generic iterators, it is advisable to use std::iterator_traits, but since using auto seems
	very convenient, this can be an issue.

	







