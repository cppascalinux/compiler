#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include <string>
#include <map>
#include "sysy.hpp"
#include "koopa.hpp"
#include "types.hpp"
#include "sysy2koopa.hpp"
#include "symtab.hpp"

using namespace std;

int temp_var_counter = 0;
int block_counter = 0;
string next_block_symbol;
vector<string> while_begin_stack, while_end_stack;
string cur_func_type;

unique_ptr<koopa::Value> LoadSymb(string symb,
vector<unique_ptr<koopa::Statement> > &stmts) {
	auto load = make_unique<koopa::Load>(symb);
	auto load_def = make_unique<koopa::LoadDef>(
		"%" + to_string(temp_var_counter++), move(load));
	stmts.push_back(move(load_def));
	return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter - 1));
}

void StoreSymb(string symb, unique_ptr<koopa::Value> val,
vector<unique_ptr<koopa::Statement> > &stmts) {
	auto store = make_unique<koopa::ValueStore>(move(val), symb);
	stmts.push_back(move(store));
}

void AllocSymb(string symb, shared_ptr<koopa::Type> type,
vector<unique_ptr<koopa::Statement> > &stmts) {
	auto mem_alloc = make_unique<koopa::MemoryDec>(type);
	auto mem_def = make_unique<koopa::MemoryDef>(symb, move(mem_alloc));
	stmts.push_back(move(mem_def));
}

unique_ptr<koopa::Block> MakeKoopaBlock(
vector<unique_ptr<koopa::Statement> > &stmts,
unique_ptr<koopa::Statement> end_stmt) {
	string symb;
	if (!next_block_symbol.empty()) {
		symb = next_block_symbol;
		next_block_symbol.clear();
	} else {
		symb = "%auto_" + to_string(block_counter++);
	}
	auto block = make_unique<koopa::Block>(symb, move(stmts), move(end_stmt));
	stmts.clear();
	return block;
}


unique_ptr<koopa::Value> AddBinExp(unique_ptr<koopa::BinaryExpr> bin_exp,
vector<unique_ptr<koopa::Statement> > &stmts) {
	auto new_symb_def = make_unique<koopa::BinExprDef>(
		"%" + to_string(temp_var_counter++), move(bin_exp));
	stmts.push_back(move(new_symb_def));
	return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter - 1));
}


unique_ptr<koopa::Value> GetLVal(const sysy::LVal *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	auto symb = symtab_stack.GetSymbol(ast->ident);
	if(symb->symb_type == symtab::CONSTSYMB) {
		auto const_symb = static_cast<const symtab::ConstSymb*>(symb);
		return make_unique<koopa::IntValue>(const_symb->val);
	}
	auto var_symb = static_cast<const symtab::VarSymb*>(symb);
	string ident = var_symb->name;
	if (var_symb->is_param && !ast->dims.empty())
	{	
		ident = "%" + to_string(temp_var_counter++);
		auto load = make_unique<koopa::Load>(var_symb->name);
		auto load_def = make_unique<koopa::LoadDef>(ident, move(load));
		stmts.push_back(move(load_def));
	}
	int is_p = var_symb->is_param;
	for (const auto &ptr: ast->dims) {
		string new_ident = "%" + to_string(temp_var_counter++);
		auto val = GetExp(ptr.get(), blocks, stmts);
		if (is_p-- > 0) {
			auto get_ptr = make_unique<koopa::GetPointer>(ident, move(val));
			auto ptr_def = make_unique<koopa::GetPtrDef>(new_ident, move(get_ptr));
			stmts.push_back(move(ptr_def));
		} else {
			auto get_elem = make_unique<koopa::GetElementPointer>(ident, move(val));
			auto ptr_def = make_unique<koopa::GetElemPtrDef>(new_ident, move(get_elem));
			stmts.push_back(move(ptr_def));
		}
		ident = new_ident;
	}
	if (var_symb->depth == ast->dims.size() || (var_symb->is_param && ast->dims.empty()))
		return LoadSymb(ident, stmts);
	else
	{
		string new_ident = "%" + to_string(temp_var_counter++);
		auto get_elem = make_unique<koopa::GetElementPointer>(ident, make_unique<koopa::IntValue>(0));
		auto ptr_def = make_unique<koopa::GetElemPtrDef>(new_ident, move(get_elem));
		stmts.push_back(move(ptr_def));
		return make_unique<koopa::SymbolValue>(new_ident);
	}
	return nullptr;
}

