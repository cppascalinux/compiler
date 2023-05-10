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
	auto load_stmt = make_unique<koopa::SymbolDefStmt>(move(load_def));
	stmts.push_back(move(load_stmt));
	return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter - 1));
}

void StoreSymb(string symb, unique_ptr<koopa::Value> val,
vector<unique_ptr<koopa::Statement> > &stmts) {
	auto store = make_unique<koopa::ValueStore>(move(val), symb);
	auto store_stmt = make_unique<koopa::StoreStmt>(move(store));
	stmts.push_back(move(store_stmt));
}

void AllocSymb(string symb,
vector<unique_ptr<koopa::Statement> > &stmts) {
	auto mem_alloc = make_unique<koopa::MemoryDec>(make_unique<koopa::IntType>());
	auto mem_def = make_unique<koopa::MemoryDef>(symb, move(mem_alloc));
	auto mem_stmt = make_unique<koopa::SymbolDefStmt>(move(mem_def));
	stmts.push_back(move(mem_stmt));
}

unique_ptr<koopa::Block> MakeKoopaBlock(
vector<unique_ptr<koopa::Statement> > &stmts,
unique_ptr<koopa::EndStatement> end_stmt) {
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
	auto new_stmt = make_unique<koopa::SymbolDefStmt>(move(new_symb_def));
	stmts.push_back(move(new_stmt));
	return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter - 1));
}

unique_ptr<koopa::Value> GetPrimExp(const unique_ptr<sysy::PrimaryExp> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->exp_type == sysy::BRACKETEXP) {
		auto new_ast = static_cast<sysy::BracketExp*>(ast.get());
		return GetExp(new_ast->exp, blocks, stmts);
	} else if (ast->exp_type == sysy::NUMBEREXP){
		auto new_ast = static_cast<sysy::NumberExp*>(ast.get());
		return make_unique<koopa::IntValue>(new_ast->num);
	} else {
		auto new_ast = static_cast<sysy::LValExp*>(ast.get());
		auto symb = symtab_stack.GetSymbol(new_ast->lval->ident);
		if (symb->symb_type == symtab::CONSTSYMB) {
			auto const_symb = static_cast<symtab::ConstSymb*>(symb);
			return make_unique<koopa::IntValue>(const_symb->val);
		} else {
			auto var_symb = static_cast<symtab::VarSymb*>(symb);
			return LoadSymb(var_symb->name, stmts);
		}
	}
	return nullptr;
}


