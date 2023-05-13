#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include "koopa.hpp"
#include "types.hpp"
#include "riscv.hpp"
#include "koopa2riscv.hpp"
#include "optim.hpp"

using namespace std;

vector<unique_ptr<riscv::Item> > code;
map<string, VarInfo> global_var_info;
string reg_name[25];
int return_counter = 0;
string cur_return_label;

void GetVarType(koopa::FunDef *ptr, map<string, VarInfo> &var_info) {
	auto body = ptr->body.get();
	auto &params = ptr->params->params;
	for (int i = 0; i < params.size(); i++) {
		string name = params[i].first;
		shared_ptr<koopa::Type> type = params[i].second;
		if (i < 8)
			var_info[name] = VarInfo(name, PARAMDEF, type, i + 17, -1);
		else
			var_info[name] = VarInfo(name, PARAMDEF, type, -1, -1);
	}
	for (auto &block: body->blocks)
		for (auto &stmt: block->stmts)
			if (stmt->stmt_type == koopa::SYMBOLDEFSTMT) {
				auto symb_def = static_cast<koopa::SymbolDef*>(stmt.get());
				string name = symb_def->symbol;
				if (symb_def->def_type == koopa::MEMORYDEF) {
					auto mem_def = static_cast<koopa::MemoryDef*>(symb_def);
					auto mem_type = mem_def->mem_dec->mem_type;
					auto new_type = make_shared<koopa::PointerType>(mem_type);
					var_info[name] = VarInfo(name, ALLOCDEF, new_type);
					} else if (symb_def->def_type == koopa::LOADDEF) {
						auto load_def = static_cast<koopa::LoadDef*>(symb_def);
						string load_symb = load_def->load->symbol;
						auto load_type = var_info[load_symb].type;
						assert(load_type->my_type == koopa::POINTERTYPE);
						auto ptr_type = static_cast<koopa::PointerType*>(load_type.get());
						var_info[name] = VarInfo(name, LOCALDEF, ptr_type->ptr);
				} else if (symb_def->def_type == koopa::GETPTRDEF) {
					auto ptr_def = static_cast<koopa::GetPtrDef*>(symb_def);
					string ptr_symb = ptr_def->get_ptr->symbol;
					auto new_type = var_info[ptr_symb].type;
					var_info[name] = VarInfo(name, LOCALDEF, new_type);
				} else if (symb_def->def_type == koopa::GETELEMPTRDEF) {
					auto ptr_def = static_cast<koopa::GetElemPtrDef*>(symb_def);
					string ptr_symb = ptr_def->get_elem_ptr->symbol;
					auto ptr_type = var_info[ptr_symb].type.get();
					assert(ptr_type->my_type == koopa::POINTERTYPE);
					auto new_type = static_cast<koopa::PointerType*>(ptr_type);
					assert(new_type->ptr->my_type == koopa::ARRAYTYPE);
					auto arr_type = static_cast<koopa::ArrayType*>(new_type->ptr.get());
					auto ret_type = make_shared<koopa::PointerType>(arr_type->arr);
					var_info[name] = VarInfo(name, LOCALDEF, ret_type);
				} else if (symb_def->def_type == koopa::BINEXPRDEF) {
					var_info[name] = VarInfo(name, LOCALDEF, make_shared<koopa::IntType>());
				} else if (symb_def->def_type == koopa::FUNCALLDEF) {
					var_info[name] = VarInfo(name, LOCALDEF, make_shared<koopa::IntType>());
				}
			}
}

