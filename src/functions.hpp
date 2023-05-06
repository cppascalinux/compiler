#pragma once

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "types.hpp"
#include "values.hpp"
#include "symboldef.hpp"
#include "jump_branch.hpp"


namespace koopa {

// Statement ::= SymbolDef | Store | FunCall;
enum StatementType {
	SYMBOLDEFSTMT,
	STORESTMT,
	FUNCALLSTMT
};

class Statement {
	public:
		StatementType stmt_type;
		Statement(StatementType a): stmt_type(a){}
		virtual ~Statement() = default;
		virtual std::string Str() const = 0;
};

class SymbolDefStmt: public Statement {
	public:
		std::unique_ptr<SymbolDef> sym_def;
		SymbolDefStmt(std::unique_ptr<SymbolDef> p):
			Statement(SYMBOLDEFSTMT), sym_def(std::move(p)){}
		std::string Str() const override {
			return "\t" + sym_def->Str();
		}
};

class StoreStmt: public Statement {
	public:
		std::unique_ptr<Store> store;
		StoreStmt(std::unique_ptr<Store> p):
			Statement(STORESTMT), store(std::move(p)){}
		std::string Str() const override {
			return "\t" + store->Str();
		}
};

class FunCallStmt: public Statement {
	public:
		std::unique_ptr<FunCall> fun_call;
	FunCallStmt(std::unique_ptr<FunCall> p):
		Statement(FUNCALLSTMT), fun_call(std::move(p)){}
	std::string Str() const override {
		return "\t" + fun_call->Str();
	}
};


// EndStatement ::= Branch | Jump | Return;
enum EndStmtType {
	BRANCHEND,
	JUMPEND,
	RETURNEND
};

class EndStatement {
	public:
		EndStmtType end_type;
		EndStatement(EndStmtType a): end_type(a){}
		virtual ~EndStatement() = default;
		virtual std::string Str() const = 0;
};

class BranchEnd: public EndStatement {
	public:
		std::unique_ptr<Branch> branch;
		BranchEnd(std::unique_ptr<Branch> p):
			EndStatement(BRANCHEND), branch(std::move(p)){}
		std::string Str() const override {
			return "\t" + branch->Str();
		}
};

class JumpEnd: public EndStatement {
	public:
		std::unique_ptr<Jump> jump;
		JumpEnd(std::unique_ptr<Jump> p):
			EndStatement(JUMPEND), jump(std::move(p)){}
		std::string Str() const override {
			return "\t" + jump->Str();
		}
};

class ReturnEnd: public EndStatement {
	public:
		std::unique_ptr<Return> ret;
		ReturnEnd(std::unique_ptr<Return> p):
			EndStatement(RETURNEND), ret(std::move(p)){}
		std::string Str() const override {
			return "\t" + ret->Str();
		}
};


// Block ::= SYMBOL ":" {Statement} EndStatement;
class Block {
	public:
		std::string symbol;
		std::vector<std::unique_ptr<Statement> > stmts;
		std::unique_ptr<EndStatement> end_stmt;
		Block(std::string s, std::vector<std::unique_ptr<Statement> > v,
			std::unique_ptr<EndStatement> p):
			symbol(s), stmts(std::move(v)), end_stmt(std::move(p)){}
		std::string Str() const {
			std::string s(symbol + ":\n");
			for (auto &ptr: stmts)
				s += ptr->Str() + "\n";
			s += end_stmt->Str();
			return s;
		}
};


// FunBody ::= {Block};
class FunBody {
	public:
		std::vector<std::unique_ptr<Block> > blocks;
		FunBody(std::vector<std::unique_ptr<Block> > s):
			blocks(std::move(s)){}
		std::string Str() const {
			std::string s;
			for (auto &ptr: blocks)
				s += ptr->Str() + "\n";
			return s;
		}
};


// FunParams ::= SYMBOL ":" Type {"," SYMBOL ":" Type};
class FunParams {
	public:
		std::vector<std::pair<std::string, std::shared_ptr<Type> > > params;
		FunParams(std::vector<std::pair<std::string, std::shared_ptr<Type> > > v):
			params(std::move(v)){}
		std::string Str() const {
			std::string s;
			for (auto &pr: params)
				s += pr.first + ": " + pr.second->Str() + ", ";
			s.erase(s.end()-2, s.end());
			return s;
		}
};


// FunDef ::= "fun" SYMBOL "(" [FunParams] ")" [":" Type] "{" FunBody "}";
class FunDef {
	public:
		std::string symbol;
		std::unique_ptr<FunParams> params;
		std::shared_ptr<Type> ret_type;
		std::unique_ptr<FunBody> body;
		FunDef(std::string s, std::unique_ptr<FunParams> a,
			std::shared_ptr<Type> b, std::unique_ptr<FunBody> c):
			symbol(s), params(std::move(a)), ret_type(b), body(std::move(c)){}
		std::string Str() const {
			std::string s("fun " + symbol + "(");
			if (params)
				s += params->Str();
			s += ")";
			if (ret_type)
				s += ": " + ret_type->Str();
			s += " {\n" + body->Str() + "}\n";
			return s;
		}
};

}