void StoreLVal(const sysy::LVal *ast,
unique_ptr<koopa::Value> val,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	auto symb = symtab_stack.GetSymbol(ast->ident);
	assert(symb->symb_type == symtab::VARSYMB);
	auto var_symb = static_cast<const symtab::VarSymb*>(symb);
	string ident = var_symb->name;
	if (var_symb->is_param && !ast->dims.empty())
	{	
		ident = "%" + to_string(temp_var_counter++);
		auto load = make_unique<koopa::Load>(var_symb->name);
		auto load_def = make_unique<koopa::LoadDef>(ident, move(load));
		stmts.push_back(move(load_def));
	}
	int is_p = var_symb->is_param;
	for (const auto &ptr: ast->dims) {
		string new_ident = "%" + to_string(temp_var_counter++);
		auto val = GetExp(ptr.get(), blocks, stmts);
		if (is_p-- > 0) {
			auto get_ptr = make_unique<koopa::GetPointer>(ident, move(val));
			auto ptr_def = make_unique<koopa::GetPtrDef>(new_ident, move(get_ptr));
			stmts.push_back(move(ptr_def));
		} else {
			auto get_elem = make_unique<koopa::GetElementPointer>(ident, move(val));
			auto ptr_def = make_unique<koopa::GetElemPtrDef>(new_ident, move(get_elem));
			stmts.push_back(move(ptr_def));
		}
		ident = new_ident;
	}
	assert(var_symb->depth == ast->dims.size());
	StoreSymb(ident, move(val), stmts);
}

unique_ptr<koopa::Value> GetPrimExp(const sysy::PrimaryExp *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->exp_type == sysy::BRACKETEXP) {
		auto new_ast = static_cast<const sysy::BracketExp*>(ast);
		return GetExp(new_ast->exp.get(), blocks, stmts);
	} else if (ast->exp_type == sysy::NUMBEREXP){
		auto new_ast = static_cast<const sysy::NumberExp*>(ast);
		return make_unique<koopa::IntValue>(new_ast->num);
	} else {
		auto new_ast = static_cast<const sysy::LValExp*>(ast);
		return GetLVal(new_ast->lval.get(), blocks, stmts);
	}
	return nullptr;
}


