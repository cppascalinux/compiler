#pragma once

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "types.hpp"
#include "values.hpp"


namespace koopa {

// FunCall ::= "call" SYMBOL "(" [Value {"," Value}] ")";
class FunCall {
	public:
		std::string symbol;
		std::vector<std::unique_ptr<Value> > params;
		FunCall(std::string s, std::vector<std::unique_ptr<Value> > v):
			symbol(s), params(std::move(v)){}
		std::string Str() const {
			std::string s("call" + symbol + "(");
			if (!params.empty()) {
				for (auto &p: params)
					s += p->Str() + ", ";
				s.erase(s.end() - 2, s.end());
			}
			return s + ")";
		}
};


// Return ::= "ret" [Value];
class Return {
	public:
		std::unique_ptr<Value> val;
		Return(std::unique_ptr<Value> p): val(std::move(p)){}
		std::string Str() const {
			if (val)
				return "ret " + val->Str();
			else
				return "ret";
		}
};


}