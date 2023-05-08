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

int temp_var_counter=0;

unique_ptr<koopa::Value> LoadSymb(const symtab::VarSymb *var_symb,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	auto load = make_unique<koopa::Load>(var_symb->name);
	auto load_def = make_unique<koopa::LoadDef>(
		"%" + to_string(temp_var_counter++), move(load));
	auto load_stmt = make_unique<koopa::SymbolDefStmt>(move(load_def));
	stmts.push_back(move(load_stmt));
	return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter - 1));
}

void StoreSymb(const symtab::VarSymb *var_symb,
	unique_ptr<koopa::Value> val, vector<unique_ptr<koopa::Statement> > &stmts) {
	auto store = make_unique<koopa::ValueStore>(move(val), var_symb->name);
	auto store_stmt = make_unique<koopa::StoreStmt>(move(store));
	stmts.push_back(move(store_stmt));
}

unique_ptr<koopa::Value> GetPrimExp(const unique_ptr<sysy::PrimaryExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->exp_type == sysy::BRACKETEXP) {
		auto new_ast = static_cast<sysy::BracketExp*>(ast.get());
		return GetExp(new_ast->exp, stmts);
	} else if (ast->exp_type == sysy::NUMBEREXP){
		auto new_ast = static_cast<sysy::NumberExp*>(ast.get());
		return make_unique<koopa::IntValue>(new_ast->num->int_const);
	} else {
		auto new_ast = static_cast<sysy::LValExp*>(ast.get());
		auto symb = symbol_table.GetSymbol(new_ast->lval->ident);
		if (symb->symb_type == symtab::CONSTSYMB) {
			auto const_symb = static_cast<symtab::ConstSymb*>(symb);
			return make_unique<koopa::IntValue>(const_symb->val);
		} else {
			auto var_symb = static_cast<symtab::VarSymb*>(symb);
			return LoadSymb(var_symb, stmts);
		}
	}
	return nullptr;
}

unique_ptr<koopa::Value> AddBinExp(unique_ptr<koopa::BinaryExpr> bin_exp,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	auto new_symb_def = make_unique<koopa::BinExprDef>(
		"%" + to_string(temp_var_counter++), move(bin_exp));
	auto new_stmt = make_unique<koopa::SymbolDefStmt>(move(new_symb_def));
	stmts.push_back(move(new_stmt));
	return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter - 1));
}