unique_ptr<koopa::Value> GetUnaryExp(const sysy::UnaryExp *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->exp_type == sysy::PRIMUNARYEXP) {
		auto new_ast = static_cast<const sysy::PrimUnaryExp*>(ast);
		return GetPrimExp(new_ast->prim_exp.get(), blocks, stmts);
	} else if (ast->exp_type == sysy::OPUNARYEXP) {
		auto new_ast = static_cast<const sysy::OpUnaryExp*>(ast);
		map<string, string> unary2inst({{"+", "add"}, {"-", "sub"}, {"!", "eq"}});
		string inst = unary2inst[new_ast->op];
		const auto &exp = new_ast->exp;
		if (inst == "add")
			return GetUnaryExp(exp.get(), blocks, stmts);
		else {
			auto ptr = GetUnaryExp(exp.get(), blocks, stmts);
			auto new_zero_val = make_unique<koopa::IntValue>(0);
			auto new_bin_exp = make_unique<koopa::BinaryExpr>(
				inst, move(new_zero_val), move(ptr));
			return AddBinExp(move(new_bin_exp), stmts);
		}
	} else if (ast->exp_type == sysy::FUNCUNARYEXP) {
		auto new_ast = static_cast<const sysy::FuncUnaryExp*>(ast);
		auto ptr = symtab_stack.GetSymbol(new_ast->ident);
		auto new_ptr = static_cast<const symtab::FuncSymb*>(ptr);
		vector<unique_ptr<koopa::Value> > vals;
		for (auto &prm: new_ast->params)
			vals.push_back(GetExp(prm.get(), blocks, stmts));
		auto fun_call = make_unique<koopa::FunCall>(
			"@" + new_ast->ident, move(vals));
		assert(ptr->symb_type == symtab::FUNCSYMB);
		if (new_ptr->is_int) {
			auto new_symb = make_unique<koopa::FunCallDef>(
				"%" + to_string(temp_var_counter++), move(fun_call));
			stmts.push_back(move(new_symb));
			return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter-1));
		} else {
			stmts.push_back(move(fun_call));
			return nullptr;
		}
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetMulExp(const sysy::MulExp *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->mul_exp) {
		map<string, string> mul2inst({{"*", "mul"}, {"/", "div"}, {"%", "mod"}});
		string inst = mul2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetMulExp(ast->mul_exp.get(), blocks,
			stmts), GetUnaryExp(ast->unary_exp.get(), blocks, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetUnaryExp(ast->unary_exp.get(), blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetAddExp(const sysy::AddExp *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->add_exp) {
		map<string, string> add2inst({{"+", "add"}, {"-", "sub"}});
		string inst = add2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetAddExp(ast->add_exp.get(), blocks, stmts),
			GetMulExp(ast->mul_exp.get(), blocks, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetMulExp(ast->mul_exp.get(), blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetRelExp(const sysy::RelExp *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->rel_exp) {
		map<string, string> rel2inst({{"<", "lt"}, {">", "gt"}, {"<=", "le"}, {">=", "ge"}});
		string inst = rel2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetRelExp(ast->rel_exp.get(), blocks, stmts),
			GetAddExp(ast->add_exp.get(), blocks, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetAddExp(ast->add_exp.get(), blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetEqExp(const sysy::EqExp *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->eq_exp) {
		map<string, string> rel2inst({{"==", "eq"}, {"!=", "ne"}});
		string inst = rel2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetEqExp(ast->eq_exp.get(), blocks, stmts),
			GetRelExp(ast->rel_exp.get(), blocks, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetRelExp(ast->rel_exp.get(), blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetLAndExp(const sysy::LAndExp *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->land_exp) {
		string then_symb = "%shortcircuit_and_true_" + to_string(block_counter++);
		string else_symb = "%shortcircuit_and_false_" + to_string(block_counter++);
		string end_symb = "%shortcircuit_and_end_" + to_string(block_counter++);
		string ret_var = ("%" + to_string(temp_var_counter++));
		AllocSymb(ret_var, make_shared<koopa::IntType>(), stmts);

		auto l_exp = GetLAndExp(ast->land_exp.get(), blocks, stmts);
		auto l_0 = make_unique<koopa::IntValue>(0);
		auto l_logi = make_unique<koopa::BinaryExpr>("ne", move(l_0), move(l_exp));
		auto l_val = AddBinExp(move(l_logi), stmts);
		auto br = make_unique<koopa::Branch>(move(l_val), then_symb, else_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(br)));

		next_block_symbol = then_symb;
		auto r_0 = make_unique<koopa::IntValue>(0);
		auto r_exp = GetEqExp(ast->eq_exp.get(), blocks, stmts);
		auto r_logi = make_unique<koopa::BinaryExpr>("ne", move(r_0), move(r_exp));
		auto r_val = AddBinExp(move(r_logi), stmts);
		StoreSymb(ret_var, move(r_val), stmts);
		auto jmp = make_unique<koopa::Jump>(end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));

		next_block_symbol = else_symb;
		auto val_0 = make_unique<koopa::IntValue>(0);
		StoreSymb(ret_var, move(val_0), stmts);
		jmp = make_unique<koopa::Jump>(end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));

		next_block_symbol = end_symb;
		return LoadSymb(ret_var, stmts);
	} else
		return GetEqExp(ast->eq_exp.get(), blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetLOrExp(const sysy::LOrExp *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->lor_exp) {
		string then_symb = "%shortcircuit_or_true_" + to_string(block_counter++);
		string else_symb = "%shortcircuit_or_false_" + to_string(block_counter++);
		string end_symb = "%shortcircuit_or_end_" + to_string(block_counter++);
		string ret_var = ("%" + to_string(temp_var_counter++));
		AllocSymb(ret_var, make_shared<koopa::IntType>(), stmts);

		auto l_exp = GetLOrExp(ast->lor_exp.get(), blocks, stmts);
		auto l_0 = make_unique<koopa::IntValue>(0);
		auto l_logi = make_unique<koopa::BinaryExpr>("ne", move(l_0), move(l_exp));
		auto l_val = AddBinExp(move(l_logi), stmts);
		auto br = make_unique<koopa::Branch>(move(l_val), then_symb, else_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(br)));

		next_block_symbol = then_symb;
		auto val_1 = make_unique<koopa::IntValue>(1);
		StoreSymb(ret_var, move(val_1), stmts);
		auto jmp = make_unique<koopa::Jump>(end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));

		next_block_symbol = else_symb;
		auto r_0 = make_unique<koopa::IntValue>(0);
		auto r_exp = GetLAndExp(ast->land_exp.get(), blocks, stmts);
		auto r_logi = make_unique<koopa::BinaryExpr>("ne", move(r_0), move(r_exp));
		auto r_val = AddBinExp(move(r_logi), stmts);
		StoreSymb(ret_var, move(r_val), stmts);
		jmp = make_unique<koopa::Jump>(end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));

		next_block_symbol = end_symb;
		return LoadSymb(ret_var, stmts);
	} else
		return GetLAndExp(ast->land_exp.get(), blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetExp(const sysy::Exp *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (!ast)
		return nullptr;
	return GetLOrExp(ast->exp.get(), blocks, stmts);
}

void GetNonIfStmt(const sysy::NonIfStmt *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->nonif_type == sysy::RETURNSTMT) {
		auto new_ast = static_cast<const sysy::ReturnStmt*>(ast);
		auto ret = make_unique<koopa::Return>(GetExp(new_ast->exp.get(), blocks, stmts));
		blocks.push_back(MakeKoopaBlock(stmts, move(ret)));
	} else if (ast->nonif_type == sysy::ASSIGNSTMT) {
		auto new_ast = static_cast<const sysy::AssignStmt*>(ast);
		auto val = GetExp(new_ast->exp.get(), blocks, stmts);
		StoreLVal(new_ast->lval.get(), move(val), blocks, stmts);
	} else if (ast->nonif_type == sysy::EXPSTMT) {
		auto new_ast = static_cast<const sysy::ExpStmt*>(ast);
		GetExp(new_ast->exp.get(), blocks, stmts);
	} else if (ast->nonif_type == sysy::BLOCKSTMT) {
		auto new_ast = static_cast<const sysy::BlockStmt*>(ast);
		GetBlock(new_ast->block.get(), blocks, stmts);
	} else if (ast->nonif_type == sysy::WHILESTMT) {
		auto new_ast = static_cast<const sysy::WhileStmt*>(ast);
		string begin_symb = "%while_begin_" + to_string(block_counter++);
		string body_symb = "%while_body_" + to_string(block_counter++);
		string end_symb = "%while_end_" + to_string(block_counter++);

		auto jmp = make_unique<koopa::Jump>(begin_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));

		next_block_symbol = begin_symb;
		auto exp = GetExp(new_ast->exp.get(), blocks, stmts);
		auto br = make_unique<koopa::Branch>(move(exp), body_symb, end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(br)));

		next_block_symbol = body_symb;
		while_begin_stack.push_back(begin_symb);
		while_end_stack.push_back(end_symb);
		GetStmt(new_ast->stmt.get(), blocks, stmts);
		while_begin_stack.pop_back();
		while_end_stack.pop_back();
		jmp = make_unique<koopa::Jump>(begin_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));

		next_block_symbol = end_symb;
	} else if (ast->nonif_type == sysy::BREAKSTMT) {
		auto jmp = make_unique<koopa::Jump>(while_end_stack.back());
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));
	} else if (ast->nonif_type == sysy::CONTINUESTMT) {
		auto jmp = make_unique<koopa::Jump>(while_begin_stack.back());
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));
	}
}

