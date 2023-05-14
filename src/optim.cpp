#include <string>
#include <memory>
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <cassert>
#include <random>
#include <algorithm>
#include "koopa.hpp"
#include "optim.hpp"
#include "koopa2riscv.hpp"

using namespace std;
using namespace koopa;


void BuildBlockCFG(FunBody *ptr) {
	map<string, Block*> block2ptr;
	for (const auto &block: ptr->blocks) {
		block2ptr[block->symbol] = block.get();
		block->next_blocks.clear();
		block->prev_blocks.clear();
	}
	for (const auto &block: ptr->blocks) {
		auto end_stmt = block->end_stmt.get();
		if (end_stmt->stmt_type == JUMPEND) {
			auto jump_end = static_cast<const Jump*>(end_stmt);
			Block *next_ptr = block2ptr[jump_end->symbol];
			block->next_blocks.push_back(next_ptr);
			next_ptr->prev_blocks.push_back(block.get());
		} else if (end_stmt->stmt_type == BRANCHEND) {
			auto br_end = static_cast<const Branch*>(end_stmt);
			Block *next1 = block2ptr[br_end->symbol1];
			Block *next2 = block2ptr[br_end->symbol2];
			block->next_blocks.push_back(next1);
			block->next_blocks.push_back(next2);
			next1->prev_blocks.push_back(block.get());
			next2->prev_blocks.push_back(block.get());
		}
	}
}

void CutDeadBlocks(FunBody *ptr) {
	vector<Block*> vec;
	for (auto &block: ptr->blocks)
		block->marked = 0;
	vec.push_back(ptr->blocks[0].get());
	ptr->blocks[0]->marked = 1;
	while (!vec.empty()) {
		Block *cur = vec.back();
		vec.pop_back();
		for (Block *nxt: cur->next_blocks)
			if (!nxt->marked) {
				nxt->marked = 1;
				vec.push_back(nxt);
			}
	}
	vector<unique_ptr<Block> > new_blocks;
	for (auto &block: ptr->blocks)
		if (block->marked)
		{
			vector<Block*> new_prev_blocks;
			for (Block *prev: block->prev_blocks)
				if (prev->marked)
					new_prev_blocks.push_back(prev);
			block->prev_blocks = move(new_prev_blocks);
			new_blocks.push_back(move(block));
		}
	ptr->blocks = move(new_blocks);
}

void CountUsedVars(Block *block, map<string, int> &used_vars) {
	string block_name = block->symbol;
	int is_while = 1;
	if (block_name.substr(1,12) == "while_begin_" ||
		block_name.substr(1,11) == "while_body_")
			is_while = 10;
	auto end_stmt = static_cast<Statement*>(block->end_stmt.get());
	set<string> &end_live_vars = end_stmt->live_vars;
	end_live_vars.clear();
	if (end_stmt->stmt_type == RETURNEND) {
		auto ret_end = static_cast<const Return*>(end_stmt);
		if (ret_end->val && ret_end->val->val_type == SYMBOLVALUE) {
			auto symb = static_cast<const SymbolValue*>(ret_end->val.get());
			used_vars[symb->symbol] += is_while;
			end_live_vars.insert(symb->symbol);
		}
	} else if (end_stmt->stmt_type == BRANCHEND) {
		auto br_end = static_cast<const Branch*>(end_stmt);
		if (br_end->val->val_type == SYMBOLVALUE) {
			auto symb = static_cast<const SymbolValue*>(br_end->val.get());
			used_vars[symb->symbol] += is_while;
			end_live_vars.insert(symb->symbol);
		}
	}
	for (const auto &stmt: block->stmts) {
		set<string> &live_vars = stmt->live_vars;
		live_vars.clear();
		if (stmt->stmt_type == SYMBOLDEFSTMT) {
			auto symb_def = static_cast<const SymbolDef*>(stmt.get());
			if (symb_def->def_type == LOADDEF) {
				auto load_def = static_cast<const LoadDef*>(symb_def);
				used_vars[load_def->load->symbol] += is_while;
				live_vars.insert(load_def->load->symbol);
			} else if (symb_def->def_type == GETPTRDEF) {
				auto ptr_def = static_cast<const GetPtrDef*>(symb_def);
				used_vars[ptr_def->get_ptr->symbol] += is_while;
				live_vars.insert(ptr_def->get_ptr->symbol);
				auto val = ptr_def->get_ptr->val.get();
				if (val->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val);
					used_vars[symb_val->symbol] += is_while;
					live_vars.insert(symb_val->symbol);
				}
			} else if (symb_def->def_type == GETELEMPTRDEF) {
				auto ptr_def = static_cast<const GetElemPtrDef*>(symb_def);
				used_vars[ptr_def->get_elem_ptr->symbol] += is_while;
				live_vars.insert(ptr_def->get_elem_ptr->symbol);
				auto val = ptr_def->get_elem_ptr->val.get();
				if (val->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val);
					used_vars[symb_val->symbol] += is_while;
					live_vars.insert(symb_val->symbol);
				}
			} else if (symb_def->def_type == BINEXPRDEF) {
				auto bin_def = static_cast<const BinExprDef*>(symb_def);
				auto val1 = bin_def->bin_expr->val1.get();
				auto val2 = bin_def->bin_expr->val2.get();
				if (val1->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val1);
					used_vars[symb_val->symbol] += is_while;
					live_vars.insert(symb_val->symbol);
				}
				if (val2->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val2);
					used_vars[symb_val->symbol] += is_while;
					live_vars.insert(symb_val->symbol);
				}
			} else if (symb_def->def_type == FUNCALLDEF) {
				auto func_def = static_cast<const FunCallDef*>(symb_def);
				for (const auto &val: func_def->fun_call->params) {
					if (val->val_type == SYMBOLVALUE) {
						auto symb_val = static_cast<const SymbolValue*>(val.get());
						used_vars[symb_val->symbol] += is_while;
						live_vars.insert(symb_val->symbol);
					}
				}
			}
		} else if (stmt->stmt_type == STORESTMT) {
			auto store = static_cast<const Store*>(stmt.get());
			used_vars[store->symbol] += is_while;
			live_vars.insert(store->symbol);
			if (store->store_type == VALUESTORE) {
				auto val_store = static_cast<const ValueStore*>(store);
				if (val_store->val->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val_store->val.get());
					used_vars[symb_val->symbol] += is_while;
					live_vars.insert(symb_val->symbol);
				}
			}
		} else if (stmt->stmt_type == FUNCALLSTMT) {
			auto func = static_cast<const FunCall*>(stmt.get());
			for (const auto &val: func->params) {
				if (val->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val.get());
					used_vars[symb_val->symbol] += is_while;
					live_vars.insert(symb_val->symbol);
				}
			}
		}
	}

}