unique_ptr<koopa::Value> GetUnaryExp(const unique_ptr<sysy::UnaryExp> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->exp_type == sysy::PRIMUNARYEXP) {
		auto new_ast = static_cast<sysy::PrimUnaryExp*>(ast.get());
		return GetPrimExp(new_ast->prim_exp, blocks, stmts);
	} else if (ast->exp_type == sysy::OPUNARYEXP) {
		auto new_ast = static_cast<sysy::OpUnaryExp*>(ast.get());
		map<string, string> unary2inst({{"+", "add"}, {"-", "sub"}, {"!", "eq"}});
		string inst = unary2inst[new_ast->op];
		const auto &exp = new_ast->exp;
		if (inst == "add")
			return GetUnaryExp(exp, blocks, stmts);
		else {
			auto ptr = GetUnaryExp(exp, blocks, stmts);
			auto new_zero_val = make_unique<koopa::IntValue>(0);
			auto new_bin_exp = make_unique<koopa::BinaryExpr>(
				inst, move(new_zero_val), move(ptr));
			return AddBinExp(move(new_bin_exp), stmts);
		}
	} else if (ast->exp_type == sysy::FUNCUNARYEXP) {
		auto new_ast = static_cast<sysy::FuncUnaryExp*>(ast.get());
		vector<unique_ptr<koopa::Value> > vals;
		for (auto &ptr: new_ast->params)
			vals.push_back(GetExp(ptr, blocks, stmts));
		auto fun_call = make_unique<koopa::FunCall>(
			"@" + new_ast->ident, move(vals));
		auto ptr = symtab_stack.GetSymbol(new_ast->ident);
		assert(ptr->symb_type == symtab::FUNCSYMB);
		auto new_ptr = static_cast<symtab::FuncSymb*>(ptr);
		if (new_ptr->ret_type == "int") {
			auto new_symb = make_unique<koopa::FunCallDef>(
				"%" + to_string(temp_var_counter++), move(fun_call));
			auto new_stmt = make_unique<koopa::SymbolDefStmt>(move(new_symb));
			stmts.push_back(move(new_stmt));
			return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter-1));
		} else {
			auto fun_stmt = make_unique<koopa::FunCallStmt>(move(fun_call));
			stmts.push_back(move(fun_stmt));
			return nullptr;
		}
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetMulExp(const unique_ptr<sysy::MulExp> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->mul_exp) {
		map<string, string> mul2inst({{"*", "mul"}, {"/", "div"}, {"%", "mod"}});
		string inst = mul2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetMulExp(ast->mul_exp, blocks, stmts), GetUnaryExp(ast->unary_exp, blocks, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetUnaryExp(ast->unary_exp, blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetAddExp(const unique_ptr<sysy::AddExp> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->add_exp) {
		map<string, string> add2inst({{"+", "add"}, {"-", "sub"}});
		string inst = add2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetAddExp(ast->add_exp, blocks, stmts), GetMulExp(ast->mul_exp, blocks, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetMulExp(ast->mul_exp, blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetRelExp(const unique_ptr<sysy::RelExp> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->rel_exp) {
		map<string, string> rel2inst({{"<", "lt"}, {">", "gt"}, {"<=", "le"}, {">=", "ge"}});
		string inst = rel2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetRelExp(ast->rel_exp, blocks, stmts), GetAddExp(ast->add_exp, blocks, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetAddExp(ast->add_exp, blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetEqExp(const unique_ptr<sysy::EqExp> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->eq_exp) {
		map<string, string> rel2inst({{"==", "eq"}, {"!=", "ne"}});
		string inst = rel2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetEqExp(ast->eq_exp, blocks, stmts), GetRelExp(ast->rel_exp, blocks, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetRelExp(ast->rel_exp, blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetLAndExp(const unique_ptr<sysy::LAndExp> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->land_exp) {
		string then_symb = "%shortcircuit_and_true_" + to_string(block_counter++);
		string else_symb = "%shortcircuit_and_false_" + to_string(block_counter++);
		string end_symb = "%shortcircuit_and_end_" + to_string(block_counter++);
		string ret_var = ("%" + to_string(temp_var_counter++));
		AllocSymb(ret_var, stmts);

		auto l_exp = GetLAndExp(ast->land_exp, blocks, stmts);
		auto l_0 = make_unique<koopa::IntValue>(0);
		auto l_logi = make_unique<koopa::BinaryExpr>("ne", move(l_0), move(l_exp));
		auto l_val = AddBinExp(move(l_logi), stmts);
		auto br = make_unique<koopa::Branch>(move(l_val), then_symb, else_symb);
		auto br_end = make_unique<koopa::BranchEnd>(move(br));
		blocks.push_back(MakeKoopaBlock(stmts, move(br_end)));

		next_block_symbol = then_symb;
		auto r_0 = make_unique<koopa::IntValue>(0);
		auto r_exp = GetEqExp(ast->eq_exp, blocks, stmts);
		auto r_logi = make_unique<koopa::BinaryExpr>("ne", move(r_0), move(r_exp));
		auto r_val = AddBinExp(move(r_logi), stmts);
		StoreSymb(ret_var, move(r_val), stmts);
		auto jmp = make_unique<koopa::Jump>(end_symb);
		auto jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));

		next_block_symbol = else_symb;
		auto val_0 = make_unique<koopa::IntValue>(0);
		StoreSymb(ret_var, move(val_0), stmts);
		jmp = make_unique<koopa::Jump>(end_symb);
		jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));

		next_block_symbol = end_symb;
		return LoadSymb(ret_var, stmts);
	} else
		return GetEqExp(ast->eq_exp, blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetLOrExp(const unique_ptr<sysy::LOrExp> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->lor_exp) {
		string then_symb = "%shortcircuit_or_true_" + to_string(block_counter++);
		string else_symb = "%shortcircuit_or_false_" + to_string(block_counter++);
		string end_symb = "%shortcircuit_or_end_" + to_string(block_counter++);
		string ret_var = ("%" + to_string(temp_var_counter++));
		AllocSymb(ret_var, stmts);

		auto l_exp = GetLOrExp(ast->lor_exp, blocks, stmts);
		auto l_0 = make_unique<koopa::IntValue>(0);
		auto l_logi = make_unique<koopa::BinaryExpr>("ne", move(l_0), move(l_exp));
		auto l_val = AddBinExp(move(l_logi), stmts);
		auto br = make_unique<koopa::Branch>(move(l_val), then_symb, else_symb);
		auto br_end = make_unique<koopa::BranchEnd>(move(br));
		blocks.push_back(MakeKoopaBlock(stmts, move(br_end)));

		next_block_symbol = then_symb;
		auto val_1 = make_unique<koopa::IntValue>(1);
		StoreSymb(ret_var, move(val_1), stmts);
		auto jmp = make_unique<koopa::Jump>(end_symb);
		auto jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));

		next_block_symbol = else_symb;
		auto r_0 = make_unique<koopa::IntValue>(0);
		auto r_exp = GetLAndExp(ast->land_exp, blocks, stmts);
		auto r_logi = make_unique<koopa::BinaryExpr>("ne", move(r_0), move(r_exp));
		auto r_val = AddBinExp(move(r_logi), stmts);
		StoreSymb(ret_var, move(r_val), stmts);
		jmp = make_unique<koopa::Jump>(end_symb);
		jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));

		next_block_symbol = end_symb;
		return LoadSymb(ret_var, stmts);
	} else
		return GetLAndExp(ast->land_exp, blocks, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetExp(const unique_ptr<sysy::Exp> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (!ast)
		return nullptr;
	return GetLOrExp(ast->exp, blocks, stmts);
}

void GetNonIfStmt(const unique_ptr<sysy::NonIfStmt> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->nonif_type == sysy::RETURNSTMT) {
		auto new_ast = static_cast<sysy::ReturnStmt*>(ast.get());
		auto ret = make_unique<koopa::Return>(GetExp(new_ast->exp, blocks, stmts));
		auto ret_end = make_unique<koopa::ReturnEnd>(move(ret));
		blocks.push_back(MakeKoopaBlock(stmts, move(ret_end)));
	} else if (ast->nonif_type == sysy::ASSIGNSTMT) {
		auto new_ast = static_cast<sysy::AssignStmt*>(ast.get());
		auto symb = symtab_stack.GetSymbol(new_ast->lval->ident);
		assert(symb->symb_type == symtab::VARSYMB);
		auto var_symb = static_cast<symtab::VarSymb*>(symb);
		auto rval = GetExp(new_ast->exp, blocks, stmts);
		StoreSymb(var_symb->name, move(rval), stmts);
	} else if (ast->nonif_type == sysy::EXPSTMT) {
		auto new_ast = static_cast<sysy::ExpStmt*>(ast.get());
		GetExp(new_ast->exp, blocks, stmts);
	} else if (ast->nonif_type == sysy::BLOCKSTMT) {
		auto new_ast = static_cast<sysy::BlockStmt*>(ast.get());
		GetBlock(new_ast->block, blocks, stmts);
	} else if (ast->nonif_type == sysy::WHILESTMT) {
		auto new_ast = static_cast<sysy::WhileStmt*>(ast.get());
		string begin_symb = "%while_begin_" + to_string(block_counter++);
		string body_symb = "%while_body_" + to_string(block_counter++);
		string end_symb = "%while_end_" + to_string(block_counter++);

		auto jmp = make_unique<koopa::Jump>(begin_symb);
		auto jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));

		next_block_symbol = begin_symb;
		auto exp = GetExp(new_ast->exp, blocks, stmts);
		auto br = make_unique<koopa::Branch>(move(exp), body_symb, end_symb);
		auto br_end = make_unique<koopa::BranchEnd>(move(br));
		blocks.push_back(MakeKoopaBlock(stmts, move(br_end)));

		next_block_symbol = body_symb;
		while_begin_stack.push_back(begin_symb);
		while_end_stack.push_back(end_symb);
		GetStmt(new_ast->stmt, blocks, stmts);
		while_begin_stack.pop_back();
		while_end_stack.pop_back();
		jmp = make_unique<koopa::Jump>(begin_symb);
		jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));

		next_block_symbol = end_symb;
	} else if (ast->nonif_type == sysy::BREAKSTMT) {
		auto jmp = make_unique<koopa::Jump>(while_end_stack.back());
		auto jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));
	} else if (ast->nonif_type == sysy::CONTINUESTMT) {
		auto jmp = make_unique<koopa::Jump>(while_begin_stack.back());
		auto jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));
	}
}