void GetClosedIf(const sysy::ClosedIf *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->closed_type == sysy::NONIFCLOSEDIF) {
		auto nonif_ast = static_cast<const sysy::NonIfClosedIf*>(ast);
		GetNonIfStmt(nonif_ast->stmt.get(), blocks, stmts);
	} else {
		auto ifelse_ast = static_cast<const sysy::IfElseClosedIf*>(ast);
		auto val = GetExp(ifelse_ast->exp.get(), blocks, stmts);
		string then_symb = "%if_then_" + to_string(block_counter++);
		string else_symb = "%if_else_" + to_string(block_counter++);
		string end_symb = "%if_end_" + to_string(block_counter++);
		auto br = make_unique<koopa::Branch>(move(val), then_symb, else_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(br)));
		next_block_symbol = then_symb;
		GetClosedIf(ifelse_ast->stmt1.get(), blocks, stmts);
		auto jmp = make_unique<koopa::Jump>(end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));
		next_block_symbol = else_symb;
		GetClosedIf(ifelse_ast->stmt2.get(), blocks, stmts);
		jmp = make_unique<koopa::Jump>(end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));
		next_block_symbol = end_symb;
	}
}

void GetOpenIf(const sysy::OpenIf *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->open_type == sysy::IFOPENIF) {
		auto if_ast = static_cast<const sysy::IfOpenIf*>(ast);
		auto val = GetExp(if_ast->exp.get(), blocks, stmts);
		string then_symb = "%if_then_" + to_string(block_counter++);
		string end_symb = "%if_end" + to_string(block_counter++);
		auto br = make_unique<koopa::Branch>(move(val), then_symb, end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(br)));
		next_block_symbol = then_symb;
		GetStmt(if_ast->stmt.get(), blocks, stmts);
		auto jmp = make_unique<koopa::Jump>(end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));
		next_block_symbol = end_symb;
	} else {
		auto ifelse_ast = static_cast<const sysy::IfElseOpenIf*>(ast);
		auto val = GetExp(ifelse_ast->exp.get(), blocks, stmts);
		string then_symb = "%if_then_" + to_string(block_counter++);
		string else_symb = "%if_else_" + to_string(block_counter++);
		string end_symb = "%if_end_" + to_string(block_counter++);
		auto br = make_unique<koopa::Branch>(move(val), then_symb, else_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(br)));
		next_block_symbol = then_symb;
		GetClosedIf(ifelse_ast->stmt1.get(), blocks, stmts);
		auto jmp = make_unique<koopa::Jump>(end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));
		next_block_symbol = else_symb;
		GetOpenIf(ifelse_ast->stmt2.get(), blocks, stmts);
		jmp = make_unique<koopa::Jump>(end_symb);
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp)));
		next_block_symbol = end_symb;
	}
}

