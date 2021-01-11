/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <vector>
#include <stdio.h>

using namespace std;

multithread* init_multithread(int, int, int, int);
void init_regs(multithread);
void init_threads(multithread);

class inst_properties {
public:
	int count_ = 0;
	Instruction inst_;
};

class multithread {
public:
	int overhead = 0;
	int thread_num_ = 0;
	int halts_num = 0;

	int store_latency = 0;
	int load_latency = 0;

	int cyc_count_ = 0;
	bool *is_thread_valid;
	vector< vector< inst_properties* > > vec_threads_;
	vector<tcontext*> vec_regs_;
};

void CORE_BlockedMT() {

	multithread* MT_block = init_multithread(SIM_GetSwitchCycles(), SIM_GetThreadsNum(),
													SIM_GetLoadLat(), SIM_GetStoreLat());
	if (!MT_block)
		return;
	if (MT_block->thread_num_ == 0)
		return;
	int lu_thread = 0;

	while (MT_block->halts_num < MT_block->thread_num_) {
		

		for (int tid = 0; tid < MT_block->thread_num_; tid++) {
			bool is_halt = (MT_block->vec_threads_[tid][0]->inst_.opcode == CMD_HALT) ? true : false;


			if (lu_thread == tid) {
				if ((MT_block->vec_threads_[tid][0]->inst_.opcode < CMD_LOAD)){
					execute_command(MT_block->vec_threads_[tid][0]->inst_, tid); // TODO : implement func
					MT_block->cyc_count_++;
					decrease_thread_cycels(MT_block); // TODO : implement func
					break;
				}
				else if (is_halt) {
					MT_block->is_thread_valid[tid] = true; // if thread[i] is valid then the thread is finished
					MT_block->cyc_count_++;
					MT_block->cyc_count_ += MT_block->overhead;
					MT_block->halts_num++;
					lu_thread = find_next_thread(MT_block); // TODO : implement func
					decrease_thread_cycels(MT_block);
					break;
				}
				else {// store \ load command
					MT_block->cyc_count_++;
					MT_block->is_thread_valid[tid] = true;
					int tmp_lu = find_next_thread(MT_block);
					execute_command(MT_block->vec_threads_[tid][0]->inst_, tid);
					if (tmp_lu != lu_thread) {
						MT_block->cyc_count_ += MT_block->overhead;
						lu_thread = tmp_lu;
					}

					decrease_thread_cycels(MT_block);
				}
			}

		}

	}
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



multithread* init_multithread(int switch_cyc, int th_num, int load_cyc, int store_cyc) {
	
	multithread* MT = new multithread;
	
	MT->overhead = switch_cyc;
	MT->thread_num_ = th_num;
	MT->store_latency = store_cyc;
	MT->load_latency = load_cyc;

	MT->is_thread_valid = new bool[th_num];
	for (int i = 0; i < MT->thread_num_; i++)
		MT->is_thread_valid[i] = false;

	init_regs(MT);
	init_threads(MT);

	return MT;
}

void init_regs(multithread* mt) {

	for (int i = 0; i < mt->thread_num_; i++) {

		tcontext* tmp_context = new tcontext;
		for (int k = 0; k < REGS_COUNT; k++)
			tmp_context->reg[k] = 0;
		mt->vec_regs_.push_back(tmp_context);
	}

}

void init_threads(multithread* mt) {
	
	// creates the vectors with dump vectors.
	vector<inst_properties*> dumpy;
	for (int i = 0; i < mt->thread_num_; i++) 
		mt->vec_threads_.push_back(dumpy);

	for (int i = 0; i < mt->thread_num_; i++) {
		int line = 0;

		while (1) {
			inst_properties* tmp_inst = new inst_properties;
			tmp_inst->count_ = (tmp_inst->inst_.opcode < CMD_LOAD) ? 1 :
												(tmp_inst->inst_.opcode == CMD_LOAD) ? mt->load_latency
																					  : mt->store_latency;
			SIM_MemInstRead(line, &(tmp_inst->inst_), i);
			mt->vec_threads_[i].push_back(tmp_inst);
			if (tmp_inst->inst_.opcode == CMD_HALT) {
				mt->vec_threads_[i].erase(mt->vec_threads_[i].begin()); // erases dump inst
				break;
			}
			line++;
		}

	}
}