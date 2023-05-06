// Classes for AST of SysY

#pragma once

#include<memory>
#include<string>
#include<iostream>


namespace sysy {


class BaseAST {
	public:
		virtual ~BaseAST() = default;
		virtual void Dump() const = 0;
};


class NumberAST: public BaseAST {
	public:
		int int_const;
		NumberAST(int x): int_const(x){}
		void Dump() const override {
			std::cout << "NumberAST { " << int_const << " }";
		}
};



// Stmt      ::= "return" Number ";";
class StmtAST: public BaseAST {
	public:
		std::unique_ptr<NumberAST> number;
		StmtAST(BaseAST *p): number(static_cast<NumberAST*>(p)){}
		void Dump() const override {
			std::cout << "StmtAST { ";
			number->Dump();
			std::cout << " }";
		}
};


// Block     ::= "{" Stmt "}";
class BlockAST: public BaseAST {
	public:
		std::unique_ptr<StmtAST> stmt;
		BlockAST(BaseAST *p): stmt(static_cast<StmtAST*>(p)){}
		void Dump() const override {
			std::cout << "BlockAST { ";
			stmt->Dump();
			std::cout << " }";
		}
};



// FuncType  ::= "int";
class FuncTypeAST: public BaseAST {
	public:
		void Dump() const override {
			std::cout << "FuncTypeAST { int }";
		}
};



// FuncDef   ::= FuncType IDENT "(" ")" Block;
class FuncDefAST: public BaseAST {
	public:
		std::unique_ptr<FuncTypeAST> func_type;
		std::string ident;
		std::unique_ptr<BlockAST> block;
		FuncDefAST(BaseAST *p, std::string s, BaseAST *q):
			func_type(static_cast<FuncTypeAST*>(p)), ident(s),
			block(static_cast<BlockAST*>(q)){}
		void Dump() const override {
			std::cout << "FuncDefAST { ";
			func_type->Dump();
			std::cout << ", " << ident << ", ";
			block->Dump();
			std::cout << " }";
		}
};

// CompUnit  ::= FuncDef;
class CompUnitAST: public BaseAST {
	public:
		std::unique_ptr<FuncDefAST> func_def;
		CompUnitAST(BaseAST *p): func_def(static_cast<FuncDefAST*>(p)){}
		void Dump() const override {
			std::cout << "CompUnitAST { ";
			func_def->Dump();
			std::cout << " }";
		}
};


}