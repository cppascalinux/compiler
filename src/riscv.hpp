#pragma once

#include <string>
#include <vector>
#include <memory>

namespace riscv {

enum ItemType {
	PSEUDOOP,
	LABEL,
	INSTR
};

class Item {
	public:
		ItemType item_type;
		Item(ItemType a): item_type(a) {}
		virtual ~Item() = default;
		virtual std::string Str() const = 0;
};

class PseudoOp: public Item {
	public:
		std::string op, args;
		PseudoOp(std::string op, std::string args):
			Item(PSEUDOOP), op(op), args(args) {}
		virtual std::string Str() const override {
			std::string s = op;
			if (!args.empty())
				s += " " + args;
			return s + "\n";
		}
};

class Label: public Item {
	public:
		std::string name;
		Label(std::string name): Item(LABEL), name(name) {}
		virtual std::string Str() const override {
			return name + ":\n";
		}
};

enum InstrType {
	REGINSTR, // instr rd, rs1[, rs2]
	IMMINSTR, // instr rd[, rs], imm/imm12
	LABELINSTR // instr [rd, ]label
};

class Instr: public Item {
	public:
		InstrType instr_type;
		std::string op;
		Instr(InstrType a, std::string op):
			Item(INSTR), instr_type(a), op(op) {}
		virtual ~Instr() = default;
		virtual std::string Str() const override = 0;
};


// instr rd, rs1[, rs2]

// add/sub/slt/sgt/xor/or/and/sll/srl/sra/mul/dev/rem rd, rs1, rs2
// seqz/snez/mv rd, rs1
class RegInstr: public Instr {
	public:
		std::string rd, rs1, rs2;
		RegInstr(std::string op, std::string rd, std::string rs1, std::string rs2):
			Instr(REGINSTR, op), rd(rd), rs1(rs1), rs2(rs2) {}
		virtual std::string Str() const override {
			std::string s = "\t" + op + " " + rd + ", " + rs1;
			if (!rs2.empty())
				s += ", " + rs2;
			return s + "\n";
		}
};


// instr rd[, rs], imm/imm12

// lw/sw rd, imm12(rs)
// addi/xori/ori/andi rd, rs, imm12
// li rd, imm
class ImmInstr: public Instr {
	public:
		std::string rd, rs;
		int imm;
		ImmInstr(std::string op, std::string rd, std::string rs, int imm):
			Instr(IMMINSTR, op), rd(rd), rs(rs), imm(imm) {}
		virtual std::string Str() const override {
			std::string s = "\t" + op + " ";
			if (op == "lw" || op == "sw")
				return s + rd + ", " + std::to_string(imm) + "(" + rs + ")\n";
			s += rd;
			if (!rs.empty())
				s += ", " + rs;
			s += ", " + std::to_string(imm) + "\n";
			return s;
		}
};


// instr [rd, ][label]

// beqz/bnez/la rd, label
// j/call label
// ret
class LabelInstr: public Instr {
	public:
		std::string rd, label;
		LabelInstr(std::string op, std::string rd, std::string label):
			Instr(LABELINSTR, op), rd(rd), label(label) {}
		virtual std::string Str() const override {
			if (!rd.empty() && !label.empty())
				return "\t" + op + " " + rd + ", " + label + "\n";
			else if (!label.empty())
				return "\t" + op + " " + label + "\n";
			else
				return "\t" + op;
		}
};

}
