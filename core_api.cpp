/* 046267 Computer Architecture - Spring 2020 - HW #4 */
#define _CRT_SECURE_NO_WARNINGS

#include "core_api.h"
#include "sim_api.h"

#include <vector>
#include <stdio.h>

#define INT_MAX 9999999

using namespace std;

class inst_properties {
public:
	int count_ = 0;
	Instruction inst_;
};

class multithread {
public:
	int overhead = -1;
	int thread_num_ = 0;
	int halts_num = 0;

	int store_latency = 0;
	int load_latency = 0;

	double Inst_num_ = 0;
	int cyc_count_ = 0;
	vector<bool> is_thread_valid;
	vector< vector< inst_properties* > > vec_threads_;
	vector<tcontext*> vec_regs_;
};

multithread* init_multithread(int, int, int, int);
void init_regs(multithread*);
void init_threads(multithread*);
void execute_command(multithread*, Instruction, int);
void decrease_thread_cycels(multithread*, int);
int find_next_thread(multithread*, int, bool);
void free_mt(multithread*);
void start_proccess(multithread*, int, bool);

multithread* MT_block;
multithread* MT_FG;

void CORE_BlockedMT() {

	MT_block = init_multithread(SIM_GetSwitchCycles(), SIM_GetThreadsNum(),
													SIM_GetLoadLat(), SIM_GetStoreLat());
	if (!MT_block)
		return;
	if (MT_block->thread_num_ == 0)
		return;
	int lu_thread = 0;

	while (MT_block->halts_num < MT_block->thread_num_) {
		
		for (int tid = 0; tid < MT_block->thread_num_; tid++) {
			bool is_halt = (MT_block->vec_threads_[tid][0]->inst_.opcode == CMD_HALT) ? true : false;

			if (lu_thread == tid) { // enters if cmd = {add, addi, sub, subi, nop}
				if ((MT_block->vec_threads_[tid][0]->inst_.opcode < CMD_LOAD)){
					execute_command(MT_block, MT_block->vec_threads_[tid][0]->inst_, tid); 
					start_proccess(MT_block, tid, false);
					decrease_thread_cycels(MT_block, tid);
					break;
				}
				else if (is_halt) {
					start_proccess(MT_block, tid, false); // if thread[i] is valid then the thread is finished
					decrease_thread_cycels(MT_block, tid);
					if (MT_block->halts_num < MT_block->thread_num_ - 1) {
						start_proccess(MT_block, tid, true);
					}
					else {
						MT_block->halts_num++;
						break;
					}
					MT_block->halts_num++;
					decrease_thread_cycels(MT_block, tid);
					lu_thread = find_next_thread(MT_block, tid, true);
					
					break;
				}
				else {// store \ load command
					execute_command(MT_block, MT_block->vec_threads_[tid][0]->inst_, tid);
					start_proccess(MT_block, tid, false);
					decrease_thread_cycels(MT_block, tid);
					int tmp_lu = find_next_thread(MT_block, tid, true);
					if (tmp_lu != lu_thread) {
						start_proccess(MT_block, tid, true);
						decrease_thread_cycels(MT_block, tid);
						lu_thread = tmp_lu;
					}
				}
			}

		}

	}
}

void CORE_FinegrainedMT() {

	MT_FG = init_multithread(SIM_GetSwitchCycles(), SIM_GetThreadsNum(),
		SIM_GetLoadLat(), SIM_GetStoreLat());
	if (!MT_FG)
		return;
	if (MT_FG->thread_num_ == 0)
		return;


	for (int tid = 0; tid < MT_FG->thread_num_; tid++) {
		bool is_halt = (MT_FG->vec_threads_[tid][0]->inst_.opcode == CMD_HALT) ? true : false;

		if (!is_halt && !(MT_FG->is_thread_valid[tid]) )
			execute_command(MT_FG, MT_FG->vec_threads_[tid][0]->inst_, tid);

		if (is_halt)
			MT_FG->halts_num++;
		
		start_proccess(MT_FG, tid, false);
		decrease_thread_cycels(MT_FG, tid);
		tid = find_next_thread(MT_FG, tid, false);
		tid--;

		if (MT_FG->halts_num == MT_FG->thread_num_)
			break;
	}

}

double CORE_BlockedMT_CPI() {
	double mt_B_CPI = MT_block->cyc_count_ / MT_block->Inst_num_;
	free_mt(MT_block);
	return mt_B_CPI;
}

double CORE_FinegrainedMT_CPI() {
	double mt_FG_CPI = MT_FG->cyc_count_ / MT_FG->Inst_num_;
	free_mt(MT_FG);
	return mt_FG_CPI;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	if ( (!context) || (threadid < 0) || (threadid >= MT_block->thread_num_))
		return;
	for(int i = 0; i < REGS_COUNT; i++)
		context[threadid].reg[i] = MT_block->vec_regs_[threadid]->reg[i];
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	if ( (!context) || (threadid < 0) || (threadid >= MT_FG->thread_num_))
		return;
	for (int i = 0; i < REGS_COUNT; i++)
		context[threadid].reg[i] = MT_FG->vec_regs_[threadid]->reg[i];

}

multithread* init_multithread(int switch_cyc, int th_num, int load_cyc, int store_cyc) {
	
	multithread* MT = new multithread;
	
	MT->overhead = switch_cyc;
	MT->thread_num_ = th_num;
	MT->store_latency = store_cyc;
	MT->load_latency = load_cyc;


	for (int i = 0; i < MT->thread_num_; i++)
		MT->is_thread_valid.push_back(false);

	init_regs(MT);
	init_threads(MT);

	for (int i = 0; i < MT->thread_num_; i++)
		MT->Inst_num_ += MT->vec_threads_[i].size();

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
			SIM_MemInstRead(line, &(tmp_inst->inst_), i);
			tmp_inst->count_ = ( (tmp_inst->inst_.opcode < CMD_LOAD) || (tmp_inst->inst_.opcode == CMD_HALT) ) ? 1 :
												(tmp_inst->inst_.opcode == CMD_LOAD) ? mt->load_latency
																					  : mt->store_latency;
			
			mt->vec_threads_[i].push_back(tmp_inst);
			if (tmp_inst->inst_.opcode == CMD_HALT) {
				break;
			}
			line++;
		}

	}
}