void GetStmt(const sysy::Stmt *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->stmt_type == sysy::CLOSEDIF) {
		auto new_ast = static_cast<const sysy::ClosedIf*>(ast);
		GetClosedIf(new_ast, blocks, stmts);
	} else {
		auto new_ast = static_cast<const sysy::OpenIf*>(ast);
		GetOpenIf(new_ast, blocks, stmts);
	}
}

void GetConstInitVal(const sysy::InitVal *init_val,
vector<int> suf_mul, vector<int> &result) {
	auto list_val = static_cast<const sysy::ListInitVal*>(init_val);
	int cur_pos = 0;
	vector<unique_ptr<koopa::Initializer> > v;
	for (const auto &ptr: list_val->inits) {
		if (ptr->init_type == sysy::EXPINITVAL) {
			auto exp_ptr = static_cast<const sysy::ExpInitVal*>(ptr.get());
			result.push_back(exp_ptr->exp->Eval());
			cur_pos++;
		} else {
			for (int i = 1; i + 1 < suf_mul.size(); i++) {
				if (cur_pos % suf_mul[i] == 0) {
					vector<int> new_suf(suf_mul.begin() + i, suf_mul.end());
					GetConstInitVal(ptr.get(), new_suf, result);
					cur_pos += suf_mul[i];
					break;
				}
			}
		}
	}
	for(; cur_pos < suf_mul[0]; cur_pos++)
		result.push_back(0);
}

void GetInitVal(const sysy::InitVal *init_val,
vector<int> suf_mul, vector<unique_ptr<koopa::Value> > &result,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	auto list_val = static_cast<const sysy::ListInitVal*>(init_val);
	int cur_pos = 0;
	vector<unique_ptr<koopa::Initializer> > v;
	for (const auto &ptr: list_val->inits) {
		if (ptr->init_type == sysy::EXPINITVAL) {
			auto exp_ptr = static_cast<const sysy::ExpInitVal*>(ptr.get());
			result.push_back(GetExp(exp_ptr->exp.get(), blocks, stmts));
			cur_pos++;
		} else {
			for (int i = 1; i + 1 < suf_mul.size(); i++) {
				if (cur_pos % suf_mul[i] == 0) {
					vector<int> new_suf(suf_mul.begin() + i, suf_mul.end());
					GetInitVal(ptr.get(), new_suf, result, blocks, stmts);
					cur_pos += suf_mul[i];
					break;
				}
			}
		}
	}
	for(; cur_pos < suf_mul[0]; cur_pos++)
		result.push_back(make_unique<koopa::IntValue>(0));
}

std::unique_ptr<koopa::Initializer> MakeKoopaInit(
vector<int> lin_init, vector<int> suf_mul) {
	if(suf_mul.size() == 1)
		return make_unique<koopa::IntInit>(lin_init[0]);
	int n1 = suf_mul[0] / suf_mul[1];
	vector<int> new_suf(suf_mul.begin() + 1, suf_mul.end());
	vector<unique_ptr<koopa::Initializer> > inits;
	for(int i = 0; i < n1; i++) {
		vector<int> new_lin(lin_init.begin() + i * suf_mul[1],
			lin_init.begin() + (i + 1) * suf_mul[1]);
		inits.push_back(MakeKoopaInit(new_lin, new_suf));
	}
	return make_unique<koopa::AggregateInit>(move(inits));
}

std::unique_ptr<koopa::Initializer> KoopaInitWith0(
vector<int> lin_init, vector<int> suf_mul) {
	int all_zero = 0;
	for (int x: lin_init)
		all_zero |= x;
	if (!all_zero)
		return make_unique<koopa::ZeroInit>();
	else
		return MakeKoopaInit(lin_init, suf_mul);
}

