#include <memory>
#include <vector>
#include <string>
#include "koopa.hpp"
#include "types.hpp"

using namespace std;

string ParseStmt(const unique_ptr<koopa::Statement> &ptr) {
	// TODO
	return "";
}

string ParseEndStmt(const unique_ptr<koopa::EndStatement> &ptr) {
	string result;
	if (ptr->end_type == koopa::RETURNEND) {
		auto new_ptr = static_cast<koopa::Return*>(ptr.get());
		auto int_val = static_cast<koopa::IntValue*>(new_ptr->val.get());
		result += "\tli a0, " + to_string(int_val->integer) + "\n";
		result += "\tret\n";
	}
	return result;
}

string ParseBlock(const unique_ptr<koopa::Block> &ptr) {
	string result;
	result += ptr->symbol.substr(1) + ":\n";
	for (const auto &stmt: ptr->stmts)
		result += ParseStmt(stmt);
	result += ParseEndStmt(ptr->end_stmt);
	return result;
}

string ParseFunBody(const unique_ptr<koopa::FunBody> &ptr ) {
	string result;
	for (const auto &block: ptr->blocks)
		result += ParseBlock(block);
	return result;
}


string ParseFunDef(const unique_ptr<koopa::FunDef> &ptr) {
	string result;
	string name = ptr->symbol.substr(1);
	result += "\t.text\n\t.globl " + name + "\n";
	result += name + ":\n";
	result += ParseFunBody(ptr->body) + "\n";
	return result;
}

string ParseGlobalSymb(const unique_ptr<koopa::GlobalSymbolDef> &ptr) {
	// TODO
	return "";
}

string ParseProgram(const unique_ptr<koopa::Program> &ptr) {
	string result;
	for (const auto &var: ptr->global_vars)
		result += ParseGlobalSymb(var);
	for (const auto &func: ptr->funcs)
		result += ParseFunDef(func) + "\n";
	return result;
}