int GetFunOffset(koopa::FunDef *ptr,
map<string, VarInfo> &var_info,
int reg_used[], int reg_offset[], int &has_call) {
	int ofst = 0;
	int max_params = 8;
	has_call = 0;
	for (auto &block: ptr->body->blocks)
		for (auto &stmt: block->stmts) {
			if (stmt->stmt_type == koopa::FUNCALLSTMT) {
				has_call = 1;
				auto fun_call = static_cast<koopa::FunCall*>(stmt.get());
				max_params = max(max_params, (int)fun_call->params.size());
			} else if (stmt->stmt_type == koopa::SYMBOLDEFSTMT) {
				auto symb_def = static_cast<koopa::SymbolDef*>(stmt.get());
				if (symb_def->def_type == koopa::FUNCALLDEF) {
					has_call = 1;
					auto fun_def = static_cast<koopa::FunCallDef*>(symb_def);
					max_params = max(max_params, (int)fun_def->fun_call->params.size());
				}
			}
		}
	ofst += (max_params - 8) * 4;
	for (int i = 0; i < 25; i++)
		if (reg_used[i])
		{
			reg_offset[i] = ofst;
			ofst += 4;
		}
	for (auto &pr: var_info) {
		string name = pr.first;
		VarInfo &info = pr.second;
		if (info.var_def == LOCALDEF) {
			if (info.reg < 0) {
				info.offset = ofst;
				ofst += 4;
			}
		} else if (info.var_def == ALLOCDEF) {
			if (info.reg < 0) {
				assert(info.type->my_type == koopa::POINTERTYPE);
				auto ptr_type = static_cast<koopa::PointerType*>(info.type.get());
				info.offset = ofst;
				ofst += ptr_type->ptr->Size();
			}
		}
	}
	ofst += has_call * 4;
	if (ofst % 16 != 0)
		ofst += 16 - ofst % 16;
	auto &params = ptr->params->params;
	for (int i = 8; i < params.size(); i++) {
		string name = params[i].first;
		shared_ptr<koopa::Type> type = params[i].second;
		var_info[name] = VarInfo(name, PARAMDEF, type, -1, ofst + (i - 8) * 4);
	}
	return ofst;
}

void LoadOffset(string reg, int ofst) {
	if (ofst < 2048)
		code.push_back(make_unique<riscv::ImmInstr>("lw", reg, "sp", ofst));
	else {
		code.push_back(make_unique<riscv::ImmInstr>("li", reg, "", ofst));
		code.push_back(make_unique<riscv::RegInstr>("add", reg, reg, "sp"));
		code.push_back(make_unique<riscv::ImmInstr>("lw", reg, reg, 0));
	}
}

void StoreOffset(string reg, int ofst) {
	if (ofst < 2048)
		code.push_back(make_unique<riscv::ImmInstr>("sw", reg, "sp", ofst));
	else {
		string tmp = reg == "t0" ? "t1" : "t0";
		code.push_back(make_unique<riscv::ImmInstr>("li", tmp, "", ofst));
		code.push_back(make_unique<riscv::RegInstr>("add", tmp, tmp, "sp"));
		code.push_back(make_unique<riscv::ImmInstr>("sw", reg, tmp, 0));
	}
}

string LoadVar(const VarInfo &info, string hint) {
	if (info.reg >= 0)
		return reg_name[info.reg];
	else if (info.var_def == LOCALDEF || info.var_def == PARAMDEF) {
		LoadOffset(hint, info.offset);
		return hint;
	} else if (info.var_def == ALLOCDEF) {
		assert(info.type->my_type == koopa::POINTERTYPE);
		auto ptr_type = static_cast<koopa::PointerType*>(info.type.get());
		if (ptr_type->ptr->my_type == koopa::ARRAYTYPE) {
			if (info.offset < 2048)
				code.push_back(make_unique<riscv::ImmInstr>("addi", hint, "sp", info.offset));
			else {
				code.push_back(make_unique<riscv::ImmInstr>("li", hint, "", info.offset));
				code.push_back(make_unique<riscv::RegInstr>("add", hint, hint, "sp"));
			}
			return hint;
		} else {
			LoadOffset(hint, info.offset);
			return hint;
		}
	} else if (info.var_def == GLOBALDEF) {
		string reg = hint;
		code.push_back(make_unique<riscv::LabelInstr>("la", reg, info.name));
		return reg;
	} else
		assert(0);
	return "";
}

string LoadInt(int val, string hint) {
	code.push_back(make_unique<riscv::ImmInstr>("li", hint, "", val));
	return hint;
}

string LoadKoopaValue(const koopa::Value *val, map<string, VarInfo> &var_info, string hint) {
	if (val->val_type == koopa::INTVALUE) {
		auto int_val = static_cast<const koopa::IntValue*>(val);
		return LoadInt(int_val->integer, hint);
	} else {
		auto symb_val = static_cast<const koopa::SymbolValue*>(val);
		const VarInfo &info = var_info[symb_val->symbol];
		return LoadVar(info, hint);
	}
	return "";
}

