/*
 * it_zip_test.cpp -- test behavior of zipped iterators
 * 	create random vectors, perform std::sort, std::sort_heap and std::unique,
 * 
 * Copyright 2018 Daniel Kondor <kondor.dani@gmail.com>
 * 
 */


#include <stdio.h>
#include <random>
#include <vector>
#include <algorithm>
#include "iterator_zip.h"


/* function to compare two vectors for checking results */
template<class T>
bool cmp_vec(const std::vector<T>& v1, const std::vector<T>& v2) {
	if(v1.size() != v2.size()) {
		fprintf(stderr,"cmp_vec: size differs!\n");
		return false;
	}
	for(size_t i=0;i<v1.size();i++) if(v1[i] != v2[i]) {
		fprintf(stderr,"cmp_vec: element %lu differs!\n",i);
		return false;
	}
	return true;
}

/* compare iterator to std::pair or similar -- mainly to test
 * operator ->() in zip_it */
template<class it1, class it2>
bool cmp_it_pair(it1 begin1, it1 end1, it2 begin2) {
	for(size_t i = 0; begin1 != end1; ++begin1, ++begin2, i++) {
		if(begin1->first != begin2->first || begin1->second != begin2->second) {
			fprintf(stderr,"cmp_it_pair. element %lu differs!\n",i);
			return false;
		}
	}
	return true;
}


/* custom quicksort
 * recursive function using [begin,end] range */
template<class It>
void quicksort_r(It begin, It end) {
	using std::swap;
	It begin0 = begin;
	ssize_t dist = end - begin;
	if(dist == 0) return;
	if(dist == 1) {
		if(*end < *begin) swap(*end,*begin);
		return;
	}
	ssize_t pivot = dist / 2;
	swap(begin[pivot],*end);
	It p = begin;
	for(;begin<end;begin++) {
		if(*begin < *end) {
			swap(*begin,*p);
			p++;
		}
	}
	swap(*end,*p);
	if(p > begin0) quicksort_r(begin0,p-1);
	if(end > p) quicksort_r(p+1,end);
}

/* interface function using [begin,end) range */
template<class It>
void quicksort(It begin, It end) {
	if(end > begin) quicksort_r(begin,end-1);
}