void GenArrayStore(vector<unique_ptr<koopa::Value> > lin_init, vector<int> suf_mul,
string cur_symb, vector<unique_ptr<koopa::Statement> > &stmts) {
	if (suf_mul.size() == 1) {
		auto store = make_unique<koopa::ValueStore>(move(lin_init[0]), cur_symb);
		stmts.push_back(move(store));
	} else {
		int n1 = suf_mul[0] / suf_mul[1];
		vector<int> new_suf = vector<int>(suf_mul.begin() + 1, suf_mul.end());
		for (int i = 0; i < n1; i++) {
			string new_symb = "%" + to_string(temp_var_counter++);
			auto i_val = make_unique<koopa::IntValue>(i);
			auto get_elem = make_unique<koopa::GetElementPointer>(cur_symb, move(i_val));
			auto symb_def = make_unique<koopa::GetElemPtrDef>(new_symb, move(get_elem));
			stmts.push_back(move(symb_def));
			vector<unique_ptr<koopa::Value> > new_lin;
			for(auto it = lin_init.begin() + i * suf_mul[1]; 
			it != lin_init.begin() + (i + 1) * suf_mul[1]; it++)
				new_lin.push_back(move(*it));
			GenArrayStore(move(new_lin), new_suf, new_symb, stmts);
		}
	}
}

shared_ptr<koopa::Type> Dims2Type(vector<int> num_dims) {
	shared_ptr<koopa::Type> type = make_shared<koopa::IntType>();
	for (auto it = num_dims.rbegin(); it != num_dims.rend(); it++)
		type = make_unique<koopa::ArrayType>(type, *it);
	return type;
}

void GetConstDef(const sysy::ConstDef *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->dims.empty()) {
		const auto &init = ast->init_val;
		assert(init->init_type == sysy::EXPINITVAL);
		auto exp_init = static_cast<const sysy::ExpInitVal*>(init.get());
		int val = exp_init->exp->Eval();
		auto new_symb = make_unique<symtab::ConstSymb>(val);
		symtab_stack.AddSymbol(ast->ident, move(new_symb));
	} else {
		string name = "@" + ast->ident + "_" + to_string(symtab_stack.GetTotal());
		vector<int> num_dims;
		for (const auto &exp: ast->dims)
			num_dims.push_back(exp->Eval());
		AllocSymb(name, Dims2Type(num_dims), stmts);
		vector<int> suf_mul(num_dims);
		for(auto it = suf_mul.rbegin() + 1; it != suf_mul.rend(); it++)
			*it *= *(it-1);
		suf_mul.push_back(1);
		vector<int> lin_init;
		GetConstInitVal(ast->init_val.get(), suf_mul, lin_init);
		vector<unique_ptr<koopa::Value> > new_lin;
		for (int x: lin_init)
			new_lin.push_back(make_unique<koopa::IntValue>(x));
		GenArrayStore(move(new_lin), suf_mul, name, stmts);
		auto new_symb = make_unique<symtab::VarSymb>(name, 0, num_dims.size());
		symtab_stack.AddSymbol(ast->ident, move(new_symb));
	}
}

void GetVarDef(const sysy::VarDef *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	string name = "@" + ast->ident + "_" + to_string(symtab_stack.GetTotal());
	if (ast->dims.empty()) {
		AllocSymb(name, make_shared<koopa::IntType>(), stmts);
		if(ast->init_val) {
			const auto &init = ast->init_val;
			assert(init->init_type == sysy::EXPINITVAL);
			auto exp_init = static_cast<const sysy::ExpInitVal*>(init.get());
			auto val = GetExp(exp_init->exp.get(), blocks, stmts);
			StoreSymb(name, move(val), stmts);
		}
		auto new_symb = make_unique<symtab::VarSymb>(name, 0, 0);
		symtab_stack.AddSymbol(ast->ident, move(new_symb));
	} else {
		vector<int> num_dims;
		for (const auto &exp: ast->dims)
			num_dims.push_back(exp->Eval());
		AllocSymb(name, Dims2Type(num_dims), stmts);
		if(ast->init_val) {
			vector<int> suf_mul(num_dims);
			for(auto it = suf_mul.rbegin() + 1; it != suf_mul.rend(); it++)
				*it *= *(it-1);
			suf_mul.push_back(1);
			vector<unique_ptr<koopa::Value> > lin_init;
			GetInitVal(ast->init_val.get(), suf_mul, lin_init, blocks, stmts);
			GenArrayStore(move(lin_init), suf_mul, name, stmts);
		}
		auto new_symb = make_unique<symtab::VarSymb>(name, 0, num_dims.size());
		symtab_stack.AddSymbol(ast->ident, move(new_symb));
	}
}

