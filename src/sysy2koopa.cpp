#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include <string>
#include <map>
#include "sysy.hpp"
#include "koopa.hpp"
#include "sysy2koopa.hpp"

using namespace std;

int temp_var_counter=0;

unique_ptr<koopa::Value> GetPrimExp(const unique_ptr<sysy::PrimaryExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->exp_type == sysy::BRACKETEXP) {
		auto new_ast = static_cast<sysy::BracketExp*>(ast.get());
		return GetExp(new_ast->exp, stmts);
	} else {
		auto new_ast = static_cast<sysy::NumberExp*>(ast.get());
		return make_unique<koopa::IntValue>(new_ast->num->int_const);
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetUnaryExp(const unique_ptr<sysy::UnaryExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->exp_type == sysy::PRIMUNARYEXP) {
		auto new_ast = static_cast<sysy::PrimUnaryExp*>(ast.get());
		return GetPrimExp(new_ast->prim_exp, stmts);
	} else {
		auto new_ast = static_cast<sysy::OpUnaryExp*>(ast.get());
		map<string, string> unary2inst({{"+", "add"}, {"-", "sub"}, {"!", "eq"}});
		string inst = unary2inst[new_ast->op->op];
		auto &exp = new_ast->exp;
		if (inst == "add")
			return GetUnaryExp(exp, stmts);
		else {
			auto ptr = GetUnaryExp(exp, stmts);
			auto new_zero_val = make_unique<koopa::IntValue>(0);
			auto new_bin_exp = make_unique<koopa::BinaryExpr>(
				inst, move(new_zero_val), move(ptr));
			auto new_symb_def = make_unique<koopa::BinExprDef>(
				"%" + to_string(temp_var_counter++), move(new_bin_exp));
			auto new_stmt = make_unique<koopa::SymbolDefStmt>(move(new_symb_def));
			stmts.push_back(move(new_stmt));
			return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter - 1));
		}
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetMulExp(const unique_ptr<sysy::MulExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->mul_type == sysy::UNARYMULEXP) {
		auto new_ast = static_cast<sysy::UnaryMulExp*>(ast.get());
		return GetUnaryExp(new_ast->exp, stmts);
	} else {
		auto new_ast = static_cast<sysy::OpMulExp*>(ast.get());
		map<string, string> mul2inst({{"*", "mul"}, {"/", "div"}, {"%", "mod"}});
		string inst = mul2inst[new_ast->op];
		auto new_bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetMulExp(new_ast->mul_exp, stmts), GetUnaryExp(new_ast->unary_exp, stmts));
		auto new_symb_def = make_unique<koopa::BinExprDef>(
			"%" + to_string(temp_var_counter++), move(new_bin_exp));
		auto new_stmt = make_unique<koopa::SymbolDefStmt>(move(new_symb_def));
		stmts.push_back(move(new_stmt));
		return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter - 1));
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetAddExp(const unique_ptr<sysy::AddExp> &ast, 
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->add_type == sysy::MULADDEXP) {
		auto new_ast = static_cast<sysy::MulAddExp*>(ast.get());
		return GetMulExp(new_ast->exp, stmts);
	} else {
		auto new_ast = static_cast<sysy::OpAddExp*>(ast.get());
		map<string, string> add2inst({{"+", "add"}, {"-", "sub"}});
		string inst = add2inst[new_ast->op];
		auto new_bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetAddExp(new_ast->add_exp, stmts), GetMulExp(new_ast->mul_exp, stmts));
		auto new_symb_def = make_unique<koopa::BinExprDef>(
			"%" + to_string(temp_var_counter++), move(new_bin_exp));
		auto new_stmt = make_unique<koopa::SymbolDefStmt>(move(new_symb_def));
		stmts.push_back(move(new_stmt));
		return make_unique<koopa::SymbolValue>("%" + to_string(temp_var_counter - 1));
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetExp(const unique_ptr<sysy::Exp> &ast,
			vector<unique_ptr<koopa::Statement> > &stmts) {
		return GetAddExp(ast->exp, stmts);
}

unique_ptr<koopa::Return> GetStmt(const unique_ptr<sysy::Stmt> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
		return make_unique<koopa::Return>(GetExp(ast->exp, stmts));
}

unique_ptr<koopa::FunBody> GetFunBody(const unique_ptr<sysy::Block> &ast) {
	vector<unique_ptr<koopa::Statement> > stmts;
	auto ret_stmt = make_unique<koopa::ReturnEnd>(GetStmt(ast->stmt, stmts));
	auto block = make_unique<koopa::Block>("%entry", move(stmts), move(ret_stmt));
	vector<unique_ptr<koopa::Block> > v;
	v.push_back(move(block));
	return make_unique<koopa::FunBody>(move(v));
}

unique_ptr<koopa::Type> GetFuncType(const unique_ptr<sysy::FuncType> &ast) {
	return make_unique<koopa::IntType>();
}

unique_ptr<koopa::FunDef> GetFuncDef(const unique_ptr<sysy::FuncDef> &ast) {
	auto fun_type = GetFuncType(ast->func_type);
	string s("@" + ast->ident);
	auto fun_body = GetFunBody(ast->block);
	return make_unique<koopa::FunDef>(s, nullptr, move(fun_type), move(fun_body));
}

unique_ptr<koopa::Program> GetProgram(const unique_ptr<sysy::CompUnit> &ast) {
	vector<unique_ptr<koopa::FunDef> > v;
	v.emplace_back(GetFuncDef(ast->func_def));
	return make_unique<koopa::Program>(vector<unique_ptr<koopa::GlobalSymbolDef> >(), move(v));
}