void GetClosedIf(const unique_ptr<sysy::ClosedIf> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->closed_type == sysy::NONIFCLOSEDIF) {
		auto nonif_ast = static_cast<sysy::NonIfClosedIf*>(ast.get());
		GetNonIfStmt(nonif_ast->stmt, blocks, stmts);
	} else {
		auto ifelse_ast = static_cast<sysy::IfElseClosedIf*>(ast.get());
		auto val = GetExp(ifelse_ast->exp, blocks, stmts);
		string then_symb = "%if_then_" + to_string(block_counter++);
		string else_symb = "%if_else_" + to_string(block_counter++);
		string end_symb = "%if_end_" + to_string(block_counter++);
		auto br = make_unique<koopa::Branch>(move(val), then_symb, else_symb);
		auto br_end = make_unique<koopa::BranchEnd>(move(br));
		blocks.push_back(MakeKoopaBlock(stmts, move(br_end)));
		next_block_symbol = then_symb;
		GetClosedIf(ifelse_ast->stmt1, blocks, stmts);
		auto jmp = make_unique<koopa::Jump>(end_symb);
		auto jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));
		next_block_symbol = else_symb;
		GetClosedIf(ifelse_ast->stmt2, blocks, stmts);
		jmp = make_unique<koopa::Jump>(end_symb);
		jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));
		next_block_symbol = end_symb;
	}
}

