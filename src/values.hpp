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
		Value(ValueType a): val_type(a){}
		virtual ~Value() = default;
		virtual std::string Str() const = 0;
};

class SymbolValue: public Value {
	public:
		std::string symbol;
		SymbolValue(std::string s): Value(SYMBOLVALUE), symbol(s){}
		std::string Str() const override {
			return symbol;
		}
};

class IntValue: public Value {
	public:
		int integer;
		IntValue(int x): Value(INTVALUE), integer(x){}
		std::string Str() const override {
			return std::to_string(integer);
		}
};

class UndefValue: public Value {
	public:
		UndefValue(): Value(UNDEFVALUE){}
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
		Initializer(InitType a): init_type(a){}
		virtual ~Initializer() = default;
		virtual std::string Str() const = 0;
};

class IntInit: public Initializer {
	public:
		int integer;
		IntInit(int x): Initializer(INTINIT), integer(x){}
		std::string Str() const override {
			return std::to_string(integer);
		}
};

class UndefInit: public Initializer {
	public:
		UndefInit(): Initializer(UNDEFINIT){}
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
		Aggregate(std::vector<std::unique_ptr<Aggregate> > a): inits(std::move(a)){}
		std::string Str() const {
			std::string s("{");
			for (auto &ptr: inits) {
				s += ptr->Str();
				s += ", ";
			}
			s.erase(s.end() - 2, s.end());
			s += "}";
			return s;
		}
};


}