void GetBlockItem(const sysy::BlockItem *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->item_type == sysy::STMTBLOCKITEM) {
		auto new_ast = static_cast<const sysy::StmtBlockItem*>(ast);
		GetStmt(new_ast->stmt.get(), blocks, stmts);
	} else {
		auto new_ast = static_cast<const sysy::DeclBlockItem*>(ast);
		const auto &decl = new_ast->decl;
		if (decl->decl_type == sysy::CONSTDECL) {
			auto const_decl = static_cast<const sysy::ConstDecl*>(decl.get());
			for (const auto &def: const_decl->defs)
				GetConstDef(def.get(), blocks, stmts);
		} else {
			auto var_decl = static_cast<const sysy::VarDecl*>(decl.get());
			for (const auto &def: var_decl->defs)
				GetVarDef(def.get(), blocks, stmts);
		}
	}
}

void GetBlock(const sysy::Block *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	symtab_stack.push();
	for (const auto &item: ast->items)
		GetBlockItem(item.get(), blocks, stmts);
	symtab_stack.pop();
}

unique_ptr<koopa::FunBody> GetFunBody(const sysy::Block *ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	GetBlock(ast, blocks, stmts);
	if (!stmts.empty() || !next_block_symbol.empty() || blocks.empty()) {
		auto ret = make_unique<koopa::Return>(make_unique<koopa::IntValue>(0));
		if (cur_func_type == "void")
			ret = make_unique<koopa::Return>(nullptr);
		blocks.push_back(MakeKoopaBlock(stmts, move(ret)));
	}
	return make_unique<koopa::FunBody>(move(blocks));
}

unique_ptr<koopa::FunDef> GetFuncDef(const sysy::FuncDef *ast) {
	symtab_stack.AddSymbol(ast->ident, make_unique<symtab::FuncSymb>(ast->func_type == "int"));
	symtab_stack.push();
	vector<unique_ptr<koopa::Block> > blocks;
	vector<unique_ptr<koopa::Statement> > stmts;
	string symbol = "@" + ast->ident;
	shared_ptr<koopa::Type> ret_type = ast->func_type == "int" ?
		make_shared<koopa::IntType>() : nullptr;
	vector<pair<string, shared_ptr<koopa::Type> > > fun_params;
	for (const auto &ptr: ast->params) {
		string ident_name = ptr->ident + "_" + to_string(symtab_stack.GetTotal());
		if(ptr->dims.empty()) {
			auto type = make_shared<koopa::IntType>();
			fun_params.emplace_back("@" + ident_name, type);
			AllocSymb("%" + ident_name, type, stmts);
			StoreSymb("%" + ident_name,
				make_unique<koopa::SymbolValue>("@" + ident_name), stmts);
			auto new_symtab = make_unique<symtab::VarSymb>("%" + ident_name, 1, 0);
			symtab_stack.AddSymbol(ptr->ident, move(new_symtab));
		} else {
			vector<int> num_dims;
			for (const auto &dim: ptr->dims)
				if (dim)
					num_dims.push_back(dim->exp->Eval());
			auto type = make_shared<koopa::PointerType>(Dims2Type(num_dims));
			fun_params.emplace_back("@" + ident_name, type);
			AllocSymb("%" + ident_name, type, stmts);
			StoreSymb("%" + ident_name,
				make_unique<koopa::SymbolValue>("@" + ident_name), stmts);
			auto new_symtab = make_unique<symtab::VarSymb>("%" + ident_name, 1, num_dims.size() + 1);
			symtab_stack.AddSymbol(ptr->ident, move(new_symtab));
		}
	}
	auto koopa_params = make_unique<koopa::FunParams>(move(fun_params));
	cur_func_type = ast->func_type;
	auto fun_body = GetFunBody(ast->block.get(), blocks, stmts);
	symtab_stack.pop();
	return make_unique<koopa::FunDef>(symbol, move(koopa_params), move(ret_type), move(fun_body));
}