void execute_command(multithread* mt, Instruction inst, int tid) {
	
	switch (inst.opcode){
		     case CMD_NOP: // NOP
				 break;
			 case CMD_ADDI:
				 mt->vec_regs_[tid]->reg[inst.dst_index] = mt->vec_regs_[tid]->reg[inst.src1_index] + inst.src2_index_imm;
				 break;
			 case CMD_SUBI:
				 mt->vec_regs_[tid]->reg[inst.dst_index] = mt->vec_regs_[tid]->reg[inst.src1_index] - inst.src2_index_imm;
				 break;
			 case CMD_ADD:
				 mt->vec_regs_[tid]->reg[inst.dst_index] = mt->vec_regs_[tid]->reg[inst.src1_index] +
																		 mt->vec_regs_[tid]->reg[inst.src2_index_imm];
				 break;
			 case CMD_SUB:
				 mt->vec_regs_[tid]->reg[inst.dst_index] = mt->vec_regs_[tid]->reg[inst.src1_index] -
																		 mt->vec_regs_[tid]->reg[inst.src2_index_imm];
				 break;
			 case CMD_LOAD:
								//mt->vec_regs_[tid]->reg[inst.src1_index] +
				 SIM_MemDataRead((inst.isSrc2Imm) ? inst.src2_index_imm : 
							 mt->vec_regs_[tid]->reg[inst.src2_index_imm], &mt->vec_regs_[tid]->reg[inst.dst_index]);
				 break;
			 case CMD_STORE:
									//mt->vec_regs_[tid]->reg[inst.src1_index] + (
				 SIM_MemDataWrite((inst.isSrc2Imm) ? inst.src2_index_imm :
							 mt->vec_regs_[tid]->reg[inst.src2_index_imm], mt->vec_regs_[tid]->reg[inst.dst_index]);
				 break;
			 case CMD_HALT:
				 break;
	}
}

void decrease_thread_cycels(multithread* mt, int tid) {

	int min_cnt = INT_MAX;
	int proccess_cmd_num = 0;
	for (int i = 0; i < mt->thread_num_; i++) {
		bool is_halt = (mt->vec_threads_[i][0]->inst_.opcode == CMD_HALT);
		if (mt->is_thread_valid[i]) {
			proccess_cmd_num++;
			if (!is_halt)
				min_cnt = (mt->vec_threads_[i][0]->count_ < min_cnt) ? mt->vec_threads_[i][0]->count_ : min_cnt;
		}
	}

	for (int i = 0; i < mt->thread_num_; i++) {
		bool is_halt = (mt->vec_threads_[i][0]->inst_.opcode == CMD_HALT);
		if (proccess_cmd_num == mt->thread_num_) { // proccessor is full
			if ( (min_cnt < INT_MAX) && (min_cnt >= 1) ) {
				mt->vec_threads_[i][0]->count_ -= min_cnt;
				if (i == tid) // happens only once in a loop
					mt->cyc_count_ += min_cnt; 
			}
			
			if ((mt->vec_threads_[i][0]->count_ == 0) && !is_halt) {
				delete mt->vec_threads_[i][0]; // deletes inst_properties class allocation
				mt->vec_threads_[i].erase(mt->vec_threads_[i].begin()); // deletes first cmd
				mt->is_thread_valid[i] = false;
			}
		}
		else if (mt->is_thread_valid[i]) {
			if ((mt->vec_threads_[i][0]->count_ == 0) && !is_halt ) {
				delete mt->vec_threads_[i][0]; // deletes inst_properties class allocation
				mt->vec_threads_[i].erase(mt->vec_threads_[i].begin()); // deletes first cmd
				mt->is_thread_valid[i] = false;
			}
		}
	}

}

int find_next_thread(multithread* mt, int tid, bool is_MT_blocked) {

	
	for (int i = ((tid + !is_MT_blocked) % mt->thread_num_); i < mt->thread_num_; i = ++i % mt->thread_num_) {
		if (!mt->is_thread_valid[i])
			return i;

		if (i == tid - is_MT_blocked) 
			return tid;
	}

}

void free_mt(multithread* mt) {

	for (int i = 0; i < mt->thread_num_; i++) {

		delete *mt->vec_threads_[i].begin(); // deletes ramining HALTS
		delete mt->vec_regs_[i]; // deletes register files (*tcontext)
	}

	delete mt;

}

void start_proccess(multithread* mt, int tid, bool is_overhead) {

	for (int i = 0; i < mt->thread_num_; i++) {
		if (mt->is_thread_valid[i])
			if(is_overhead)
				mt->vec_threads_[i][0]->count_ -= mt->overhead;
			else
				mt->vec_threads_[i][0]->count_--;
		if (mt->vec_threads_[i][0]->count_ < 0)
			mt->vec_threads_[i][0]->count_ = 0;
	}

	mt->is_thread_valid[tid] = true;
	if (mt->vec_threads_[tid][0]->count_ == 1 && (mt->vec_threads_[tid][0]->count_ <= CMD_LOAD) )
		mt->vec_threads_[tid][0]->count_--;

	if (is_overhead)
		mt->cyc_count_ += mt->overhead;
	else
		mt->cyc_count_++;
}
