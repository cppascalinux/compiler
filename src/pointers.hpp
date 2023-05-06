#pragma once

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "types.hpp"
#include "values.hpp"


namespace koopa {

// GetPointer ::= "getptr" SYMBOL "," Value;
class GetPointer {
	public:
		std::string symbol;
		std::unique_ptr<Value> val;
		GetPointer(std::string s, std::unique_ptr<Value> p):
			symbol(s), val(std::move(p)){}
		std::string Str() const {
			return "getptr " + symbol + ", " + val->Str();
		}
};


// GetElementPointer ::= "getelemptr" SYMBOL "," Value;
class GetElementPointer {
	public:
		std::string symbol;
		std::unique_ptr<Value> val;
		GetElementPointer(std::string s, std::unique_ptr<Value> p):
			symbol(s), val(std::move(p)){}
		std::string Str() const {
			return "getelemptr " + symbol + ", " + val->Str();
		}
};


}