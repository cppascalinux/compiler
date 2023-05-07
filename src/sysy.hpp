// Classes for  of SysY

#pragma once

#include <memory>
#include <string>
#include <iostream>


namespace sysy {


class Base {
	public:
		virtual ~Base() = default;
		virtual void Dump() const = 0;
};

// Number      ::= INT_CONST;
class Number: public Base {
	public:
		int int_const;
		Number(int x): int_const(x){}
		virtual void Dump() const override {
			std::cout << "Number { " << int_const << " }";
		}
};


class Exp;


// PrimaryExp  ::= "(" Exp ")" | Number;
enum PrimaryExpType {
	BRACKETEXP,
	NUMBEREXP
};

class PrimaryExp: public Base {
	public:
		PrimaryExpType exp_type;
		PrimaryExp(PrimaryExpType a): exp_type(a){}
		virtual ~PrimaryExp() override = default;
		virtual void Dump() const override = 0;
};

class BracketExp: public PrimaryExp {
	public:
		std::unique_ptr<Exp> exp;
		BracketExp(Base *p);
		virtual void Dump() const override;
};

class NumberExp: public PrimaryExp {
	public:
		std::unique_ptr<Number> num;
		NumberExp(Base *p): PrimaryExp(NUMBEREXP), num(static_cast<Number*>(p)){}
		virtual void Dump() const override {
			std::cout << "PrimaryExp { ";
			num->Dump();
			std::cout << " }";
		}
};


// UnaryOp     ::= "+" | "-" | "!";
class UnaryOp: public Base {
	public:
		std::string op;
		UnaryOp(std::string s): op(s){}
		virtual void Dump() const override {
			std::cout << "UnaryOp { " << op << " }";
		}
};


// UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
enum UnaryExpType {
	PRIMUNARYEXP,
	OPUNARYEXP
};

class UnaryExp: public Base {
	public:
		UnaryExpType exp_type;
		UnaryExp(UnaryExpType a): exp_type(a){}
		virtual ~UnaryExp() override = default;
		virtual void Dump() const override = 0;
};

class PrimUnaryExp: public UnaryExp {
	public:
		std::unique_ptr<PrimaryExp> prim_exp;
		PrimUnaryExp(Base *p):
			UnaryExp(PRIMUNARYEXP), prim_exp(static_cast<PrimaryExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "UnaryExp { ";
			prim_exp->Dump();
			std::cout << " }";
		}
};

class OpUnaryExp: public UnaryExp {
	public:
		std::unique_ptr<UnaryOp> op;
		std::unique_ptr<UnaryExp> exp;
		OpUnaryExp(Base *p, Base *q): UnaryExp(OPUNARYEXP),
			op(static_cast<UnaryOp*>(p)), exp(static_cast<UnaryExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "UnaryExp { ";
			op->Dump();
			std::cout << ", ";
			exp->Dump();
			std::cout << " }";
		}
};


// Exp         ::= UnaryExp;
class Exp: public Base {
	public:
		std::unique_ptr<UnaryExp> exp;
		Exp(Base *p): exp(static_cast<UnaryExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "Exp { ";
			exp->Dump();
			std::cout << " }";
		}
};



// Stmt        ::= "return" Exp ";";
class Stmt: public Base {
	public:
		std::unique_ptr<Exp> exp;
		Stmt(Base *p): exp(static_cast<Exp*>(p)){}
		virtual void Dump() const override {
			std::cout << "Stmt { ";
			exp->Dump();
			std::cout << " }";
		}
};


// Block     ::= "{" Stmt "}";
class Block: public Base {
	public:
		std::unique_ptr<Stmt> stmt;
		Block(Base *p): stmt(static_cast<Stmt*>(p)){}
		virtual void Dump() const override {
			std::cout << "Block { ";
			stmt->Dump();
			std::cout << " }";
		}
};



// FuncType  ::= "int";
class FuncType: public Base {
	public:
		virtual void Dump() const override {
			std::cout << "FuncType { int }";
		}
};



// FuncDef   ::= FuncType IDENT "(" ")" Block;
class FuncDef: public Base {
	public:
		std::unique_ptr<FuncType> func_type;
		std::string ident;
		std::unique_ptr<Block> block;
		FuncDef(Base *p, std::string s, Base *q):
			func_type(static_cast<FuncType*>(p)), ident(s),
			block(static_cast<Block*>(q)){}
		virtual void Dump() const override {
			std::cout << "FuncDef { ";
			func_type->Dump();
			std::cout << ", " << ident << ", ";
			block->Dump();
			std::cout << " }";
		}
};

// CompUnit  ::= FuncDef;
class CompUnit: public Base {
	public:
		std::unique_ptr<FuncDef> func_def;
		CompUnit(Base *p): func_def(static_cast<FuncDef*>(p)){}
		virtual void Dump() const override {
			std::cout << "CompUnit { ";
			func_def->Dump();
			std::cout << " }";
		}
};


}