void GetOpenIf(const unique_ptr<sysy::OpenIf> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->open_type == sysy::IFOPENIF) {
		auto if_ast = static_cast<sysy::IfOpenIf*>(ast.get());
		auto val = GetExp(if_ast->exp, blocks, stmts);
		string then_symb = "%if_then_" + to_string(block_counter++);
		string end_symb = "%if_end" + to_string(block_counter++);
		auto br = make_unique<koopa::Branch>(move(val), then_symb, end_symb);
		auto br_end = make_unique<koopa::BranchEnd>(move(br));
		blocks.push_back(MakeKoopaBlock(stmts, move(br_end)));
		next_block_symbol = then_symb;
		GetStmt(if_ast->stmt, blocks, stmts);
		auto jmp = make_unique<koopa::Jump>(end_symb);
		auto jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));
		next_block_symbol = end_symb;
	} else {
		auto ifelse_ast = static_cast<sysy::IfElseOpenIf*>(ast.get());
		auto val = GetExp(ifelse_ast->exp, blocks, stmts);
		string then_symb = "%if_then_" + to_string(block_counter++);
		string else_symb = "%if_else_" + to_string(block_counter++);
		string end_symb = "%if_end_" + to_string(block_counter++);
		auto br = make_unique<koopa::Branch>(move(val), then_symb, else_symb);
		auto br_end = make_unique<koopa::BranchEnd>(move(br));
		blocks.push_back(MakeKoopaBlock(stmts, move(br_end)));
		next_block_symbol = then_symb;
		GetClosedIf(ifelse_ast->stmt1, blocks, stmts);
		auto jmp = make_unique<koopa::Jump>(end_symb);
		auto jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));
		next_block_symbol = else_symb;
		GetOpenIf(ifelse_ast->stmt2, blocks, stmts);
		jmp = make_unique<koopa::Jump>(end_symb);
		jmp_end = make_unique<koopa::JumpEnd>(move(jmp));
		blocks.push_back(MakeKoopaBlock(stmts, move(jmp_end)));
		next_block_symbol = end_symb;
	}
}

