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
		Symbol(SymbolType a): symb_type(a) {}
		virtual ~Symbol() = default;
};

class ConstSymb: public Symbol {
	public:
		int val;
		ConstSymb(int x): Symbol(CONSTSYMB), val(x) {}
};

class VarSymb: public Symbol {
	public:
		std::string name;
		VarSymb(std::string s): Symbol(VARSYMB), name(s) {}
};

class SymTab {
	public:
		std::map<std::string, std::unique_ptr<Symbol> > table;
		int AddSymbol(std::string s, std::unique_ptr<Symbol> ptr) {
			if (table.count(s))
				assert(0);
			table[s] = std::move(ptr);
			return 1;
		}
		Symbol *GetSymbol(std::string s) {
			if (!table.count(s))
				return nullptr;
			return table[s].get();
		}
};

class SymTabStack {
	public:
		std::vector<std::unique_ptr<SymTab> > symtabs;
		int total;
		SymTabStack(): symtabs(), total(0) {push(); total=0;}
		void push() {
			symtabs.push_back(std::make_unique<SymTab>());
			total++;
		}
		std::unique_ptr<SymTab> pop() {
			total++;
			auto ptr = std::move(symtabs.back());
			symtabs.pop_back();
			return ptr;
		}
		int GetTotal() {
			return total;
		}
		SymTab *GetTop() {
			return symtabs.back().get();
		}
		int AddSymbol(std::string s, std::unique_ptr<Symbol> ptr) {
			auto top_symtab = GetTop();
			return top_symtab->AddSymbol(s, move(ptr));
		}
		Symbol *GetSymbol(std::string s) {
			for (auto it = symtabs.rbegin(); it != symtabs.rend(); it++) {
				auto symb = (*it)->GetSymbol(s);
				if (symb)
					return symb;
			}
			assert(0);
			return nullptr;
		}
};

}

extern symtab::SymTabStack symtab_stack;