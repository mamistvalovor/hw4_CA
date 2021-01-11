/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <vector>
#include <stdio.h>

using namespace std;


class inst_properties {
public:
	int count_ = 0;
	Instruction inst_;
};

class multithread {
public:

	int thread_num_ = 0;

	vector< vector< inst_properties > > vec_threads_;
	vector<tcontext*> vec_regs_;




};

void CORE_BlockedMT() {



}

void CORE_FinegrainedMT() {
}

double CORE_BlockedMT_CPI() {
	return 0;
}

double CORE_FinegrainedMT_CPI() {
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