int main(int argc, char **argv)
{
	const size_t size = 1000;
	const int mod = 25;
	
	
	/* create two vectors with random elements */
	std::vector<int> v1(size);
	std::vector<int> v2(size);
	std::vector<int> v3(size);
	
	std::mt19937 rg;
	std::uniform_int_distribution<int> rd(1,mod);
	for(size_t i=0;i<size;i++) {
		v1[i] = rd(rg);
		v2[i] = rd(rg);
		v3[i] = rd(rg);
	}
	
	/* create copies for different algorithms */
	std::vector<int> c1 = v1; /* note: v1 and v2 are not modified during the tests */
	std::vector<int> c2 = v2; /* so it is easy to add more tests at the end */
	std::vector<int> c3 = v3;
	std::vector<std::pair<int,int> > p1(size);
	std::vector<std::pair<int,int> > p2;
	
	for(size_t i = 0;i<size;i++) p1[i] = std::make_pair(v1[i],v2[i]);
	
	/* zip iterator uses
	 * 1. copy std::pairs, should be trivial case */
	{
		auto it1 = zi::make_zip_it(c1.begin(),c2.begin());
		auto it2 = zi::make_zip_it(c1.end(), c2.end());
		for(;it1 != it2;++it1) p2.push_back(*it1);
	}
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error creating pair vectors!\n");
		return 1;
	}
	/* same, but use std::copy() */
	{
		auto it3 = std::copy( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()), p2.begin() );
		if(it3 != p2.end()) {
			fprintf(stderr,"Error using std::copy!\n");
			return 1;
		}
	}
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error after std::copy!\n");
		return 1;
	}
	/* test std::mismatch */
	{
		auto itpair1 = std::mismatch( p1.begin(), p1.end(), zi::make_zip_it(c1.begin(),c2.begin()) );
		if(itpair1.first != p1.end() || itpair1.second != zi::make_zip_it(c1.end(),c2.end()) ) {
			fprintf(stderr,"Error using std::mismatch!\n");
			return 1;
		}
		auto itpair2 = std::mismatch( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()), p1.begin() );
		if(itpair2.second != p1.end() || itpair2.first != zi::make_zip_it(c1.end(),c2.end()) ) {
			fprintf(stderr,"Error using std::mismatch!\n");
			return 1;
		}
	}
	/* test comparing using operator ->() */
	if(!cmp_it_pair( p1.begin(), p1.end(), zi::make_zip_it(c1.begin(),c2.begin()) )) {
		fprintf(stderr,"Error comparing using operator ->()!\n");
		return 1;
	}
	/* test iterating over with range-for syntax */
	{
		size_t i = 0;
		for( auto x : zi::make_const_zip_range(c1,c2) ) {
			/* note: x is std::pair<const int&, const int&> */
			if(x.first != p1[i].first || x.second != p1[i].second) {
				fprintf(stderr,"Error comparing using range-for!\n");
				return 1;
			}
			i++;
		}
	}
	
	
	/* sort vectors */
	std::sort(p1.begin(),p1.end());
	/* test custom quicksort first */
	quicksort(p2.begin(),p2.end());
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error comparing sorted vector pairs!\n");
		return 1;
	}
	
	
	/* custom quicksort with zip_it */
	quicksort(zi::make_zip_it(c1.begin(),c2.begin()),zi::make_zip_it(c1.end(),c2.end()));
	
	/* copy to paired version and compare */
	for(size_t i = 0;i<size;i++) p2[i] = std::make_pair(c1[i],c2[i]);
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error comparing sorted vectors after quicksort()!\n");
		return 1;
	}
	
	
	/* create a nested zip_iterators, use those to copy vector elements */
	{
		auto it1 = zi::make_zip_it( zi::make_zip_it(v1.begin(),v2.begin()), zi::make_zip_it(c1.rbegin(),c2.rbegin()) );
		auto it2 = zi::make_zip_it( zi::make_zip_it(v1.end(),v2.end()), zi::make_zip_it(c1.rend(),c2.rend()) );
		auto it3 = zi::make_zip_it( p1.begin(), p2.begin() );
		for(; it1 != it2; ++it1, ++it3) {
			it3->first = it1->first;
			it3->second = it1->second;
			/* or potentially use *it3 = *it1; ?? */
		}
	}
	{
		/* test using const zip_iterators */
		auto it1 = zi::make_zip_it( zi::make_zip_it(v1.cbegin(),v2.cbegin()), zi::make_zip_it(c1.crbegin(),c2.crbegin()) );
		auto it2 = zi::make_zip_it( zi::make_zip_it(v1.cend(),v2.cend()), zi::make_zip_it(c1.crend(),c2.crend()) );
		auto it3 = zi::make_zip_it( p1.cbegin(), p2.cbegin() );
		for(; it1 != it2; ++it1, ++it3) {
			if( !(it1->first == it3->first && it1->second == it3->second)  ) {
				fprintf(stderr,"Error comparing using nested iterators!\n");
				return 1;
			}
		}
		
		/* test the result using indices */
		/* p1 should be the same as {v1,v2}, while p2 should be the reverse of {c1,c2} */
		for(size_t i=0;i<size;i++) {
			if(p1[i].first != v1[i] || p1[i].second != v2[i] ||
				p2[i].first != c1[size-i-1] || p2[i].second != c2[size-i-1]) {
					fprintf(stderr,"Error comparing after copying with nested iterators!\n");
					return 1;
			}
		}
	}
	
	/* test modifying using operator ->() */
	{
		size_t i = 0;
		for( auto it0 = zi::make_zip_it(c1.begin(),c2.begin()); it0 != zi::make_zip_it(c1.end(),c2.end()); ++it0 ) {
			it0->first = v1[i];
			it0->second = v2[i];
			i++;
		}
		if( ! (cmp_vec(c1,v1) && cmp_vec(c2,v2)) ) {
			fprintf(stderr,"Error comparing after assigning using operator -> ()!\n");
			return 1;
		}
	}
	/* test std::sort using zip_it */
	std::sort( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()) );
	std::sort(p1.begin(),p1.end()); /* note: p1 was modified before as well */
	
	/* copy to paired version and compare */
	for(size_t i = 0;i<size;i++) p2[i] = std::make_pair(c1[i],c2[i]);
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error comparing sorted vectors after std::sort!\n");
		return 1;
	}
	
	
	/* test modifying using range-for */
	{
		size_t i = 0;
		for( auto x : zi::make_zip_range(c1,c2) ) {
			x.first = v1[i];
			x.second = v2[i];
			i++;
		}
		if( ! (cmp_vec(c1,v1) && cmp_vec(c2,v2)) ) {
			fprintf(stderr,"Error comparing after assigning using range for!\n");
			return 1;
		}
	}
	
	/* test std::make_heap and std::sort_heap */
	std::make_heap( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()) );
	std::sort_heap( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()) );
	/* copy to paired version and compare */
	for(size_t i = 0;i<size;i++) p2[i] = std::make_pair(c1[i],c2[i]);
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error comparing sorted vectors after std::sort!\n");
		return 1;
	}
	
	/* test std::unique and std::unique_copy on the sorted arrays */
	auto it1 = std::unique(p1.begin(),p1.end());
	p1.erase(it1,p1.end());
	
	auto it2 = std::unique_copy( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()),
		p2.begin() );
	p2.erase(it2,p2.end());
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error comparing sorted vectors after std::unique_copy!\n");
		return 1;
	}
	
	
	auto it3 = std::unique( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()) );
	c1.erase(it3.get_it1(),c1.end());
	c2.erase(it3.get_it2(),c2.end());
	if(p2.size() != c1.size() || c1.size() != c2.size()) {
		fprintf(stderr,"Error after std::unique!\n");
		return 1;
	}
	it2 = std::copy( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()), p2.begin() );
	if(it2 != p2.end()) {
		fprintf(stderr,"Error after std::copy!\n");
		return 1;
	}
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error comparing sorted vectors after separate std::unique and std::copy!\n");
		return 1;
	}
	
	
	/* test sorting when comparing only the keys */
	/* re-create the original vector in p1 as well */
	c1 = v1;
	c2 = v2;
	p1.resize(c1.size());
	p2.resize(c1.size());
	/* copy the original to p1 -- this was tested previously, so it should work */
	std::copy( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()), p1.begin() );
	/* sort p1 using only the first elements */
	std::sort(p1.begin(),p1.end(),[](const auto& x, const auto& y){return x.first < y.first;});
	/* sort using zip_it and the supplied comparison */
	std::sort( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()),
		zi::make_cmp_less_first(c1,c2) );
	/* note: order of the second elements could differ, but since exactly the same
	 * algorithm was run in both case, both should be the same */
	std::copy( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()), p2.begin() );
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error comparing vectors after sorting using the first element only!\n");
		return 1;
	}
	
	/* test std::unique on these as well */
	it1 = std::unique(p1.begin(),p1.end(), [](const auto& x, const auto& y){return x.first == y.first;});
	p1.erase(it1,p1.end());
	it2 = std::unique_copy( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()),
		p2.begin(), zi::make_cmp_eq_first(c1,c2) );
	p2.erase(it2,p2.end());
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error comparing vectors after std::unique_copy using the first element only!\n");
		return 1;
	}
	
	
	/* test sorting three vectors together 
	 * note that std::sort will not work with the default comparison operators
	 * but it will work with the lambda below */
	{
		c1 = v1; c2 = v2; c3 = v3;
		std::vector<std::pair<std::pair<int,int>,int> > p3(size);
		for(size_t i=0;i<size;i++) 
			p3[i] = std::make_pair(std::make_pair(v1[i],v2[i]),v3[i]);
		
		std::sort(p3.begin(),p3.end());
		std::sort( zi::make_zip_it( zi::make_zip_it(c1.begin(),c2.begin()), c3.begin() ),
			zi::make_zip_it( zi::make_zip_it(c1.end(),c2.end()), c3.end() ), 
			[](const auto& x, const auto& y) { return x.first < y.first || (x.first == y.first && x.second < y.second); } );
		
		for(size_t i=0;i<size;i++) {
			if(p3[i].first.first != c1[i] || p3[i].first.second != c2[i] || p3[i].second != c3[i]) {
				fprintf(stderr,"Error comparing after sorting three vectors together; mismatch at element %lu!\n",i);
				return 1;
			}
		}
		
		auto itpair2 = std::mismatch(p3.begin(), p3.end(), zi::make_zip_it( zi::make_zip_it(c1.begin(),c2.begin()), c3.begin() ),
			[](const auto& x, const auto& y){ return x.first == y.first && x.second == y.second; } );
		if(itpair2.first != p3.end() || itpair2.second != zi::make_zip_it( zi::make_zip_it(c1.end(),c2.end()), c3.end() ) ) {
			fprintf(stderr,"Error comparing with std::mismatch after sorting three vectors together at element %lu!\n",
				itpair2.first - p3.begin());
			return 1;
		}
	}
	
	
	fprintf(stdout,"All tests OK\n");
	
	
	return 0;
}


