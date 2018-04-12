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
	
	std::mt19937 rg;
	std::uniform_int_distribution<int> rd(1,mod);
	for(size_t i=0;i<size;i++) {
		v1[i] = rd(rg);
		v2[i] = rd(rg);
	}
	
	/* create copies for different algorithms */
	std::vector<int> c1 = v1; /* note: v1 and v2 are not modified during the tests */
	std::vector<int> c2 = v2; /* so it is easy to add more tests at the end */
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
	
	/* test std::sort using zip_it */
	c1 = v1;
	c2 = v2;
	std::sort( zi::make_zip_it(c1.begin(),c2.begin()), zi::make_zip_it(c1.end(),c2.end()) );
	
	/* copy to paired version and compare */
	for(size_t i = 0;i<size;i++) p2[i] = std::make_pair(c1[i],c2[i]);
	if(!cmp_vec(p1,p2)) {
		fprintf(stderr,"Error comparing sorted vectors after std::sort!\n");
		return 1;
	}
	
	
	/* test std::make_heap and std::sort_heap */
	c1 = v1;
	c2 = v2;
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
	
	
	fprintf(stdout,"All tests OK\n");
	
	
	return 0;
}


