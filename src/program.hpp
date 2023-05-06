#pragma once

#include <memory>
#include <string>
#include <vector>
#include "functions.hpp"
#include "symboldef.hpp"

namespace koopa {

class Program {
	public:
		std::vector<std::unique_ptr<GlobalSymbolDef> > global_vars;
		std::vector<std::unique_ptr<FunDef> > funcs;
		Program(std::vector<std::unique_ptr<GlobalSymbolDef> > p,
			std::vector<std::unique_ptr<FunDef> > q):
			global_vars(std::move(p)), funcs(std::move(q)){}
		std::string Str() const {
			std::string s;
			for (auto &ptr: global_vars)
				s += ptr->Str() + "\n";
			for (auto &ptr: funcs)
				s += ptr->Str();
			return s;
		}
};

}