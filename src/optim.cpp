#include <string>
#include <memory>
#include <set>
#include <map>
#include <vector>
#include "koopa.hpp"

using namespace std;
using namespace koopa;

const int max_regs = 25;

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
}

void CountUsedVars(Block *block, set<string> &used_vars) {
	auto end_stmt = static_cast<Statement*>(block->end_stmt.get());
	set<string> &end_live_vars = end_stmt->live_vars;
	if (end_stmt->stmt_type == RETURNEND) {
		auto ret_end = static_cast<const Return*>(end_stmt);
		if (ret_end->val && ret_end->val->val_type == SYMBOLVALUE) {
			auto symb = static_cast<const SymbolValue*>(ret_end->val.get());
			used_vars.insert(symb->symbol);
			end_live_vars.insert(symb->symbol);
		}
	} else if (end_stmt->stmt_type == BRANCHEND) {
		auto br_end = static_cast<const Branch*>(end_stmt);
		if (br_end->val->val_type == SYMBOLVALUE) {
			auto symb = static_cast<const SymbolValue*>(br_end->val.get());
			used_vars.insert(symb->symbol);
			end_live_vars.insert(symb->symbol);
		}
	}
	for (const auto &stmt: block->stmts) {
		set<string> &live_vars = stmt->live_vars;
		if (stmt->stmt_type == SYMBOLDEFSTMT) {
			auto symb_def = static_cast<const SymbolDef*>(stmt.get());
			if (symb_def->def_type == LOADDEF) {
				auto load_def = static_cast<const LoadDef*>(symb_def);
				used_vars.insert(load_def->load->symbol);
				live_vars.insert(load_def->load->symbol);
			} else if (symb_def->def_type == GETPTRDEF) {
				auto ptr_def = static_cast<const GetPtrDef*>(symb_def);
				used_vars.insert(ptr_def->get_ptr->symbol);
				live_vars.insert(ptr_def->get_ptr->symbol);
				auto val = ptr_def->get_ptr->val.get();
				if (val->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val);
					used_vars.insert(symb_val->symbol);
					live_vars.insert(symb_val->symbol);
				}
			} else if (symb_def->def_type == GETELEMPTRDEF) {
				auto ptr_def = static_cast<const GetElemPtrDef*>(symb_def);
				used_vars.insert(ptr_def->get_elem_ptr->symbol);
				live_vars.insert(ptr_def->get_elem_ptr->symbol);
				auto val = ptr_def->get_elem_ptr->val.get();
				if (val->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val);
					used_vars.insert(symb_val->symbol);
					live_vars.insert(symb_val->symbol);
				}
			} else if (symb_def->def_type == BINEXPRDEF) {
				auto bin_def = static_cast<const BinExprDef*>(symb_def);
				auto val1 = bin_def->bin_expr->val1.get();
				auto val2 = bin_def->bin_expr->val2.get();
				if (val1->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val1);
					used_vars.insert(symb_val->symbol);
					live_vars.insert(symb_val->symbol);
				}
				if (val2->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val2);
					used_vars.insert(symb_val->symbol);
					live_vars.insert(symb_val->symbol);
				}
			} else if (symb_def->def_type == FUNCALLDEF) {
				auto func_def = static_cast<const FunCallDef*>(symb_def);
				for (const auto &val: func_def->fun_call->params) {
					if (val->val_type == SYMBOLVALUE) {
						auto symb_val = static_cast<const SymbolValue*>(val.get());
						used_vars.insert(symb_val->symbol);
						live_vars.insert(symb_val->symbol);
					}
				}
			}
		} else if (stmt->stmt_type == STORESTMT) {
			auto store = static_cast<const Store*>(stmt.get());
			used_vars.insert(store->symbol);
			live_vars.insert(store->symbol);
			if (store->store_type == VALUESTORE) {
				auto val_store = static_cast<const ValueStore*>(store);
				if (val_store->val->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val_store->val.get());
					used_vars.insert(symb_val->symbol);
					live_vars.insert(symb_val->symbol);
				}
			}
		} else if (stmt->stmt_type == FUNCALLSTMT) {
			auto func = static_cast<const FunCall*>(stmt.get());
			for (const auto &val: func->params) {
				if (val->val_type == SYMBOLVALUE) {
					auto symb_val = static_cast<const SymbolValue*>(val.get());
					used_vars.insert(symb_val->symbol);
					live_vars.insert(symb_val->symbol);
				}
			}
		}
	}

}

void CutDeadVars(FunBody *ptr) {
	set<string> used_vars;
	for (auto &block: ptr->blocks)
		CountUsedVars(block.get(), used_vars);
	for (auto &block: ptr->blocks) {
		vector<unique_ptr<Statement> > new_stmts;
		for (auto &stmt: block->stmts) {
			if (stmt->stmt_type ==  SYMBOLDEFSTMT) {
				auto symb_stmt = static_cast<SymbolDef*>(stmt.get());
				string symb = symb_stmt->symbol;
				if (!used_vars.count(symb)) {
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
	set<Statement*> s;
	for (auto &block: ptr->blocks) {
		if (!block->end_stmt->live_vars.empty())
			s.insert(block->end_stmt.get());
		for (auto &stmt: block->stmts) {
			if (!stmt->live_vars.empty())
				s.insert(stmt.get());
		}
	}
	while (!s.empty()) {
		Statement *cur = *s.begin();
		s.erase(s.begin());
		for (Statement *nxt: cur->prev_stmts) {
			int upd = 0;
			string del;
			if (nxt->stmt_type == SYMBOLDEFSTMT) {
				auto symb_def = static_cast<SymbolDef*>(cur);
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
				s.insert(nxt);
		}
	}
}