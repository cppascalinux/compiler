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


FuncUnaryExp::FuncUnaryExp(std::string s, void *p):
UnaryExp(FUNCUNARYEXP), ident(s),
params(move(*std::unique_ptr<std::vector<std::unique_ptr<Exp> > >(
static_cast<std::vector<std::unique_ptr<Exp> >*>(p)))) {}

void FuncUnaryExp::Dump() const {
	std::cout << "UnaryExp { " + ident + " ( ";
	for (const auto &ptr: params) {
		ptr->Dump();
		std::cout << ", ";
	}
	std::cout << " ) }";
}

int FuncUnaryExp::Eval() const {
	assert(0);
	return 0;
}


LVal::LVal(std::string s, void *p): ident(s),
	dims(move(*std::unique_ptr<std::vector<std::unique_ptr<Exp> > >
	(static_cast<std::vector<std::unique_ptr<Exp> >*>(p)))) {}
void LVal::Dump() const {
	std::cout << "LVal { " + ident;
	for (const auto &ptr: dims) {
		std::cout << "[";
		ptr->Dump();
		std::cout << "]";
	}
	std::cout << " }";
}