void StoreVar(const VarInfo &info, string rs) {
	if (info.reg >= 0) {
		string rd = reg_name[info.reg];
		if (rd != rs)
			code.push_back(make_unique<riscv::RegInstr>("mv", rd, rs, ""));
	} else if (info.var_def == LOCALDEF || info.var_def == ALLOCDEF) {
		StoreOffset(rs, info.offset);
	} else if (info.var_def == GLOBALDEF) {
		string tmp = rs == "t0" ? "t1" : "t0";
		code.push_back(make_unique<riscv::LabelInstr>("la", tmp, info.name));
		code.push_back(make_unique<riscv::ImmInstr>("sw", rs, tmp, 0));
	} else
		assert(0);
}

void ParseFunCall(koopa::FunCall *ptr,
map<string, VarInfo> &var_info,
int reg_used[], int reg_offset[]) {
	int num_params = ptr->params.size();
	for (int i = 0; i < 8 && i < num_params; i++) {
		auto val = ptr->params[i].get();
		string ai = "a" + to_string(i);
		string rs = LoadKoopaValue(val, var_info, ai);
		if (rs != ai)
			code.push_back(make_unique<riscv::RegInstr>("mv", ai, rs, ""));
	}
	for (int i = 8; i < num_params; i++) {
		auto val = ptr->params[i].get();
		string rs = LoadKoopaValue(val, var_info, "t0");
		StoreOffset(rs, (i-8)*4);
	}
	code.push_back(make_unique<riscv::LabelInstr>("call", "", ptr->symbol.substr(1)));
}

