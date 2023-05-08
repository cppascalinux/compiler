
#include <memory>
#include <string>
#include <iostream>
#include "sysy.hpp"

using namespace sysy;

BracketExp::BracketExp(Base *p): PrimaryExp(BRACKETEXP), exp(static_cast<Exp*>(p)){}
void BracketExp::Dump() const {
	std::cout << "PrimaryExp { ( ";
	exp->Dump();
	std::cout << " ) }";
}

int BracketExp::Eval() const {
	return exp->Eval();
}