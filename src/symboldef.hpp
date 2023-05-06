#pragma once

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "types.hpp"
#include "values.hpp"
#include "memory.hpp"
#include "pointers.hpp"
#include "binary.hpp"
#include "call_ret.hpp"


namespace koopa {

// SymbolDef ::= SYMBOL "=" (MemoryDeclaration | Load | GetPointer | BinaryExpr | FunCall);
enum SymbolDefType {
	MEMORYDEF,
	LOADDEF,
	GETPTRDEF,
	BINEXPRDEF,
	FUNCALLDEF
};

class SymbolDef {
	public:
		SymbolDefType def_type;
		std::string symbol;
		SymbolDef(SymbolDefType a, std::string s): def_type(a), symbol(s){}
		virtual ~SymbolDef() = default;
		virtual std::string Str() const = 0;
};

class MemoryDef: public SymbolDef {
	public:
		std::unique_ptr<MemoryDec> mem_dec;
		MemoryDef(std::string s, std::unique_ptr<MemoryDec> p):
			SymbolDef(MEMORYDEF, s), mem_dec(std::move(p)){}
		std::string Str() const override {
			return symbol + " = " + mem_dec->Str();
		}
};

class LoadDef: public SymbolDef {
	public:
		std::unique_ptr<Load> load;
		LoadDef(std::string s, std::unique_ptr<Load> p):
			SymbolDef(LOADDEF, s), load(std::move(p)){}
		std::string Str() const override {
			return symbol + " = " + load->Str();
		}
};

class GetPtrDef: public SymbolDef {
	public:
		std::unique_ptr<GetPointer> get_ptr;
		GetPtrDef(std::string s, std::unique_ptr<GetPointer> p):
			SymbolDef(GETPTRDEF, s), get_ptr(std::move(p)){}
		std::string Str() const override {
			return symbol + " = " + get_ptr->Str();
		}
};

class BinExprDef: public SymbolDef {
	public:
		std::unique_ptr<BinaryExpr> bin_expr;
		BinExprDef(std::string s, std::unique_ptr<BinaryExpr> p):
			SymbolDef(BINEXPRDEF, s), bin_expr(std::move(p)){}
		std::string Str() const override {
			return symbol + " = " + bin_expr->Str();
		}
};

class FunCallDef: public SymbolDef {
	public:
		std::unique_ptr<FunCall> fun_call;
		FunCallDef(std::string s, std::unique_ptr<FunCall> p):
			SymbolDef(FUNCALLDEF, s), fun_call(std::move(p)){}
		std::string Str() const override {
			return symbol + " = " + fun_call->Str();
		}
};


// GlobalSymbolDef ::= "global" SYMBOL "=" GlobalMemoryDeclaration;
class GlobalSymbolDef {
	public:
		std::string symbol;
		std::unique_ptr<GlobalMemDec> mem_dec;
		GlobalSymbolDef(std::string s, std::unique_ptr<GlobalMemDec> p):
			symbol(s), mem_dec(std::move(p)){}
		std::string Str() const {
			return "global " + symbol + " = " + mem_dec->Str();
		}
};


}