void ParseFunBody(koopa::FunBody *ptr,
map<string, VarInfo> &var_info,
int reg_used[], int reg_offset[]) {
	for (auto &block: ptr->blocks) {
		code.push_back(make_unique<riscv::Label>(block->symbol.substr(1)));
		for (auto &stmt: block->stmts) {
			if (stmt->stmt_type == koopa::SYMBOLDEFSTMT) {
				auto symb_def = static_cast<koopa::SymbolDef*>(stmt.get());
				string symb_dest = symb_def->symbol;
				const VarInfo &dest_info = var_info[symb_dest];
				string dest_reg = "t0";
				if (dest_info.reg >= 0)
					dest_reg = reg_name[dest_info.reg];
				if (symb_def->def_type == koopa::MEMORYDEF) {
				} else if (symb_def->def_type == koopa::LOADDEF) {
					auto load_def = static_cast<koopa::LoadDef*>(symb_def);
					string load_from = load_def->load->symbol;
					const VarInfo &load_info = var_info[load_from];
					if (load_info.var_def == ALLOCDEF) {
						string reg = LoadVar(load_info, dest_reg);
						StoreVar(dest_info, reg);
					} else {
						string reg = LoadVar(load_info, "t0");
						code.push_back(make_unique<riscv::ImmInstr>("lw", dest_reg, reg, 0));
						StoreVar(dest_info, dest_reg);
					}
				} else if (symb_def->def_type == koopa::GETPTRDEF) {
					auto ptr_def = static_cast<koopa::GetPtrDef*>(symb_def);
					string base = ptr_def->get_ptr->symbol;
					auto len = ptr_def->get_ptr->val.get();
					const VarInfo &base_info = var_info[base];
					auto base_type = base_info.type.get();
					assert(base_type->my_type == koopa::POINTERTYPE);
					auto ptr_type = static_cast<koopa::PointerType*>(base_type);
					int size = ptr_type->ptr->Size();
					string len_reg = LoadKoopaValue(len, var_info, "t0");
					string mul_int = LoadInt(size, "t1");
					code.push_back(make_unique<riscv::RegInstr>("mul", len_reg, mul_int, len_reg));
					string base_reg = LoadVar(base_info, "t1");
					code.push_back(make_unique<riscv::RegInstr>("add", dest_reg, base_reg, len_reg));
					StoreVar(dest_info, dest_reg);
				} else if (symb_def->def_type == koopa::GETELEMPTRDEF) {
					auto elem_def = static_cast<koopa::GetElemPtrDef*>(symb_def);
					string base = elem_def->get_elem_ptr->symbol;
					auto len = elem_def->get_elem_ptr->val.get();
					const VarInfo &base_info = var_info[base];
					auto base_type = base_info.type.get();
					assert(base_type->my_type == koopa::POINTERTYPE);
					auto ptr_type = static_cast<koopa::PointerType*>(base_type);
					auto arr_type = ptr_type->ptr.get();
					assert(arr_type->my_type == koopa::ARRAYTYPE);
					auto new_type = static_cast<koopa::ArrayType*>(arr_type);
					int size = new_type->arr->Size();
					string len_reg = LoadKoopaValue(len, var_info, "t0");
					string mul_int = LoadInt(size, "t1");
					code.push_back(make_unique<riscv::RegInstr>("mul", len_reg, mul_int, len_reg));
					string base_reg = LoadVar(base_info, "t1");
					code.push_back(make_unique<riscv::RegInstr>("add", dest_reg, base_reg, len_reg));
					StoreVar(dest_info, dest_reg);
				} else if (symb_def->def_type == koopa::BINEXPRDEF) {
					auto bin_def = static_cast<koopa::BinExprDef*>(symb_def);
					string op = bin_def->bin_expr->op;
					string reg1 = LoadKoopaValue(bin_def->bin_expr->val1.get(), var_info, "t0");
					string reg2 = LoadKoopaValue(bin_def->bin_expr->val2.get(), var_info, "t1");
					if (op == "ne") {
						code.push_back(make_unique<riscv::RegInstr>("xor", dest_reg, reg1, reg2));
						code.push_back(make_unique<riscv::RegInstr>("snez", dest_reg, dest_reg, ""));
					} else if (op == "eq") {
						code.push_back(make_unique<riscv::RegInstr>("xor", dest_reg, reg1, reg2));
						code.push_back(make_unique<riscv::RegInstr>("seqz", dest_reg, dest_reg, ""));
					} else if (op == "le") {
						code.push_back(make_unique<riscv::RegInstr>("sgt", dest_reg, reg1, reg2));
						code.push_back(make_unique<riscv::RegInstr>("seqz", dest_reg, dest_reg, ""));
					} else if (op == "ge") {
						code.push_back(make_unique<riscv::RegInstr>("slt", dest_reg, reg1, reg2));
						code.push_back(make_unique<riscv::RegInstr>("seqz", dest_reg, dest_reg, ""));
					} else {
						map<string, string> op_map = {
							{"lt", "slt"}, {"gt", "sgt"}, {"add", "add"}, {"sub", "sub"},
							{"mul", "mul"}, {"div", "div"}, {"mod", "rem"}, {"and", "and"},
							{"or", "or"}, {"xor", "xor"}, {"shl", "sll"}, {"shr", "srl"},
							{"sar", "sra"}};
						code.push_back(make_unique<riscv::RegInstr>(op_map[op], dest_reg, reg1, reg2));
					}
					StoreVar(dest_info, dest_reg);
				} else if (symb_def->def_type == koopa::FUNCALLDEF) {
					auto func_def = static_cast<koopa::FunCallDef*>(symb_def);
					auto fun_call = func_def->fun_call.get();
					for (int i = 12; i < 25; i++)
						if (reg_used[i])
							StoreOffset(reg_name[i], reg_offset[i]);
					ParseFunCall(fun_call, var_info, reg_used, reg_offset);
					for (int i = 12; i < 25; i++)
						if (i != 17 && reg_used[i])
							LoadOffset(reg_name[i], reg_offset[i]);
					if (dest_reg != "a0") {
						StoreVar(dest_info, "a0");
						if (reg_used[17])
							LoadOffset("a0", reg_offset[17]);
					}
				}
			} else if (stmt->stmt_type == koopa::STORESTMT) {
				auto store = static_cast<koopa::Store*>(stmt.get());
				assert(store->store_type == koopa::VALUESTORE);
				auto val_store = static_cast<koopa::ValueStore*>(store);
				string symb_dest = val_store->symbol;
				const VarInfo &dest_info = var_info[symb_dest];
				if (dest_info.var_def == ALLOCDEF) {
					string dest_reg = "t0";
					if (dest_info.reg >= 0)
						dest_reg = reg_name[dest_info.reg];
					string src_reg = LoadKoopaValue(val_store->val.get(), var_info, dest_reg);
					StoreVar(dest_info, src_reg);
				} else {
					string src_reg = LoadKoopaValue(val_store->val.get(), var_info, "t0");
					string addr_reg = LoadVar(dest_info, "t1");
					code.push_back(make_unique<riscv::ImmInstr>("sw", src_reg, addr_reg, 0));
				}
			} else if (stmt->stmt_type == koopa::FUNCALLSTMT) {
				auto fun_call = static_cast<koopa::FunCall*>(stmt.get());
				for (int i = 12; i < 25; i++)
					if (reg_used[i])
						StoreOffset(reg_name[i], reg_offset[i]);
				ParseFunCall(fun_call, var_info, reg_used, reg_offset);
				for (int i = 12; i < 25; i++)
					if (reg_used[i])
						LoadOffset(reg_name[i], reg_offset[i]);
			}
		}
		auto end_stmt = block->end_stmt.get();
		if (end_stmt->stmt_type == koopa::BRANCHEND) {
			auto branch = static_cast<koopa::Branch*>(end_stmt);
			string val_reg = LoadKoopaValue(branch->val.get(), var_info, "t0");
			code.push_back(make_unique<riscv::LabelInstr>("beqz", val_reg, branch->symbol2.substr(1)));
			code.push_back(make_unique<riscv::LabelInstr>("j", "", branch->symbol1.substr(1)));
		} else if (end_stmt->stmt_type == koopa::JUMPEND) {
			auto jump = static_cast<koopa::Jump*>(end_stmt);
			code.push_back(make_unique<riscv::LabelInstr>("j", "", jump->symbol.substr(1)));
		} else if (end_stmt->stmt_type == koopa::RETURNEND){
			auto ret = static_cast<koopa::Return*>(end_stmt);
			if (ret->val) {
				string val_reg = LoadKoopaValue(ret->val.get(), var_info, "a0");
				if (val_reg != "a0")
					code.push_back(make_unique<riscv::RegInstr>("mv", "a0", val_reg, ""));
			}
			code.push_back(make_unique<riscv::LabelInstr>("j", "", cur_return_label));
		}
	}
}

