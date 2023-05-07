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


// MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
enum MulExpType {
	UNARYMULEXP,
	OPMULEXP
};

class MulExp: public Base {
	public:
		MulExpType mul_type;
		MulExp(MulExpType a): mul_type(a){}
		virtual ~MulExp() = default;
		virtual void Dump() const override = 0;
};

class UnaryMulExp: public MulExp {
	public:
		std::unique_ptr<UnaryExp> exp;
		UnaryMulExp(Base *p): MulExp(UNARYMULEXP), exp(static_cast<UnaryExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "MulExp { ";
			exp->Dump();
			std::cout << " }";
		}
};

class OpMulExp: public MulExp {
	public:
		std::unique_ptr<MulExp> mul_exp;
		std::string op;
		std::unique_ptr<UnaryExp> unary_exp;
		OpMulExp(Base *p, std::string s, Base *q):MulExp(OPMULEXP),
			mul_exp(static_cast<MulExp*>(p)), op(s), unary_exp(static_cast<UnaryExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "MulExp { ";
			mul_exp->Dump();
			std::cout << " " + op + " ";
			unary_exp->Dump();
			std::cout << " }";
		}
};


// AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
enum AddExpType {
	MULADDEXP,
	OPADDEXP
};

class AddExp: public Base {
	public:
		AddExpType add_type;
		AddExp(AddExpType a): add_type(a){}
		virtual ~AddExp() = default;
		virtual void Dump() const override = 0;
};

class MulAddExp: public AddExp {
	public:
		std::unique_ptr<MulExp> exp;
		MulAddExp(Base *p): AddExp(MULADDEXP), exp(static_cast<MulExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "AddExp { ";
			exp->Dump();
			std::cout << "} ";
		}
};

class OpAddExp: public AddExp {
	public:
		std::unique_ptr<AddExp> add_exp;
		std::string op;
		std::unique_ptr<MulExp> mul_exp;
		OpAddExp(Base *p, std::string s, Base *q): AddExp(OPADDEXP),
			add_exp(static_cast<AddExp*>(p)), op(s), mul_exp(static_cast<MulExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "AddExp { ";
			add_exp->Dump();
			std::cout << " " + op + " ";
			mul_exp->Dump();
			std::cout << " }";
		}
};


// RelExp      ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
enum RelExpType {
	ADDRELEXP,
	OPRELEXP
};

class RelExp: public Base {
	public:
		RelExpType rel_type;
		RelExp(RelExpType a): rel_type(a){}
		virtual ~RelExp() = default;
		virtual void Dump() const override = 0;
};

class AddRelExp: public RelExp {
	public:
		std::unique_ptr<AddExp> exp;
		AddRelExp(Base *p): RelExp(ADDRELEXP), exp(static_cast<AddExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "RelExp { ";
			exp->Dump();
			std::cout << " }";
		}
};

class OpRelExp: public RelExp {
	public:
		std::unique_ptr<RelExp> rel_exp;
		std::string op;
		std::unique_ptr<AddExp> add_exp;
		OpRelExp(Base *p, std::string s, Base *q): RelExp(OPRELEXP),
			rel_exp(static_cast<RelExp*>(p)), op(s), add_exp(static_cast<AddExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "RelExp { ";
			rel_exp->Dump();
			std::cout << " " + op + " ";
			add_exp->Dump();
			std::cout << " }";
		}
};


// EqExp       ::= RelExp | EqExp ("==" | "!=") RelExp;
enum EqExpType {
	RELEQEXP,
	OPEQEXP
};

class EqExp: public Base {
	public:
		EqExpType eq_type;
		EqExp(EqExpType a): eq_type(a){};
		virtual ~EqExp() = default;
		virtual void Dump() const override = 0;
};

class RelEqExp: public EqExp {
	public:
		std::unique_ptr<RelExp> exp;
		RelEqExp(Base *p): EqExp(RELEQEXP), exp(static_cast<RelExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "EqExp { ";
			exp->Dump();
			std::cout << " }";
		}
};

class OpEqExp: public EqExp {
	public:
		std::unique_ptr<EqExp> eq_exp;
		std::string op;
		std::unique_ptr<RelExp> rel_exp;
		OpEqExp(Base *p, std::string s, Base *q): EqExp(OPEQEXP),
			eq_exp(static_cast<EqExp*>(p)), op(s), rel_exp(static_cast<RelExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "EqExp { ";
			eq_exp->Dump();
			std::cout << " " + op + " ";
			rel_exp->Dump();
			std::cout << " }";
		}
};


// LAndExp     ::= EqExp | LAndExp "&&" EqExp;
enum LAndExpType {
	EQLANDEXP,
	OPLANDEXP
};

class LAndExp: public Base {
	public:
		LAndExpType land_type;
		LAndExp(LAndExpType a): land_type(a){}
		virtual ~LAndExp() = default;
		virtual void Dump() const override = 0;
};

class EqLAndExp: public LAndExp {
	public:
		std::unique_ptr<EqExp> exp;
		EqLAndExp(Base *p): LAndExp(EQLANDEXP), exp(static_cast<EqExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "LAndExp { ";
			exp->Dump();
			std::cout << " }";
		}
};

class OpLAndExp: public LAndExp {
	public:
		std::unique_ptr<LAndExp> land_exp;
		std::unique_ptr<EqExp> eq_exp;
		OpLAndExp(Base *p, Base *q): LAndExp(OPLANDEXP),
			land_exp(static_cast<LAndExp*>(p)), eq_exp(static_cast<EqExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "LAndExp { ";
			land_exp->Dump();
			std::cout << " && ";
			eq_exp->Dump();
			std::cout << " }";
		}
};

// LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
enum LOrExpType {
	LANDLOREXP,
	OPLOREXP
};

class LOrExp: public Base {
	public:
		LOrExpType lor_type;
		LOrExp(LOrExpType a): lor_type(a){}
		virtual ~LOrExp() = default;
		virtual void Dump() const override = 0;
};

class LAndLOrExp: public LOrExp {
	public:
		std::unique_ptr<LAndExp> exp;
		LAndLOrExp(Base *p): LOrExp(LANDLOREXP), exp(static_cast<LAndExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "LOrExp { ";
			exp->Dump();
			std::cout << " }";
		};
};

class OpLOrExp: public LOrExp {
	public:
		std::unique_ptr<LOrExp> lor_exp;
		std::unique_ptr<LAndExp> land_exp;
		OpLOrExp(Base *p, Base *q): LOrExp(OPLOREXP),
			lor_exp(static_cast<LOrExp*>(p)), land_exp(static_cast<LAndExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "LOrExp { ";
			lor_exp->Dump();
			std::cout << " || ";
			land_exp->Dump();
			std::cout << " }";
		}
};

// Exp         ::= LOrExp;
class Exp: public Base {
	public:
		std::unique_ptr<LOrExp> exp;
		Exp(Base *p): exp(static_cast<LOrExp*>(p)){}
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