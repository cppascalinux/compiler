#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <vector>



namespace koopa {

// Type ::= "i32" | ArrayType | PointerType | FunType;
enum TypeNum {
	INTTYPE,
	ARRAYTYPE,
	POINTERTYPE,
	FUNTYPE
};

class Type {
	public:
		TypeNum my_type;
		Type(TypeNum type): my_type(type){}
		virtual ~Type() = default;
		virtual std::string Str() const = 0;
};

class IntType: public Type {
	public:
		IntType(): Type(INTTYPE){};
		std::string Str() const override {
			return "i32";
		}
};

// ArrayType ::= "[" Type "," INT "]";
class ArrayType: public Type {
	public:
		std::shared_ptr<Type> arr;
		int len;
		ArrayType(std::shared_ptr<Type> type, int length):
			Type(ARRAYTYPE), arr(type), len(length){}
		std::string Str() const override {
			return "[" + arr->Str() + ", " + std::to_string(len) + "]";
		}
};

// PointerType ::= "*" Type;
class PointerType: public Type {
	public:
		std::shared_ptr<Type> ptr;
		PointerType(std::shared_ptr<Type> type): Type(POINTERTYPE), ptr(type){}
		std::string Str() const override {
			return "*" + ptr->Str();
		}
};

// FunType ::= "(" [Type {"," Type}] ")" [":" Type];
class FunType: public Type {
	public:
		std::vector<std::shared_ptr<Type> > params;
		std::shared_ptr<Type> ret;
		FunType(std::vector<std::shared_ptr<Type> > p, std::shared_ptr<Type> q):
			Type(FUNTYPE), params(p), ret(q){}
		std::string Str() const override {
			std::string s("(");
			if (!params.empty()) {
				for (auto &ptr: params)
					s += ptr->Str() + ", ";
				s.erase(s.end() - 2, s.end());
			}
			s += ")";
			if (ret)
				s += ": " + ret->Str();
			return s;
		}
};


}