void ParseFunDef(koopa::FunDef *ptr) {
	string name = ptr->symbol.substr(1);
	code.push_back(make_unique<riscv::PseudoOp>("\t.text", ""));
	code.push_back(make_unique<riscv::PseudoOp>("\t.globl", name));
	code.push_back(make_unique<riscv::Label>(name));
	auto body = ptr->body.get();
	BuildBlockCFG(body);
	CutDeadBlocks(body);
	CutDeadVars(body);
	BuildStmtCFG(body);
	GetLiveVars(body);
	map<string, int> var_reg;
	AllocRegs(body, var_reg);
	map<string, VarInfo> var_info(global_var_info);
	GetVarType(ptr, var_info);
	int reg_used[25] = {};
	int reg_offset[25] = {};
	for (auto pr: var_reg) {
		if (pr.second >= 0)
			reg_used[pr.second] = 1;
		var_info[pr.first].reg = pr.second;
	}
	int has_call = 0;
	int ofst = GetFunOffset(ptr, var_info, reg_used, reg_offset, has_call);
	if (ofst <= 2048)
		code.push_back(make_unique<riscv::ImmInstr>("addi", "sp", "sp", -ofst));
	else {
		code.push_back(make_unique<riscv::ImmInstr>("li", "t0", "", ofst));
		code.push_back(make_unique<riscv::RegInstr>("sub", "sp", "sp", "t0"));
	}
	if (has_call)
		StoreOffset("ra", ofst - 4);
	for (int i = 0; i < 12; i++)
		if (reg_used[i])
			StoreOffset(reg_name[i], reg_offset[i]);
	cur_return_label = name + "_ret_" + to_string(return_counter++);
	ParseFunBody(ptr->body.get(), var_info, reg_used, reg_offset);
	code.push_back(make_unique<riscv::Label>(cur_return_label));
	for (int i = 0; i < 12; i++)
		if (reg_used[i])
			LoadOffset(reg_name[i], reg_offset[i]);
	if (has_call)
		LoadOffset("ra", ofst - 4);
	if (ofst < 2048)
		code.push_back(make_unique<riscv::ImmInstr>("addi", "sp", "sp", ofst));
	else {
		code.push_back(make_unique<riscv::ImmInstr>("li", "t0", "", ofst));
		code.push_back(make_unique<riscv::RegInstr>("add", "sp", "sp", "t0"));
	}
	code.push_back(make_unique<riscv::LabelInstr>("ret", "", ""));
	code.push_back(make_unique<riscv::PseudoOp>("", ""));
}