void GetGlobalConstDef(const sysy::ConstDef *ast,
vector<unique_ptr<koopa::GlobalSymbolDef> > &global_symbs) {
	if (ast->dims.empty()) {
		const auto &init = ast->init_val;
		assert(init->init_type == sysy::EXPINITVAL);
		auto exp_init = static_cast<const sysy::ExpInitVal*>(init.get());
		int val = exp_init->exp->Eval();
		auto new_symb = make_unique<symtab::ConstSymb>(val);
		symtab_stack.AddSymbol(ast->ident, move(new_symb));
	} else {
		vector<int> num_dims;
		for (const auto &exp: ast->dims)
			num_dims.push_back(exp->Eval());
		vector<int> suf_mul(num_dims);
		for(auto it = suf_mul.rbegin() + 1; it != suf_mul.rend(); it++)
			*it *= *(it-1);
		suf_mul.push_back(1);
		vector<int> lin_init;
		if(ast->init_val)
			GetConstInitVal(ast->init_val.get(), suf_mul, lin_init);
		else
			lin_init = vector<int>(suf_mul[0], 0);
		auto init = KoopaInitWith0(lin_init, suf_mul);
		auto mem_dec = make_unique<koopa::GlobalMemDec>(Dims2Type(num_dims), move(init));
		string name = "@" + ast->ident + "_" + to_string(symtab_stack.GetTotal());
		symtab_stack.AddSymbol(ast->ident, make_unique<symtab::VarSymb>(name, 0, num_dims.size()));
		global_symbs.push_back(make_unique<koopa::GlobalSymbolDef>(name, move(mem_dec)));
	}
}

void GetGlobalVarDef(const sysy::VarDef *ast,
vector<unique_ptr<koopa::GlobalSymbolDef> > &global_symbs) {
	if (ast->dims.empty()) {
		int val;
		if (ast->init_val)
		{
			assert(ast->init_val->init_type == sysy::EXPINITVAL);
			auto new_init = static_cast<const sysy::ExpInitVal*>(ast->init_val.get());
			val = new_init->exp->Eval();
		}
		else
			val = 0;
		unique_ptr<koopa::Initializer> init;
		if (val)
			init = make_unique<koopa::IntInit>(val);
		else
			init = make_unique<koopa::ZeroInit>();
		auto mem_dec = make_unique<koopa::GlobalMemDec>(
			make_shared<koopa::IntType>(), move(init));
		string name = "@" + ast->ident + "_" + to_string(symtab_stack.GetTotal());
		symtab_stack.AddSymbol(ast->ident, make_unique<symtab::VarSymb>(name, 0, 0));
		global_symbs.push_back(make_unique<koopa::GlobalSymbolDef>(name, move(mem_dec)));
	} else {
		vector<int> num_dims;
		for (const auto &exp: ast->dims)
			num_dims.push_back(exp->Eval());
		vector<int> suf_mul(num_dims);
		for(auto it = suf_mul.rbegin() + 1; it != suf_mul.rend(); it++)
			*it *= *(it-1);
		suf_mul.push_back(1);
		vector<int> lin_init;
		unique_ptr<koopa::Initializer> init;
		if(ast->init_val)
		{
			GetConstInitVal(ast->init_val.get(), suf_mul, lin_init);
			init = KoopaInitWith0(lin_init, suf_mul);;
		}
		else
			init = make_unique<koopa::ZeroInit>();
		auto mem_dec = make_unique<koopa::GlobalMemDec>(Dims2Type(num_dims), move(init));
		string name = "@" + ast->ident + "_" + to_string(symtab_stack.GetTotal());
		symtab_stack.AddSymbol(ast->ident, make_unique<symtab::VarSymb>(name, 0, num_dims.size()));
		global_symbs.push_back(make_unique<koopa::GlobalSymbolDef>(name, move(mem_dec)));
	}
}

void GetGlobalSymb(const sysy::Decl *ast,
vector<unique_ptr<koopa::GlobalSymbolDef> > &global_symbs) {
	if (ast->decl_type == sysy::CONSTDECL) {
		auto new_ast = static_cast<const sysy::ConstDecl*>(ast);
		for (const auto &def: new_ast->defs)
			GetGlobalConstDef(def.get(), global_symbs);
	} else {
		auto new_ast = static_cast<const sysy::VarDecl*>(ast);
		for (const auto &def: new_ast->defs) {
			GetGlobalVarDef(def.get(), global_symbs);
		}
	}
}

unique_ptr<koopa::Program> GetCompUnit(const sysy::CompUnit *ast) {
	vector<unique_ptr<koopa::FunDef> > funs;
	vector<unique_ptr<koopa::GlobalSymbolDef> > global_symbs;
	for (const auto &ptr: ast->items) {
		if (ptr->item_type == sysy::FUNCDEFITEM) {
			auto new_ptr = static_cast<const sysy::FuncDefItem*>(ptr.get());
			funs.push_back(GetFuncDef(new_ptr->func_def.get()));
		} else {
			auto new_ptr = static_cast<const sysy::DeclItem*>(ptr.get());
			GetGlobalSymb(new_ptr->decl.get(), global_symbs);
		}
	}
	return make_unique<koopa::Program>(move(global_symbs), move(funs));
}