void CutDeadVars(FunBody *ptr, map<string, int> &used_vars) {
	while(1) {
		used_vars.clear();
		for (auto &block: ptr->blocks)
			CountUsedVars(block.get(), used_vars);
		int cut = 0;
		for (auto &block: ptr->blocks) {
			vector<unique_ptr<Statement> > new_stmts;
			for (auto &stmt: block->stmts) {
				if (stmt->stmt_type ==  SYMBOLDEFSTMT) {
					auto symb_stmt = static_cast<SymbolDef*>(stmt.get());
					string symb = symb_stmt->symbol;
					if (!used_vars.count(symb)) {
						cut = 1;
						if (symb_stmt->def_type != FUNCALLDEF)
							continue;
						else {
							auto func_def = static_cast<FunCallDef*>(symb_stmt);
							auto new_stmt = move(func_def->fun_call);
							new_stmts.push_back(move(new_stmt));
							continue;
						}
					}
				}
				new_stmts.push_back(move(stmt));
			}
			block->stmts = move(new_stmts);
		}
		if (!cut)
			break;
	}
	
}

void BuildStmtCFG(FunBody *ptr) {
	for (auto &block: ptr->blocks) {
		Statement *prev = nullptr;
		auto end_stmt = static_cast<Statement*>(block->end_stmt.get());
		for (auto &stmt: block->stmts) {
			if (prev) {
				prev->next_stmts.push_back(stmt.get());
				stmt->prev_stmts.push_back(prev);
			}
			prev = stmt.get();
		}
		if (prev) {
			prev->next_stmts.push_back(end_stmt);
			end_stmt->prev_stmts.push_back(prev);
		}
		for (Block *next_blk: block->next_blocks) {
			if (!next_blk->stmts.empty()) {
				end_stmt->next_stmts.push_back(next_blk->stmts[0].get());
				next_blk->stmts[0]->prev_stmts.push_back(end_stmt);
			} else {
				end_stmt->next_stmts.push_back(next_blk->end_stmt.get());
				next_blk->end_stmt->prev_stmts.push_back(end_stmt);
			}
		}
	}
}

void GetLiveVars(FunBody *ptr) {
	queue<Statement*> q;
	for (auto &block: ptr->blocks) {
		if (!block->end_stmt->live_vars.empty())
			q.push(block->end_stmt.get());
		for (auto &stmt: block->stmts) {
			if (!stmt->live_vars.empty())
				q.push(stmt.get());
		}
	}
	while (!q.empty()) {
		Statement *cur = q.front();
		q.pop();
		for (Statement *nxt: cur->prev_stmts) {
			int upd = 0;
			string del;
			if (nxt->stmt_type == SYMBOLDEFSTMT) {
				auto symb_def = static_cast<SymbolDef*>(nxt);
				del = symb_def->symbol;
			}
			for (string var: cur->live_vars)
				if (var != del) {
					if (!nxt->live_vars.count(var)) {
						upd = 1;
						nxt->live_vars.insert(var);
					}
				}
			if (upd)
				q.push(nxt);
		}
	}
}