void UnpackAggregate(const koopa::Initializer *ptr, vector<int> &vals) {
	if (ptr->init_type == koopa::INTINIT) {
		auto int_init = static_cast<const koopa::IntInit*>(ptr);
		vals.push_back(int_init->integer);
	} else if (ptr->init_type == koopa::AGGREGATEINIT) {
		auto aggr_init = static_cast<const koopa::AggregateInit*>(ptr);
		for (auto &nxt: aggr_init->inits)
			UnpackAggregate(nxt.get(), vals);
	}
}

void ParseGlobalSymb(const koopa::GlobalSymbolDef *ptr) {
	string name = ptr->symbol.substr(1);
	code.push_back(make_unique<riscv::PseudoOp>("\t.data", ""));
	code.push_back(make_unique<riscv::PseudoOp>("\t.globl", name));
	code.push_back(make_unique<riscv::Label>(name));
	auto mem_type = ptr->mem_dec->mem_type;
	auto mem_init = ptr->mem_dec->mem_init.get();
	if (mem_init->init_type == koopa::ZEROINIT) {
		int len = mem_type->Size();
		code.push_back(make_unique<riscv::PseudoOp>("\t.zero", to_string(len)));
	} else {
		vector<int> init_vals;
		UnpackAggregate(mem_init, init_vals);
		for (int x: init_vals)
			code.push_back(make_unique<riscv::PseudoOp>("\t.word", to_string(x)));
	}
	code.push_back(make_unique<riscv::PseudoOp>("", ""));
	auto ptr_type = make_shared<koopa::PointerType>(mem_type);
	global_var_info.emplace(make_pair(ptr->symbol, VarInfo(name, GLOBALDEF, ptr_type)));
}

void CutDeadLoad() {
	vector<unique_ptr<riscv::Item> > new_code;
	riscv::Item *prev = nullptr;
	for (auto &ptr: code) {
		if (ptr->item_type == riscv::INSTR && prev && prev->item_type == riscv::INSTR) {
			auto instr1 = static_cast<riscv::Instr*>(prev);
			auto instr2 = static_cast<riscv::Instr*>(ptr.get());
			if (instr1->instr_type == riscv::IMMINSTR && instr2->instr_type == riscv::IMMINSTR) {
				auto imm1 = static_cast<riscv::ImmInstr*>(instr1);
				auto imm2 = static_cast<riscv::ImmInstr*>(instr2);
				if (imm1->op == "sw" && imm2->op == "lw" && imm1->imm == imm2->imm && imm1->rs == imm2->rs) {
					if (imm1->rd != imm2->rd) {
						new_code.push_back(make_unique<riscv::RegInstr>("mv", imm2->rd, imm1->rd, ""));
					}
					prev = ptr.get();
					continue;
				}
			}
		}
		prev = ptr.get();
		new_code.push_back(move(ptr));
	}
	code = move(new_code);
}

string ParseProgram(koopa::Program *ptr) {
	for (int i = 0; i < 12; i++)
		reg_name[i] = "s" + to_string(i);
	for (int i = 12; i < 17; i++)
		reg_name[i] = "t" + to_string(i-10);
	for (int i = 17; i < 25; i++)
		reg_name[i] = "a" + to_string(i-17);
	for (const auto &var: ptr->global_vars)
		ParseGlobalSymb(var.get());
	for (const auto &func: ptr->funcs)
		ParseFunDef(func.get());
	CutDeadLoad();
	string result;
	for (const auto &ptr: code)
		result += ptr->Str();
	return result;
}