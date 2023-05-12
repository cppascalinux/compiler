#include <memory>
#include <vector>
#include <string>
#include <map>
#include "koopa.hpp"
#include "types.hpp"
#include "riscv.hpp"

using namespace std;

vector<string> temp_regs;
map<string, string> var2reg;
vector<unique_ptr<riscv::Item> > code;

string GetVar2Reg(string s) {
	if (!var2reg.count(s)) {
		var2reg[s] = temp_regs.back();
		temp_regs.pop_back();
	}
	return var2reg[s];
}

void ParseBinExprDef(const koopa::BinExprDef *ptr) {
	auto expr = ptr->bin_expr.get();
	auto val1 = expr->val1.get(), val2 = expr->val2.get();
	string op = expr->op;
	string reg1, reg2, regd;
	if (val1->val_type == koopa::INTVALUE) {
		auto int_val = static_cast<const koopa::IntValue*>(val1);
		code.push_back(make_unique<riscv::ImmInstr>("li", "a0", "", int_val->integer));
		reg1 = "a0";
	} else {
		auto symb_val = static_cast<const koopa::SymbolValue*>(val1);
		reg1 = GetVar2Reg(symb_val->symbol);
	}
	if (val2->val_type == koopa::INTVALUE) {
		auto int_val = static_cast<const koopa::IntValue*>(val2);
		code.push_back(make_unique<riscv::ImmInstr>("li", "a1", "", int_val->integer));
		reg2 = "a1";
	} else {
		auto symb_val = static_cast<const koopa::SymbolValue*>(val2);
		reg2 = GetVar2Reg(symb_val->symbol);
	}
	regd = GetVar2Reg(ptr->symbol);
	if (op == "ne") {
		code.push_back(make_unique<riscv::RegInstr>("xor", regd, reg1, reg2));
		code.push_back(make_unique<riscv::RegInstr>("snez", regd, regd, ""));
	} else if (op == "eq") {
		code.push_back(make_unique<riscv::RegInstr>("xor", regd, reg1, reg2));
		code.push_back(make_unique<riscv::RegInstr>("seqz", regd, regd, ""));
	} else if (op == "le") {
		code.push_back(make_unique<riscv::RegInstr>("sgt", regd, reg2, reg1));
	} else if (op == "ge") {
		code.push_back(make_unique<riscv::RegInstr>("slt", regd, reg2, reg1));
	} else {
		map<string, string> op_map = {
			{"lt", "slt"}, {"gt", "sgt"}, {"add", "add"}, {"sub", "sub"},
			{"mul", "mul"}, {"div", "div"}, {"mod", "rem"}, {"and", "and"},
			{"or", "or"}, {"xor", "xor"}, {"shl", "sll"}, {"shr", "srl"},
			{"sar", "sra"}};
		code.push_back(make_unique<riscv::RegInstr>(op_map[op], regd, reg1, reg2));
	}
}

void ParseSymDef(const koopa::SymbolDef *ptr) {
	if (ptr->def_type == koopa::BINEXPRDEF) {
		auto bin_ptr = static_cast<const koopa::BinExprDef*>(ptr);
		ParseBinExprDef(bin_ptr);
	}
}

void ParseStmt(const koopa::Statement *ptr) {
	if (ptr->stmt_type == koopa::SYMBOLDEFSTMT)
		ParseSymDef(static_cast<const koopa::SymbolDef*>(ptr));
}

void ParseReturn(const koopa::Return *ptr) {
	auto val = ptr->val.get();
	if (val->val_type == koopa::INTVALUE) {
		auto int_val = static_cast<const koopa::IntValue*>(val);
		code.push_back(make_unique<riscv::ImmInstr>("li", "a0", "", int_val->integer));
		code.push_back(make_unique<riscv::LabelInstr>("ret", "", ""));
	} else if (val->val_type == koopa::SYMBOLVALUE) {
		auto symb_val = static_cast<const koopa::SymbolValue*>(val);
		code.push_back(make_unique<riscv::RegInstr>("mv", "a0", GetVar2Reg(symb_val->symbol), ""));
		code.push_back(make_unique<riscv::LabelInstr>("ret", "", ""));
	}
}

void ParseEndStmt(const koopa::Statement *ptr) {
	if (ptr->stmt_type == koopa::RETURNEND)
		ParseReturn(static_cast<const koopa::Return*>(ptr));
}

void ParseBlock(const koopa::Block *ptr) {
	code.push_back(make_unique<riscv::Label>(ptr->symbol.substr(1)));
	temp_regs.clear();
	for (int i = 0; i < 7; i ++)
		temp_regs.push_back("t"+to_string(i));
	for (int i = 2; i < 8; i ++)
		temp_regs.push_back("a"+to_string(i));
	var2reg.clear();
	var2reg["0"] = "x0";
	for (const auto &stmt: ptr->stmts)
		ParseStmt(stmt.get());
	ParseEndStmt(ptr->end_stmt.get());
}

void ParseFunBody(const koopa::FunBody *ptr) {
	for (const auto &block: ptr->blocks)
		ParseBlock(block.get());
}


void ParseFunDef(const koopa::FunDef *ptr) {
	string name = ptr->symbol.substr(1);
	code.push_back(make_unique<riscv::PseudoOp>(".text", ""));
	code.push_back(make_unique<riscv::PseudoOp>(".globl", name));
	code.push_back(make_unique<riscv::Label>(name));
	ParseFunBody(ptr->body.get());
}

void ParseGlobalSymb(const koopa::GlobalSymbolDef *ptr) {
	// TODO
}

string ParseProgram(const koopa::Program *ptr) {
	for (const auto &var: ptr->global_vars)
		ParseGlobalSymb(var.get());
	for (const auto &func: ptr->funcs)
		ParseFunDef(func.get());
	string result;
	for (const auto &ptr: code)
		result += ptr->Str();
	return result;
}