void GetStmt(const unique_ptr<sysy::Stmt> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->stmt_type == sysy::CLOSEDIF) {
		auto new_ast = static_cast<sysy::ClosedIf*>(ast.get());
		auto new_ptr = unique_ptr<sysy::ClosedIf>(new_ast);
		GetClosedIf(new_ptr, blocks, stmts);
		new_ptr.release();
	} else {
		auto new_ast = static_cast<sysy::OpenIf*>(ast.get());
		auto new_ptr = unique_ptr<sysy::OpenIf>(new_ast);
		GetOpenIf(new_ptr, blocks, stmts);
		new_ptr.release();
	}
}

void GetConstDef(const unique_ptr<sysy::ConstDef> &ast) {
	string ident = ast->ident;
	const auto &exp = ast->val->exp;
	int val = exp->Eval();
	auto new_symb = make_unique<symtab::ConstSymb>(val);
	symtab_stack.AddSymbol(ident, move(new_symb));
}

void GetVarDef(const unique_ptr<sysy::VarDef> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	string ident = ast->ident;
	string name = "@" + ident + "_" + to_string(symtab_stack.GetTotal());
	auto new_symb = make_unique<symtab::VarSymb>(name);
	AllocSymb(name, stmts);
	if(ast->init_val) {
		auto val = GetExp(ast->init_val->exp, blocks, stmts);
		StoreSymb(name, move(val), stmts);
	}
	symtab_stack.AddSymbol(ident, move(new_symb));
}

void GetBlockItem(const unique_ptr<sysy::BlockItem> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->item_type == sysy::STMTBLOCKITEM) {
		auto new_ast = static_cast<sysy::StmtBlockItem*>(ast.get());
		GetStmt(new_ast->stmt, blocks, stmts);
	} else {
		auto new_ast = static_cast<sysy::DeclBlockItem*>(ast.get());
		const auto &decl = new_ast->decl;
		if (decl->decl_type == sysy::CONSTDECL) {
			auto const_decl = static_cast<sysy::ConstDecl*>(decl.get());
			for (const auto &def: const_decl->defs)
				GetConstDef(def);
		} else {
			auto var_decl = static_cast<sysy::VarDecl*>(decl.get());
			for (const auto &def: var_decl->defs)
				GetVarDef(def, blocks, stmts);
		}
	}
}

void GetBlock(const unique_ptr<sysy::Block> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	symtab_stack.push();
	for (const auto &item: ast->items)
		GetBlockItem(item, blocks, stmts);
	symtab_stack.pop();
}

unique_ptr<koopa::FunBody> GetFunBody(const unique_ptr<sysy::Block> &ast,
vector<unique_ptr<koopa::Block> > &blocks,
vector<unique_ptr<koopa::Statement> > &stmts) {
	GetBlock(ast, blocks, stmts);
	if (!stmts.empty() || !next_block_symbol.empty() || blocks.empty()) {
		auto ret = make_unique<koopa::Return>(make_unique<koopa::IntValue>(0));
		if (cur_func_type == "void")
			ret = make_unique<koopa::Return>(nullptr);
		auto ret_stmt = make_unique<koopa::ReturnEnd>(move(ret));
		blocks.push_back(MakeKoopaBlock(stmts, move(ret_stmt)));
	}
	return make_unique<koopa::FunBody>(move(blocks));
}