void AllocRegs(FunBody *ptr, map<string, int> &var2reg, map<string, int> &used_vars) {
	set<string> defed_vars;
	map<string, set<string> > edges;
	map<string, int> degree;
	map<string, int> spill_cost;
	for (auto &block: ptr->blocks) {
		for (auto &stmt: block->stmts)
			if (stmt->stmt_type == SYMBOLDEFSTMT) {
				auto symb_def = static_cast<SymbolDef*>(stmt.get());
				if (symb_def->def_type == MEMORYDEF) {
					auto mem_def = static_cast<MemoryDef*>(symb_def);
					if (mem_def->mem_dec->mem_type->my_type == ARRAYTYPE) {
						var2reg[symb_def->symbol] = -1;
						continue;
					}
				}
				defed_vars.insert(symb_def->symbol);
				var2reg[symb_def->symbol] = -1;
			}
	}
	for (auto &block: ptr->blocks) {
		auto end_stmt = block->end_stmt.get();
		for (string var1: end_stmt->live_vars)
			for (string var2: end_stmt->live_vars)
				if (var1 != var2 && defed_vars.count(var1) && defed_vars.count(var2)) {
					edges[var1].insert(var2);
					edges[var2].insert(var1);
				}
		for (auto &stmt: block->stmts)
			for (string var1: stmt->live_vars)
				for (string var2: stmt->live_vars)
					if (var1 != var2 && defed_vars.count(var1) && defed_vars.count(var2)) {
						edges[var1].insert(var2);
						edges[var2].insert(var1);
					}
	}
	for (auto pr: edges)
		degree[pr.first] = pr.second.size();
	vector<string> free_vars;
	vector<string> spilled_vars;
	while (!defed_vars.empty()) {
		queue<string> q;
		for (string var: defed_vars) {
			if (degree[var] < max_regs)
				q.push(var);
		}
		while(!q.empty()) {
			string cur = q.front();
			q.pop();
			if (!defed_vars.count(cur))
				continue;
			defed_vars.erase(cur);
			free_vars.push_back(cur);
			for (string nxt: edges[cur])
				if (defed_vars.count(nxt))
					if (--degree[nxt] < max_regs) {
						q.push(nxt);
					}
		}
		if (defed_vars.empty())
			break;
		double min_cost = 1e18;
		string max_var;
		for (auto var: defed_vars)
			if (used_vars[var] / degree[var] < min_cost) {
				min_cost = used_vars[var] / degree[var];
				max_var = var;
			}
		defed_vars.erase(max_var);
		spilled_vars.push_back(max_var);
		var2reg[max_var] = -1;
		for (string nxt: edges[max_var])
			if (defed_vars.count(nxt))
				degree[nxt]--;
	}
	while(!free_vars.empty()) {
		string cur = free_vars.back();
		free_vars.pop_back();
		int colored[max_regs] = {};
		for (string nxt: edges[cur])
			if (var2reg[nxt] >= 0) {
				colored[var2reg[nxt]] = 1;
			}
		for (int i = 0; i < max_regs; i++)
			if (!colored[i])
			{
				var2reg[cur] = i;
				break;
			}
		assert(var2reg[cur] != -1);
	}
	mt19937 rnd(514);
	shuffle(spilled_vars.begin(), spilled_vars.end(), rnd);
	for (string cur: spilled_vars) {
		int colored[max_regs] = {};
		for (string nxt: edges[cur])
			if (var2reg[nxt] >= 0) {
				colored[var2reg[nxt]] = 1;
			}
		for (int i = 0; i < max_regs; i++)
			if (!colored[i])
			{
				var2reg[cur] = i;
				break;
			}
	}
	int banned[8] = {};
	int remap_regs[25] = {};
	for (int i = 0; i < 25;i ++)
		remap_regs[i] = i;
	for (auto &pr: var2reg) {
		string name = pr.first;
		int reg = pr.second;
		if (name[0] == '%' && !(name[1] >= '0' && name[1] <= '9')) {
			if (reg >= 17)
				banned[reg-17] = 1;
		}
	}
	for (int i = 0; i < 8; i++)
		if (banned[i])
			swap(remap_regs[i], remap_regs[i+17]);
	for (auto &pr: var2reg)
		if(pr.second >= 0)
			pr.second = remap_regs[pr.second];
}
