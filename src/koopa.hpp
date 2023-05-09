// All classes for koopa

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "types.hpp"


namespace koopa {

// Value ::= SYMBOL | INT | "undef";
enum ValueType {
	SYMBOLVALUE,
	INTVALUE,
	UNDEFVALUE
};

class Value {
	public:
		ValueType val_type;
		Value(ValueType a): val_type(a) {}
		virtual ~Value() = default;
		virtual std::string Str() const = 0;
};

class SymbolValue: public Value {
	public:
		std::string symbol;
		SymbolValue(std::string s): Value(SYMBOLVALUE), symbol(s) {}
		std::string Str() const override {
			return symbol;
		}
};

class IntValue: public Value {
	public:
		int integer;
		IntValue(int x): Value(INTVALUE), integer(x) {}
		std::string Str() const override {
			return std::to_string(integer);
		}
};

class UndefValue: public Value {
	public:
		UndefValue(): Value(UNDEFVALUE) {}
		std::string Str() const override {
			return "undef";
		}
};


// Initializer ::= INT | "undef" | Aggregate | "zeroinit";
enum InitType {
	INTINIT,
	UNDEFINIT,
	AGGREGATEINIT,
	ZEROINIT
};

class Aggregate;

class Initializer {
	public:
		InitType init_type;
		Initializer(InitType a): init_type(a) {}
		virtual ~Initializer() = default;
		virtual std::string Str() const = 0;
};

class IntInit: public Initializer {
	public:
		int integer;
		IntInit(int x): Initializer(INTINIT), integer(x) {}
		std::string Str() const override {
			return std::to_string(integer);
		}
};

class UndefInit: public Initializer {
	public:
		UndefInit(): Initializer(UNDEFINIT) {}
		std::string Str() const override {
			return "undef";
		}
};

class AggregateInit: public Initializer {
	public:
		std::unique_ptr<Aggregate> aggr;
		AggregateInit(std::unique_ptr<Aggregate> a);
		std::string Str() const override;
};


// Aggregate ::= "{" Initializer {"," Initializer} "}";
class Aggregate {
	public:
		std::vector<std::unique_ptr<Aggregate> > inits;
		Aggregate(std::vector<std::unique_ptr<Aggregate> > a): inits(std::move(a)) {}
		std::string Str() const {
			std::string s("{");
			for (const auto &ptr: inits) {
				s += ptr->Str();
				s += ", ";
			}
			s.erase(s.end() - 2, s.end());
			s += "}";
			return s;
		}
};


// BinaryExpr ::= BINARY_OP Value "," Value;
class BinaryExpr {
	public:
		std::string op;
		std::unique_ptr<Value> val1, val2;
		BinaryExpr(std::string s, std::unique_ptr<Value> x, std::unique_ptr<Value> y):
			op(s), val1(std::move(x)), val2(std::move(y)) {}
		std::string Str() const {
			return op + " " + val1->Str() + ", " + val2->Str();
		}
};


// MemoryDeclaration ::= "alloc" Type;
class MemoryDec {
	public:
		std::shared_ptr<Type> mem_type;
		MemoryDec(std::shared_ptr<Type> a): mem_type(a) {}
		std::string Str() const {
			return "alloc " + mem_type->Str();
		}
};


// GlobalMemoryDeclaration ::= "alloc" Type "," Initializer;
class GlobalMemDec {
	public:
		std::shared_ptr<Type> mem_type;
		std::unique_ptr<Initializer> mem_init;
		GlobalMemDec(std::shared_ptr<Type> a, std::unique_ptr<Initializer> b):
			mem_type(a), mem_init(std::move(b)) {}
		std::string Str() const {
			return "alloc " + mem_type->Str() + ", " + mem_init->Str();
		}
};


// Load ::= "load" SYMBOL;
class Load {
	public:
		std::string symbol;
		Load(std::string s): symbol(s) {}
		std::string Str() const {
			return "load " + symbol;
		}
};


// Store ::= "store" (Value | Initializer) "," SYMBOL;
enum StoreType {
	VALUESTORE,
	INITSTORE
};

class Store {
	public:
		StoreType store_type;
		std::string symbol;
		Store(StoreType a, std::string s): store_type(a), symbol(s) {}
		virtual ~Store() = default;
		virtual std::string Str() const = 0;
};

class ValueStore: public Store {
	public:
		std::unique_ptr<Value> val;
		ValueStore(std::unique_ptr<Value> p, std::string s):
			Store(VALUESTORE, s), val(std::move(p)) {}
		std::string Str() const override {
			return "store " + val->Str() + ", " + symbol;
		}
};

class InitStore: public Store {
	public:
		std::unique_ptr<Initializer> init;
		InitStore(std::unique_ptr<Initializer> p, std::string s):
			Store(INITSTORE, s), init(std::move(p)) {}
		std::string Str() const override {
			return "store " + init->Str() + ", " + symbol;
		}
};


// Branch ::= "br" Value "," SYMBOL "," SYMBOL;
class Branch {
	public:
		std::unique_ptr<Value> val;
		std::string symbol1, symbol2;
		Branch(std::unique_ptr<Value> p, std::string a, std::string b):
			val(std::move(p)), symbol1(a), symbol2(b) {}
		std::string Str() const {
			return "br " + val->Str() + ", " + symbol1 + ", " + symbol2;
		}
};


// Jump ::= "jump" SYMBOL;
class Jump {
	public:
		std::string symbol;
		Jump(std::string s): symbol(s) {}
		std::string Str() const {
			return "jump " + symbol;
		}
};


// FunCall ::= "call" SYMBOL "(" [Value {"," Value}] ")";
class FunCall {
	public:
		std::string symbol;
		std::vector<std::unique_ptr<Value> > params;
		FunCall(std::string s, std::vector<std::unique_ptr<Value> > v):
			symbol(s), params(std::move(v)) {}
		std::string Str() const {
			std::string s("call" + symbol + "(");
			if (!params.empty()) {
				for (const auto &p: params)
					s += p->Str() + ", ";
				s.erase(s.end() - 2, s.end());
			}
			return s + ")";
		}
};


// Return ::= "ret" [Value];
class Return {
	public:
		std::unique_ptr<Value> val;
		Return(std::unique_ptr<Value> p): val(std::move(p)) {}
		std::string Str() const {
			if (val)
				return "ret " + val->Str();
			else
				return "ret";
		}
};


// GetPointer ::= "getptr" SYMBOL "," Value;
class GetPointer {
	public:
		std::string symbol;
		std::unique_ptr<Value> val;
		GetPointer(std::string s, std::unique_ptr<Value> p):
			symbol(s), val(std::move(p)) {}
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
			symbol(s), val(std::move(p)) {}
		std::string Str() const {
			return "getelemptr " + symbol + ", " + val->Str();
		}
};


// SymbolDef ::= SYMBOL "=" (MemoryDeclaration | Load | GetPointer | BinaryExpr | FunCall);
enum SymbolDefType {
	MEMORYDEF,
	LOADDEF,
	GETPTRDEF,
	BINEXPRDEF,
	FUNCALLDEF
};

class SymbolDef {
	public:
		SymbolDefType def_type;
		std::string symbol;
		SymbolDef(SymbolDefType a, std::string s): def_type(a), symbol(s) {}
		virtual ~SymbolDef() = default;
		virtual std::string Str() const = 0;
};

class MemoryDef: public SymbolDef {
	public:
		std::unique_ptr<MemoryDec> mem_dec;
		MemoryDef(std::string s, std::unique_ptr<MemoryDec> p):
			SymbolDef(MEMORYDEF, s), mem_dec(std::move(p)) {}
		std::string Str() const override {
			return symbol + " = " + mem_dec->Str();
		}
};

class LoadDef: public SymbolDef {
	public:
		std::unique_ptr<Load> load;
		LoadDef(std::string s, std::unique_ptr<Load> p):
			SymbolDef(LOADDEF, s), load(std::move(p)) {}
		std::string Str() const override {
			return symbol + " = " + load->Str();
		}
};

class GetPtrDef: public SymbolDef {
	public:
		std::unique_ptr<GetPointer> get_ptr;
		GetPtrDef(std::string s, std::unique_ptr<GetPointer> p):
			SymbolDef(GETPTRDEF, s), get_ptr(std::move(p)) {}
		std::string Str() const override {
			return symbol + " = " + get_ptr->Str();
		}
};

class BinExprDef: public SymbolDef {
	public:
		std::unique_ptr<BinaryExpr> bin_expr;
		BinExprDef(std::string s, std::unique_ptr<BinaryExpr> p):
			SymbolDef(BINEXPRDEF, s), bin_expr(std::move(p)) {}
		std::string Str() const override {
			return symbol + " = " + bin_expr->Str();
		}
};

class FunCallDef: public SymbolDef {
	public:
		std::unique_ptr<FunCall> fun_call;
		FunCallDef(std::string s, std::unique_ptr<FunCall> p):
			SymbolDef(FUNCALLDEF, s), fun_call(std::move(p)) {}
		std::string Str() const override {
			return symbol + " = " + fun_call->Str();
		}
};


// GlobalSymbolDef ::= "global" SYMBOL "=" GlobalMemoryDeclaration;
class GlobalSymbolDef {
	public:
		std::string symbol;
		std::unique_ptr<GlobalMemDec> mem_dec;
		GlobalSymbolDef(std::string s, std::unique_ptr<GlobalMemDec> p):
			symbol(s), mem_dec(std::move(p)) {}
		std::string Str() const {
			return "global " + symbol + " = " + mem_dec->Str();
		}
};


// Statement ::= SymbolDef | Store | FunCall;
enum StatementType {
	SYMBOLDEFSTMT,
	STORESTMT,
	FUNCALLSTMT
};

class Statement {
	public:
		StatementType stmt_type;
		Statement(StatementType a): stmt_type(a) {}
		virtual ~Statement() = default;
		virtual std::string Str() const = 0;
};

class SymbolDefStmt: public Statement {
	public:
		std::unique_ptr<SymbolDef> sym_def;
		SymbolDefStmt(std::unique_ptr<SymbolDef> p):
			Statement(SYMBOLDEFSTMT), sym_def(std::move(p)) {}
		std::string Str() const override {
			return "\t" + sym_def->Str();
		}
};

class StoreStmt: public Statement {
	public:
		std::unique_ptr<Store> store;
		StoreStmt(std::unique_ptr<Store> p):
			Statement(STORESTMT), store(std::move(p)) {}
		std::string Str() const override {
			return "\t" + store->Str();
		}
};

class FunCallStmt: public Statement {
	public:
		std::unique_ptr<FunCall> fun_call;
	FunCallStmt(std::unique_ptr<FunCall> p):
		Statement(FUNCALLSTMT), fun_call(std::move(p)) {}
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
		EndStatement(EndStmtType a): end_type(a) {}
		virtual ~EndStatement() = default;
		virtual std::string Str() const = 0;
};

class BranchEnd: public EndStatement {
	public:
		std::unique_ptr<Branch> branch;
		BranchEnd(std::unique_ptr<Branch> p):
			EndStatement(BRANCHEND), branch(std::move(p)) {}
		std::string Str() const override {
			return "\t" + branch->Str();
		}
};

class JumpEnd: public EndStatement {
	public:
		std::unique_ptr<Jump> jump;
		JumpEnd(std::unique_ptr<Jump> p):
			EndStatement(JUMPEND), jump(std::move(p)) {}
		std::string Str() const override {
			return "\t" + jump->Str();
		}
};

class ReturnEnd: public EndStatement {
	public:
		std::unique_ptr<Return> ret;
		ReturnEnd(std::unique_ptr<Return> p):
			EndStatement(RETURNEND), ret(std::move(p)) {}
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
			symbol(s), stmts(std::move(v)), end_stmt(std::move(p)) {}
		std::string Str() const {
			std::string s(symbol + ":\n");
			for (const auto &ptr: stmts)
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
			blocks(std::move(s)) {}
		std::string Str() const {
			std::string s;
			for (const auto &ptr: blocks)
				s += ptr->Str() + "\n";
			return s;
		}
};


// FunParams ::= SYMBOL ":" Type {"," SYMBOL ":" Type};
class FunParams {
	public:
		std::vector<std::pair<std::string, std::shared_ptr<Type> > > params;
		FunParams(std::vector<std::pair<std::string, std::shared_ptr<Type> > > v):
			params(std::move(v)) {}
		std::string Str() const {
			std::string s;
			for (const auto &pr: params)
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
			symbol(s), params(std::move(a)), ret_type(b), body(std::move(c)) {}
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


class Program {
	public:
		std::vector<std::unique_ptr<GlobalSymbolDef> > global_vars;
		std::vector<std::unique_ptr<FunDef> > funcs;
		Program(std::vector<std::unique_ptr<GlobalSymbolDef> > p,
			std::vector<std::unique_ptr<FunDef> > q):
			global_vars(std::move(p)), funcs(std::move(q)) {}
		std::string Str() const {
			std::string s;
			for (const auto &ptr: global_vars)
				s += ptr->Str() + "\n";
			for (const auto &ptr: funcs)
				s += ptr->Str();
			return s;
		}
};


}