unique_ptr<koopa::FunDef> GetFuncDef(const unique_ptr<sysy::FuncDef> &ast) {
	symtab_stack.AddSymbol(ast->ident, make_unique<symtab::FuncSymb>(ast->func_type));
	symtab_stack.push();
	vector<unique_ptr<koopa::Block> > blocks;
	vector<unique_ptr<koopa::Statement> > stmts;
	string symbol = "@" + ast->ident;
	unique_ptr<koopa::Type> ret_type = ast->func_type == "int" ?
		make_unique<koopa::IntType>() : nullptr;
	vector<pair<string, unique_ptr<koopa::Type> > > fun_params;
	for (const auto &ptr: ast->params) {
		string ident_name = ptr->ident + "_" + to_string(symtab_stack.GetTotal());
		fun_params.emplace_back("@" + ident_name, make_unique<koopa::IntType>());
		AllocSymb("%" + ident_name, stmts);
		StoreSymb("%" + ident_name,
			make_unique<koopa::SymbolValue>("@" + ident_name), stmts);
		auto new_symtab = make_unique<symtab::VarSymb>("%" + ident_name);
		symtab_stack.AddSymbol(ptr->ident, move(new_symtab));
	}
	auto koopa_params = make_unique<koopa::FunParams>(move(fun_params));
	cur_func_type = ast->func_type;
	auto fun_body = GetFunBody(ast->block, blocks, stmts);
	symtab_stack.pop();
	return make_unique<koopa::FunDef>(symbol, move(koopa_params), move(ret_type), move(fun_body));
}

void GetGlobalVarDef(const unique_ptr<sysy::VarDef> &ast,
vector<unique_ptr<koopa::GlobalSymbolDef> > &global_symbs) {
	int val;
	if (ast->init_val)
		val = ast->init_val->exp->Eval();
	else
		val = 0;
	auto val_init = make_unique<koopa::IntInit>(move(val));
	auto mem_dec = make_unique<koopa::GlobalMemDec>(
		make_unique<koopa::IntType>(), move(val_init));
	string name = "@" + ast->ident + "_" + to_string(symtab_stack.GetTotal());
	symtab_stack.AddSymbol(ast->ident, make_unique<symtab::VarSymb>(name));
	global_symbs.push_back(make_unique<koopa::GlobalSymbolDef>(name, move(mem_dec)));
}

void GetGlobalSymb(const unique_ptr<sysy::Decl> &ast,
vector<unique_ptr<koopa::GlobalSymbolDef> > &global_symbs) {
	if (ast->decl_type == sysy::CONSTDECL) {
		auto new_ast = static_cast<sysy::ConstDecl*>(ast.get());
		for (const auto &def: new_ast->defs)
			GetConstDef(def);
	} else {
		auto new_ast = static_cast<sysy::VarDecl*>(ast.get());
		for (const auto &def: new_ast->defs) {
			GetGlobalVarDef(def, global_symbs);
		}
	}
}

unique_ptr<koopa::Program> GetCompUnit(const unique_ptr<sysy::CompUnit> &ast) {
	vector<unique_ptr<koopa::FunDef> > funs;
	vector<unique_ptr<koopa::GlobalSymbolDef> > global_symbs;
	for (const auto &ptr: ast->items) {
		if (ptr->item_type == sysy::FUNCDEFITEM) {
			auto new_ptr = static_cast<sysy::FuncDefItem*>(ptr.get());
			funs.push_back(GetFuncDef(new_ptr->func_def));
		} else {
			auto new_ptr = static_cast<sysy::DeclItem*>(ptr.get());
			GetGlobalSymb(new_ptr->decl, global_symbs);
		}
	}
	return make_unique<koopa::Program>(move(global_symbs), move(funs));
}


