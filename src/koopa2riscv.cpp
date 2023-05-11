#include <memory>
#include <vector>
#include <string>
#include <map>
#include "koopa.hpp"
#include "types.hpp"

using namespace std;

vector<string> temp_regs;
map<string, string> var2reg;

string ParseSymDef(const koopa::SymbolDef *ptr) {
	// TODO
	return "";
}

string ParseStmt(const koopa::Statement *ptr) {
	if (ptr->stmt_type == koopa::SYMBOLDEFSTMT)
		return ParseSymDef(static_cast<const koopa::SymbolDef*>(ptr));
	return "";
}

string ParseReturn(const koopa::Return *ptr) {
	// TODO
	return "";
}

string ParseEndStmt(const koopa::EndStatement *ptr) {
	if (ptr->end_type == koopa::RETURNEND)
		return ParseReturn(static_cast<const koopa::Return*>(ptr));
	return "";
}

string ParseBlock(const koopa::Block *ptr) {
	string result;
	result += ptr->symbol.substr(1) + ":\n";
	temp_regs.clear();
	for (int i = 0; i < 7; i ++)
		temp_regs.push_back("t"+to_string(i));
	for (int i = 0; i < 8; i ++)
		temp_regs.push_back("a"+to_string(i));
	for (const auto &stmt: ptr->stmts)
		result += ParseStmt(stmt.get());
	result += ParseEndStmt(ptr->end_stmt.get());
	return result;
}

string ParseFunBody(const koopa::FunBody *ptr) {
	string result;
	for (const auto &block: ptr->blocks)
		result += ParseBlock(block.get());
	return result;
}


string ParseFunDef(const koopa::FunDef *ptr) {
	string result;
	string name = ptr->symbol.substr(1);
	result += "\t.text\n\t.globl " + name + "\n";
	result += name + ":\n";
	result += ParseFunBody(ptr->body.get()) + "\n";
	return result;
}

string ParseGlobalSymb(const koopa::GlobalSymbolDef *ptr) {
	// TODO
	return "";
}

string ParseProgram(const koopa::Program *ptr) {
	string result;
	for (const auto &var: ptr->global_vars)
		result += ParseGlobalSymb(var.get());
	for (const auto &func: ptr->funcs)
		result += ParseFunDef(func.get()) + "\n";
	return result;
}