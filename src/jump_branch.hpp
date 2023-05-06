#pragma once

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "types.hpp"
#include "values.hpp"


namespace koopa {

// Branch ::= "br" Value "," SYMBOL "," SYMBOL;
class Branch {
	public:
		std::unique_ptr<Value> val;
		std::string symbol1, symbol2;
		Branch(std::unique_ptr<Value> p, std::string a, std::string b):
			val(std::move(p)), symbol1(a), symbol2(b){}
		std::string Str() const {
			return "br " + val->Str() + ", " + symbol1 + ", " + symbol2;
		}
};


// Jump ::= "jump" SYMBOL;
class Jump {
	public:
		std::string symbol;
		Jump(std::string s): symbol(s){}
		std::string Str() const {
			return "jump " + symbol;
		}
};

}