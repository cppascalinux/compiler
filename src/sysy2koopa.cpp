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
		string inst = unary2inst[new_ast->op->op];
		auto &exp = new_ast->exp;
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
	if (ast->mul_type == sysy::UNARYMULEXP) {
		auto new_ast = static_cast<sysy::UnaryMulExp*>(ast.get());
		return GetUnaryExp(new_ast->exp, stmts);
	} else {
		auto new_ast = static_cast<sysy::OpMulExp*>(ast.get());
		map<string, string> mul2inst({{"*", "mul"}, {"/", "div"}, {"%", "mod"}});
		string inst = mul2inst[new_ast->op];
		auto new_bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetMulExp(new_ast->mul_exp, stmts), GetUnaryExp(new_ast->unary_exp, stmts));
		return AddBinExp(move(new_bin_exp), stmts);
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
		return AddBinExp(move(new_bin_exp), stmts);
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetRelExp(const unique_ptr<sysy::RelExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->rel_type == sysy::ADDRELEXP) {
		auto new_ast = static_cast<sysy::AddRelExp*>(ast.get());
		return GetAddExp(new_ast->exp, stmts);
	} else {
		auto new_ast = static_cast<sysy::OpRelExp*>(ast.get());
		map<string, string> rel2inst({{"<", "lt"}, {">", "gt"}, {"<=", "le"}, {">=", "ge"}});
		string inst = rel2inst[new_ast->op];
		auto new_bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetRelExp(new_ast->rel_exp, stmts), GetAddExp(new_ast->add_exp, stmts));
		return AddBinExp(move(new_bin_exp), stmts);
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetEqExp(const unique_ptr<sysy::EqExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->eq_type == sysy::RELEQEXP) {
		auto new_ast = static_cast<sysy::RelEqExp*>(ast.get());
		return GetRelExp(new_ast->exp, stmts);
	} else {
		auto new_ast = static_cast<sysy::OpEqExp*>(ast.get());
		map<string, string> rel2inst({{"==", "eq"}, {"!=", "ne"}});
		string inst = rel2inst[new_ast->op];
		auto new_bin_exp = make_unique<koopa::BinaryExpr>(inst,
			GetEqExp(new_ast->eq_exp, stmts), GetRelExp(new_ast->rel_exp, stmts));
		return AddBinExp(move(new_bin_exp), stmts);
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetLAndExp(const unique_ptr<sysy::LAndExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->land_type == sysy::EQLANDEXP) {
		auto new_ast = static_cast<sysy::EqLAndExp*>(ast.get());
		return GetEqExp(new_ast->exp, stmts);
	} else {
		auto new_ast = static_cast<sysy::OpLAndExp*>(ast.get());
		auto l_exp = GetLAndExp(new_ast->land_exp, stmts);
		auto r_exp = GetEqExp(new_ast->eq_exp, stmts);
		auto l_0 = make_unique<koopa::IntValue>(0);
		auto r_0 = make_unique<koopa::IntValue>(0);
		auto l_logi = make_unique<koopa::BinaryExpr>("ne", move(l_0), move(l_exp));
		auto l_val = AddBinExp(move(l_logi), stmts);
		auto r_logi = make_unique<koopa::BinaryExpr>("ne", move(r_0), move(r_exp));
		auto r_val = AddBinExp(move(r_logi), stmts);
		auto and_exp = make_unique<koopa::BinaryExpr>("and", move(l_val), move(r_val));
		return AddBinExp(move(and_exp), stmts);
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetLOrExp(const unique_ptr<sysy::LOrExp> &ast,
	vector<unique_ptr<koopa::Statement> > &stmts) {
	if (ast->lor_type == sysy::LANDLOREXP) {
		auto new_ast = static_cast<sysy::LAndLOrExp*>(ast.get());
		return GetLAndExp(new_ast->exp, stmts);
	} else {
		auto new_ast = static_cast<sysy::OpLOrExp*>(ast.get());
		auto l_exp = GetLOrExp(new_ast->lor_exp, stmts);
		auto r_exp = GetLAndExp(new_ast->land_exp, stmts);
		auto l_0 = make_unique<koopa::IntValue>(0);
		auto r_0 = make_unique<koopa::IntValue>(0);
		auto l_logi = make_unique<koopa::BinaryExpr>("ne", move(l_0), move(l_exp));
		auto l_val = AddBinExp(move(l_logi), stmts);
		auto r_logi = make_unique<koopa::BinaryExpr>("ne", move(r_0), move(r_exp));
		auto r_val = AddBinExp(move(r_logi), stmts);
		auto or_exp = make_unique<koopa::BinaryExpr>("or", move(l_val), move(r_val));
		return AddBinExp(move(or_exp), stmts);
	}
	return nullptr;
}

unique_ptr<koopa::Value> GetExp(const unique_ptr<sysy::Exp> &ast,
			vector<unique_ptr<koopa::Statement> > &stmts) {
		return GetLOrExp(ast->exp, stmts);
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


