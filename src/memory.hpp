#pragma once

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "types.hpp"
#include "values.hpp"


namespace koopa {

// MemoryDeclaration ::= "alloc" Type;
class MemoryDec {
	public:
		std::shared_ptr<Type> mem_type;
		MemoryDec(std::shared_ptr<Type> a): mem_type(a){}
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
			mem_type(a), mem_init(std::move(b)){}
		std::string Str() const {
			return "alloc " + mem_type->Str() + ", " + mem_init->Str();
		}
};


// Load ::= "load" SYMBOL;
class Load {
	public:
		std::string symbol;
		Load(std::string s): symbol(s){}
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
		Store(StoreType a, std::string s): store_type(a), symbol(s){}
		virtual ~Store() = default;
		virtual std::string Str() const = 0;
};

class ValueStore: public Store {
	public:
		std::unique_ptr<Value> val;
		ValueStore(std::unique_ptr<Value> p, std::string s):
			Store(VALUESTORE, s), val(std::move(p)){}
		std::string Str() const override {
			return "store " + val->Str() + ", " + symbol;
		}
};

class InitStore: public Store {
	public:
		std::unique_ptr<Initializer> init;
		InitStore(std::unique_ptr<Initializer> p, std::string s):
			Store(INITSTORE, s), init(std::move(p)){}
		std::string Str() const override {
			return "store " + init->Str() + ", " + symbol;
		}
};


}