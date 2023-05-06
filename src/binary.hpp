#pragma once

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "types.hpp"
#include "values.hpp"

namespace koopa {

// BinaryExpr ::= BINARY_OP Value "," Value;
class BinaryExpr {
	public:
		std::string op;
		std::unique_ptr<Value> val1, val2;
		BinaryExpr(std::string s, std::unique_ptr<Value> x, std::unique_ptr<Value> y):
			op(s), val1(std::move(x)), val2(std::move(y)){}
		std::string Str() const {
			return op + " " + val1->Str() + ", " + val2->Str();
		}
};

}