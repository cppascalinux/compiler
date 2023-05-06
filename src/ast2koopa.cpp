#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include "ast.hpp"
#include "koopa.hpp"
#include "ast2koopa.hpp"

using namespace std;

unique_ptr<koopa::Value> GetValue(const unique_ptr<sysy::NumberAST> &ast) {
	return make_unique<koopa::IntValue>(ast->int_const);
}

unique_ptr<koopa::Return> GetReturn(const unique_ptr<sysy::StmtAST> &ast) {
	return make_unique<koopa::Return>(GetValue(ast->number));
}

unique_ptr<koopa::FunBody> GetFunBody(const unique_ptr<sysy::BlockAST> &ast) {
	auto end_stmt = make_unique<koopa::ReturnEnd>(GetReturn(ast->stmt));
	auto block = make_unique<koopa::Block>("%entry",
		vector<unique_ptr<koopa::Statement> >(), move(end_stmt));
	vector<unique_ptr<koopa::Block> > v;
	v.push_back(move(block));
	return make_unique<koopa::FunBody>(move(v));
}

unique_ptr<koopa::Type> GetFuncType(const unique_ptr<sysy::FuncTypeAST> &ast) {
	return make_unique<koopa::IntType>();
}

unique_ptr<koopa::FunDef> GetFuncDef(const unique_ptr<sysy::FuncDefAST> &ast) {
	auto fun_type = GetFuncType(ast->func_type);
	string s("@" + ast->ident);
	auto fun_body = GetFunBody(ast->block);
	return make_unique<koopa::FunDef>(s, nullptr, move(fun_type), move(fun_body));
}

unique_ptr<koopa::Program> GetProgram(const unique_ptr<sysy::CompUnitAST> &ast) {
	vector<unique_ptr<koopa::FunDef> > v;
	v.emplace_back(GetFuncDef(ast->func_def));
	return make_unique<koopa::Program>(vector<unique_ptr<koopa::GlobalSymbolDef> >(), move(v));
}


