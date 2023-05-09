// Classes for  of SysY

#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include "symtab.hpp"

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


// LVal          ::= IDENT;
class LVal: public Base {
	public:
		std::string ident;
		LVal(std::string s): ident(s){}
		virtual void Dump() const override {
			std::cout << "LVal { " << ident << " }";
		}
};

class Exp;


// PrimaryExp  ::= "(" Exp ")" | Number;
enum PrimaryExpType {
	BRACKETEXP,
	LVALEXP,
	NUMBEREXP
};

class PrimaryExp: public Base {
	public:
		PrimaryExpType exp_type;
		PrimaryExp(PrimaryExpType a): exp_type(a){}
		virtual ~PrimaryExp() override = default;
		virtual void Dump() const override = 0;
		virtual int Eval() const = 0;
};

class BracketExp: public PrimaryExp {
	public:
		std::unique_ptr<Exp> exp;
		BracketExp(Base *p);
		virtual void Dump() const override;
		virtual int Eval() const override;
};

class LValExp: public PrimaryExp {
	public:
		std::unique_ptr<LVal> lval;
		LValExp(Base *p): PrimaryExp(LVALEXP),
			lval(static_cast<LVal*>(p)){}
		virtual void Dump() const override {
			std::cout << "PrimaryExp { ";
			lval->Dump();
			std::cout << " }";
		}
		virtual int Eval() const override {
			auto symb = symtab_stack.GetSymbol(lval->ident);
			assert(symb && symb->symb_type == symtab::CONSTSYMB);
			auto new_symb = static_cast<symtab::ConstSymb*>(symb);
			return new_symb->val;
		}
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
		virtual int Eval() const override {
			return num->int_const;
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
		virtual int Eval() const = 0;
};

class PrimUnaryExp: public UnaryExp {
	public:
		std::unique_ptr<PrimaryExp> prim_exp;
		PrimUnaryExp(Base *p):
			UnaryExp(PRIMUNARYEXP), prim_exp(static_cast<PrimaryExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "UnaryExp { " << std::endl;
			prim_exp->Dump();
			std::cout << " }";
		}
		virtual int Eval() const override {
			return prim_exp->Eval();
		}
};

class OpUnaryExp: public UnaryExp {
	public:
		std::string op;
		std::unique_ptr<UnaryExp> exp;
		OpUnaryExp(std::string s, Base *p): UnaryExp(OPUNARYEXP),
			op(s), exp(static_cast<UnaryExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "UnaryExp { " << op << " ";
			exp->Dump();
			std::cout << " }";
		}
		virtual int Eval() const override {
			int val = exp->Eval();
			if (op == "+")
				return val;
			else if (op == "-")
				return -val;
			return !val;
		}
};


// MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
class MulExp: public Base {
	public:
		std::unique_ptr<MulExp> mul_exp;
		std::string op;
		std::unique_ptr<UnaryExp> unary_exp;
		MulExp(Base *p, std::string s, Base *q): mul_exp(static_cast<MulExp*>(p)),
			op(s), unary_exp(static_cast<UnaryExp*>(q)){}
		virtual void Dump() const override{
			std::cout << "MulExp { ";
			if (mul_exp) {
				mul_exp->Dump();
				std::cout << " " + op + " ";
			}
			unary_exp->Dump();
			std::cout << " }";
		}
		int Eval() const {
			int unary_val = unary_exp->Eval();
			if (mul_exp) {
				int mul_val = mul_exp->Eval();
				if (op == "*")
					return mul_val*unary_val;
				else if (op == "/")
					return mul_val/unary_val;
				return mul_val%unary_val;
			}
			return unary_val;
		}
};


// AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
class AddExp: public Base {
	public:
		std::unique_ptr<AddExp> add_exp;
		std::string op;
		std::unique_ptr<MulExp> mul_exp;
		AddExp(Base *p, std::string s, Base *q): add_exp(static_cast<AddExp*>(p)),
			op(s), mul_exp(static_cast<MulExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "AddExp { ";
			if (add_exp) {
				add_exp->Dump();
				std::cout << " " + op + " ";
			}
			mul_exp->Dump();
			std::cout << " }";
		}
		int Eval() const {
			int mul_val = mul_exp->Eval();
			if (add_exp) {
				int add_val = add_exp->Eval();
				if (op == "+")
					return add_val + mul_val;
				return add_val - mul_val;
			}
			return mul_val;
		}
};

// RelExp      ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
class RelExp: public Base {
	public:
		std::unique_ptr<RelExp> rel_exp;
		std::string op;
		std::unique_ptr<AddExp> add_exp;
		RelExp(Base *p, std::string s, Base *q): rel_exp(static_cast<RelExp*>(p)),
			op(s), add_exp(static_cast<AddExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "RelExp { ";
			if (rel_exp) {
				rel_exp->Dump();
				std::cout << " " + op + " ";
			}
			add_exp->Dump();
			std::cout << " }";
		}
		int Eval() const {
			int add_val = add_exp->Eval();
			if (rel_exp) {
				int rel_val = rel_exp->Eval();
				if (op == "<")
					return rel_val < add_val;
				else if (op == ">")
					return rel_val > add_val;
				else if (op == "<=")
					return rel_val <= add_val;
				return rel_val >= add_val;
			}
			return add_val;
		}
};


// EqExp       ::= RelExp | EqExp ("==" | "!=") RelExp;
class EqExp: public Base {
	public:
		std::unique_ptr<EqExp> eq_exp;
		std::string op;
		std::unique_ptr<RelExp> rel_exp;
		EqExp(Base *p, std::string s, Base *q): eq_exp(static_cast<EqExp*>(p)),
			op(s), rel_exp(static_cast<RelExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "EqExp { ";
			if (eq_exp) {
				eq_exp->Dump();
				std::cout << " " + op + " ";
			}
			rel_exp->Dump();
			std::cout << " }";
		}
		int Eval() const {
			int rel_val = rel_exp->Eval();
			if (eq_exp) {
				int eq_val = eq_exp->Eval();
				if (op == "==")
					return eq_val == rel_val;
				return eq_val != rel_val;
			}
			return rel_val;
		}
};


// LAndExp     ::= EqExp | LAndExp "&&" EqExp;
class LAndExp: public Base {
	public:
		std::unique_ptr<LAndExp> land_exp;
		std::unique_ptr<EqExp> eq_exp;
		LAndExp(Base *p, Base *q): land_exp(static_cast<LAndExp*>(p)),
			eq_exp(static_cast<EqExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "LAndExp { ";
			if (land_exp) {
				land_exp->Dump();
				std::cout << " && ";
			}
			eq_exp->Dump();
			std::cout << " }";
		}
		int Eval() const {
			int eq_val = eq_exp->Eval();
			if (land_exp) {
				int land_val = land_exp->Eval();
				return land_val && eq_val;
			}
			return eq_val;
		}
};


// LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
class LOrExp: public Base {
	public:
		std::unique_ptr<LOrExp> lor_exp;
		std::unique_ptr<LAndExp> land_exp;
		LOrExp(Base *p, Base *q): lor_exp(static_cast<LOrExp*>(p)),
			land_exp(static_cast<LAndExp*>(q)){}
		virtual void Dump() const override {
			std::cout << "LOrExp { ";
			if (lor_exp) {
				lor_exp->Dump();
				std::cout << " || ";
			}
			land_exp->Dump();
			std::cout << " }";
		}
		int Eval() const {
			int land_val = land_exp->Eval();
			if (lor_exp) {
				int lor_val = lor_exp->Eval();
				return lor_val || land_val;
			}
			return land_val;
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
		int Eval() const {
			return exp->Eval();
		}
};


// ConstExp      ::= Exp;
class ConstExp: public Base {
	public:
		std::unique_ptr<Exp> exp;
		ConstExp(Base *p): exp(static_cast<Exp*>(p)){}
		virtual void Dump() const override {
			std::cout << "ConstExp {";
			exp->Dump();
			std::cout << " }";
		}
};


// ConstInitVal  ::= ConstExp;
class ConstInitVal: public Base {
	public:
		std::unique_ptr<ConstExp> const_exp;
		ConstInitVal(Base *p): const_exp(static_cast<ConstExp*>(p)){}
		virtual void Dump() const override {
			std::cout << "ConstInitVal { ";
			const_exp->Dump();
			std::cout << " }";
		}
};


// ConstDef      ::= IDENT "=" ConstInitVal;
class ConstDef: public Base {
	public:
		std::string ident;
		std::unique_ptr<ConstInitVal> val;
		ConstDef(std::string s, Base *p): ident(s),
			val(static_cast<ConstInitVal*>(p)){}
		virtual void Dump() const override {
			std::cout << "ConstDef { " << ident << " = ";
			val->Dump();
			std::cout << " }";
		}
};


// InitVal       ::= Exp;
class InitVal: public Base {
	public:
		std::unique_ptr<Exp> exp;
		InitVal(Base *p): exp(static_cast<Exp*>(p)){}
		virtual void Dump() const override {
			std::cout << "InitVal { ";
			exp->Dump();
			std::cout << " }";
		}
};

// VarDef        ::= IDENT | IDENT "=" InitVal;
class VarDef: public Base {
	public:
		std::string ident;
		std::unique_ptr<InitVal> init_val;
		VarDef(std::string s, Base *p):
			ident(s), init_val(static_cast<InitVal*>(p)){}
		virtual void Dump() const override {
			std::cout << "VarDef { " << ident;
			if (init_val) {
				std::cout << " = ";
				init_val->Dump();
			}
			std::cout << " }";
		}
};

// BType         ::= "int";
class BType: public Base {
	public:
		virtual void Dump() const override {
			std::cout << "BType { int }";
		}
};


// Decl          ::= ConstDecl | VarDecl;
enum DeclType {
	CONSTDECL,
	VARDECL
};

class Decl: public Base {
	public:
		DeclType decl_type;
		Decl(DeclType a): decl_type(a){}
		virtual ~Decl() = default;
		virtual void Dump() const override = 0;
};


// ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
class ConstDecl: public Decl {
	public:
		std::vector<std::unique_ptr<ConstDef> > defs;
		ConstDecl(void *q): Decl(CONSTDECL),
			defs(move(*std::unique_ptr<std::vector<std::unique_ptr<ConstDef> > >(
			static_cast<std::vector<std::unique_ptr<ConstDef> >*>(q)))){}
		virtual void Dump() const override {
			std::cout << "ConstDecl { const int ";
			for (auto it = defs.begin(); it != defs.end(); it ++) {
				(*it)->Dump();
				if (it + 1 != defs.end())
					std::cout << ", ";
				else
					std::cout << ";";
			}
			std::cout << " }";
		}
};


// VarDecl       ::= BType VarDef {"," VarDef} ";";
class VarDecl: public Decl {
	public:
		std::vector<std::unique_ptr<VarDef> > defs;
		VarDecl(void *p): Decl(VARDECL),
			defs(move(*std::unique_ptr<std::vector<std::unique_ptr<VarDef> > >(
			static_cast<std::vector<std::unique_ptr<VarDef> >*>(p)))){}
		virtual void Dump() const override {
			std::cout << "VarDecl { int ";
			for (auto it = defs.begin(); it != defs.end(); it ++) {
				(*it)->Dump();
				if(it + 1 != defs.end())
					std::cout << ", ";
				else
					std::cout << ";";
			}
			std::cout << " }";
		}
};


// Stmt        ::= "return" Exp ";";
enum StmtType {
	ASSIGNSTMT,
	EXPSTMT,
	BLOCKSTMT,
	RETURNSTMT
};

class Stmt: public Base {
	public:
		StmtType stmt_type;
		Stmt(StmtType a): stmt_type(a){}
		virtual ~Stmt() = default;
		virtual void Dump() const override = 0;
};

class AssignStmt: public Stmt {
	public:
		std::unique_ptr<LVal> lval;
		std::unique_ptr<Exp> exp;
		AssignStmt(Base *p, Base *q): Stmt(ASSIGNSTMT),
			lval(static_cast<LVal*>(p)), exp(static_cast<Exp*>(q)){}
		virtual void Dump() const override {
			std::cout << "Stmt { ";
			lval->Dump();
			std::cout << " = ";
			exp->Dump();
			std::cout << " }";
		}
};

class ExpStmt: public Stmt {
	public:
		std::unique_ptr<Exp> exp;
		ExpStmt(Base *p): Stmt(EXPSTMT),
			exp(static_cast<Exp*>(p)){}
		virtual void Dump() const override {
			std::cout << "Stmt { ";
			if (exp)
				exp->Dump();
			std::cout << " ; }";
		}
};

class ReturnStmt: public Stmt {
	public:
		std::unique_ptr<Exp> exp;
		ReturnStmt(Base *p): Stmt(RETURNSTMT),
			exp(static_cast<Exp*>(p)){}
		virtual void Dump() const override {
			std::cout << "Stmt { return ";
			if (exp)
				exp->Dump();
			std::cout << " }";
		}
};

// BlockItem     ::= Decl | Stmt;
enum BlockItemType {
	DECLBLOCKITEM,
	STMTBLOCKITEM
};

class BlockItem: public Base {
	public:
		BlockItemType item_type;
		BlockItem(BlockItemType a): item_type(a){}
		virtual ~BlockItem() = default;
		virtual void Dump() const override = 0;
};

class DeclBlockItem: public BlockItem {
	public:
		std::unique_ptr<Decl> decl;
		DeclBlockItem(Base *p): BlockItem(DECLBLOCKITEM),
			decl(static_cast<Decl*>(p)){}
		virtual void Dump() const override {
			std::cout << "BlockItem { ";
			decl->Dump();
			std::cout << " }";
		}
};

class StmtBlockItem: public BlockItem {
	public:
		std::unique_ptr<Stmt> stmt;
		StmtBlockItem(Base *p): BlockItem(STMTBLOCKITEM),
			stmt(static_cast<Stmt*>(p)){}
		virtual void Dump() const override {
			std::cout << "BlockItem { ";
			stmt->Dump();
			std::cout << " }";
		}
};


// Block         ::= "{" {BlockItem} "}";
// Block         ::= "{" BlockItemList "}";
class Block: public Base {
	public:
		std::vector<std::unique_ptr<BlockItem> > items;
		Block(void *p): items(std::move(*std::unique_ptr<std::vector<std::unique_ptr<BlockItem> > >(
			static_cast<std::vector<std::unique_ptr<BlockItem> >*>(p)))){}
		virtual void Dump() const override {
			std::cout << "Block { ";
			for (const auto &ptr: items) {
				ptr->Dump();
				std::cout << " ";
			}
			std::cout << " }";
		}
};

class BlockStmt: public Stmt {
	public:
		std::unique_ptr<Block> block;
		BlockStmt(Base *p): Stmt(BLOCKSTMT),
			block(static_cast<Block*>(p)){}
		virtual void Dump() const override {
			std::cout << "Stmt { ";
			block->Dump();
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