unique_ptr<koopa::Value> GetUnaryExp(const unique_ptr<sysy::UnaryExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->exp_type == sysy::PRIMUNARYEXP) {
		auto new_ast = static_cast<sysy::PrimUnaryExp*>(ast.get());
		return GetPrimExp(new_ast->prim_exp, stmts);
	} else {
		auto new_ast = static_cast<sysy::OpUnaryExp*>(ast.get());
		map<string, string> unary2inst({{"+", "add"}, {"-", "sub"}, {"!", "eq"}});
		string inst = unary2inst[new_ast->op];
		const auto &exp = new_ast->exp;
		if (inst == "add")
			return GetUnaryExp(exp, stmts);
		else {
			auto ptr = GetUnaryExp(exp, stmts);
			auto new_zero_val = make_unique<koopa::IntValue>(0);
			auto new_bin_exp = make_unique<koopa::BinaryExpr>(
				inst, move(new_zero_val), move(ptr));
			return AddBinExp(move(new_bin_exp), stmts);
		}
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetMulExp(const unique_ptr<sysy::MulExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->mul_exp) {
		map<string, string> mul2inst({{"*", "mul"}, {"/", "div"}, {"%", "mod"}});
		string inst = mul2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetMulExp(ast->mul_exp, stmts), GetUnaryExp(ast->unary_exp, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetUnaryExp(ast->unary_exp, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetAddExp(const unique_ptr<sysy::AddExp> &ast, 
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->add_exp) {
		map<string, string> add2inst({{"+", "add"}, {"-", "sub"}});
		string inst = add2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetAddExp(ast->add_exp, stmts), GetMulExp(ast->mul_exp, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetMulExp(ast->mul_exp, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetRelExp(const unique_ptr<sysy::RelExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->rel_exp) {
		map<string, string> rel2inst({{"<", "lt"}, {">", "gt"}, {"<=", "le"}, {">=", "ge"}});
		string inst = rel2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetRelExp(ast->rel_exp, stmts), GetAddExp(ast->add_exp, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetAddExp(ast->add_exp, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetEqExp(const unique_ptr<sysy::EqExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->eq_exp) {
		map<string, string> rel2inst({{"==", "eq"}, {"!=", "ne"}});
		string inst = rel2inst[ast->op];
		auto bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetEqExp(ast->eq_exp, stmts), GetRelExp(ast->rel_exp, stmts));
		return AddBinExp(move(bin_exp), stmts);
	} else
		return GetRelExp(ast->rel_exp, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetLAndExp(const unique_ptr<sysy::LAndExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->land_exp) {
		auto l_exp = GetLAndExp(ast->land_exp, stmts);
		auto r_exp = GetEqExp(ast->eq_exp, stmts);
		auto l_0 = make_unique<koopa::IntValue>(0);
		auto r_0 = make_unique<koopa::IntValue>(0);
		auto l_logi = make_unique<koopa::BinaryExpr>("ne", move(l_0), move(l_exp));
		auto l_val = AddBinExp(move(l_logi), stmts);
		auto r_logi = make_unique<koopa::BinaryExpr>("ne", move(r_0), move(r_exp));
		auto r_val = AddBinExp(move(r_logi), stmts);
		auto and_exp = make_unique<koopa::BinaryExpr>("and", move(l_val), move(r_val));
		return AddBinExp(move(and_exp), stmts);
	} else
		return GetEqExp(ast->eq_exp, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetLOrExp(const unique_ptr<sysy::LOrExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->lor_exp) {
		auto l_exp = GetLOrExp(ast->lor_exp, stmts);
		auto r_exp = GetLAndExp(ast->land_exp, stmts);
		auto l_0 = make_unique<koopa::IntValue>(0);
		auto r_0 = make_unique<koopa::IntValue>(0);
		auto l_logi = make_unique<koopa::BinaryExpr>("ne", move(l_0), move(l_exp));
		auto l_val = AddBinExp(move(l_logi), stmts);
		auto r_logi = make_unique<koopa::BinaryExpr>("ne", move(r_0), move(r_exp));
		auto r_val = AddBinExp(move(r_logi), stmts);
		auto or_exp = make_unique<koopa::BinaryExpr>("or", move(l_val), move(r_val));
		return AddBinExp(move(or_exp), stmts);
	} else
		return GetLAndExp(ast->land_exp, stmts);
	return nullptr;
}

unique_ptr<koopa::Value> GetExp(const unique_ptr<sysy::Exp> &ast,
			vector<unique_ptr<koopa::Statement> > &stmts) {
	return GetLOrExp(ast->exp, stmts);
}

unique_ptr<koopa::EndStatement> GetStmt(const unique_ptr<sysy::Stmt> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->stmt_type == sysy::RETURNSTMT) {
		auto new_ast = static_cast<sysy::ReturnStmt*>(ast.get());
		auto ret = make_unique<koopa::Return>(GetExp(new_ast->exp, stmts));
		return make_unique<koopa::ReturnEnd>(move(ret));
	} else {
		auto new_ast = static_cast<sysy::AssignStmt*>(ast.get());
		auto symb = symbol_table.GetSymbol(new_ast->lval->ident);
		assert(symb->symb_type == symtab::VARSYMB);
		auto var_symb = static_cast<symtab::VarSymb*>(symb);
		auto rval = GetExp(new_ast->exp, stmts);
		StoreSymb(var_symb, move(rval), stmts);
		return nullptr;
	}
	return nullptr;
}

void GetConstDef(const unique_ptr<sysy::ConstDef> &ast) {
	string ident = ast->ident;
	const auto &exp = ast->val->const_exp->exp;
	int val = exp->Eval();
	auto new_symb = make_unique<symtab::ConstSymb>(val);
	symbol_table.AddSymbol(ident, move(new_symb));
}

void GetVarDef(const unique_ptr<sysy::VarDef> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	string ident = ast->ident;
	string name = "@" + ident;
	auto new_symb = make_unique<symtab::VarSymb>(name);
	auto var_symb = new_symb.get();
	symbol_table.AddSymbol(ident, move(new_symb));
	auto mem_alloc = make_unique<koopa::MemoryDec>(make_shared<koopa::IntType>());
	auto mem_def = make_unique<koopa::MemoryDef>(name, move(mem_alloc));
	auto mem_stmt = make_unique<koopa::SymbolDefStmt>(move(mem_def));
	stmts.push_back(move(mem_stmt));
	if(ast->init_val) {
		auto val = GetExp(ast->init_val->exp, stmts);
		StoreSymb(var_symb, move(val), stmts);
	}
}

unique_ptr<koopa::EndStatement> GetBlockItem(const unique_ptr<sysy::BlockItem> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->item_type == sysy::STMTBLOCKITEM) {
		auto new_ast = static_cast<sysy::StmtBlockItem*>(ast.get());
		auto ret = GetStmt(new_ast->stmt, stmts);
		return ret;
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
				GetVarDef(def, stmts);
		}
		return nullptr;
	}
	return nullptr;
}

unique_ptr<koopa::FunBody> GetBlock(const unique_ptr<sysy::Block> &ast) {
	vector<unique_ptr<koopa::Block> > blocks;
	auto stmts = make_unique<vector<unique_ptr<koopa::Statement> > >();
	for (const auto &item: ast->items) {
		auto end_stmt = GetBlockItem(item, *stmts);
		if (end_stmt) {
			auto block = make_unique<koopa::Block>("%entry", move(*stmts), move(end_stmt));
			blocks.push_back(move(block));
			stmts = make_unique<vector<unique_ptr<koopa::Statement> > >();
		}
	}
	return make_unique<koopa::FunBody>(move(blocks));
}

unique_ptr<koopa::Type> GetFuncType(const unique_ptr<sysy::FuncType> &ast) {
	return make_unique<koopa::IntType>();
}

unique_ptr<koopa::FunDef> GetFuncDef(const unique_ptr<sysy::FuncDef> &ast) {
	auto fun_type = GetFuncType(ast->func_type);
	string s("@" + ast->ident);
	auto fun_body = GetBlock(ast->block);
	return make_unique<koopa::FunDef>(s, nullptr, move(fun_type), move(fun_body));
}

unique_ptr<koopa::Program> GetProgram(const unique_ptr<sysy::CompUnit> &ast) {
	vector<unique_ptr<koopa::FunDef> > v;
	v.push_back(GetFuncDef(ast->func_def));
	return make_unique<koopa::Program>(vector<unique_ptr<koopa::GlobalSymbolDef> >(), move(v));
}


