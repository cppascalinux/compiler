#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include "koopa.hpp"

namespace symtab{

enum SymbolType {
	CONSTSYMB,
	VARSYMB
};

class Symbol {
	public:
		SymbolType symb_type;
		Symbol(SymbolType a): symb_type(a){}
		virtual ~Symbol() = default;
};

class ConstSymb: public Symbol {
	public:
		int val;
		ConstSymb(int x): Symbol(CONSTSYMB), val(x){}
};

class VarSymb: public Symbol {
	public:
		std::string name;
		VarSymb(std::string s): Symbol(VARSYMB), name(s){}
};

class SymTab {
	public:
		std::map<std::string, std::unique_ptr<Symbol> > table;
		int AddSymbol(std::string s, std::unique_ptr<Symbol> ptr) {
			if (table.count(s))
				return 0;
			table[s] = move(ptr);
			return 1;
		}
		Symbol *GetSymbol(std::string s) {
			if (!table.count(s))
				return nullptr;
			return table[s].get();
		}
};

}

extern symtab::SymTab symbol_table;