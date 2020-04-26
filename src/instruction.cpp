#include <queue>
#include <regex>
#include <unordered_map>

#include "exceptions.h"
#include "types.h"
#include "utils.h"
#include "section.h"
#include "instruction.h"

using namespace std;


uint8_t Instruction::operands[] = { 0, 0, 2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0 };


unordered_map<string, pair<InstructionCode, uint8_t>> Instruction::instructionMap = {
	{ "halt", { HALT, 0 } },
	{ "xchg", { XCHG, 2 } },
	{ "int",  { INT,  1 } },
	{ "mov",  { MOV,  2 } },
	{ "add",  { ADD,  2 } },
	{ "sub",  { SUB,  2 } },
	{ "mul",  { MUL,  2 } },
	{ "div",  { DIV,  2 } },
	{ "cmp",  { CMP,  2 } },
	{ "not",  { NOT,  1 } },
	{ "and",  { AND,  2 } },
	{ "or",   { OR,   2 } },
	{ "xor",  { XOR,  2 } },
	{ "test", { TEST, 2 } },
	{ "shl",  { SHL,  2 } },
	{ "shr",  { SHR,  2 } },
	{ "push", { PUSH, 1 } },
	{ "pop",  { POP,  1 } },
	{ "jmp",  { JMP,  1 } },
	{ "jeq",  { JEQ,  1 } },
	{ "jne",  { JNE,  1 } },
	{ "jgt",  { JGT,  1 } },
	{ "call", { CALL, 1 } },
	{ "ret",  { RET,  0 } },
	{ "iret", { IRET, 0 } }
};


Instruction::Operand::Operand(size_t t_size,
							  OperandType t_type,
							  AddresingType t_addressing) :
	size(t_size), type(t_type), addressing(t_addressing) {}


Instruction::Operand::Operand(const std::string& t_value,
							  const std::string& t_displacement,
							  size_t t_size,
							  OperandType t_type,
							  AddresingType t_addressing) :
	value(t_value), displacement(t_displacement), size(t_size), type(t_type), addressing(t_addressing) {}


Instruction::CodedOperand::CodedOperand(int16_t t_value,
										  int16_t t_displacement,
										  AddresingType t_addressing) :
	Operand(0, PSW, t_addressing), value(t_value), displacement(t_displacement) {}


Instruction::Instruction(InstructionCode t_code,
						 size_t t_size,
						 OperandSize t_operandSize,
						 Operand* t_destination,
						 Operand* t_source) :
	code(t_code), size(t_size), operandSize(t_operandSize), destination(t_destination), source(t_source) {}


Instruction::~Instruction() {
	delete destination;
	delete source;
}


Instruction* Instruction::extract(queue<string>& tokens, smatch& matches, uint16_t line) {
	string token;
	string mnemonic = matches[1];
	string suffix   = matches[2].matched ? matches[2].str() : "w";

	InstructionCode code = instructionMap[mnemonic].first;
	uint8_t operands = instructionMap[mnemonic].second;

	OperandSize operandSize;

	if (suffix == "b")
		operandSize = BYTE;
	else
		operandSize = WORD;

	size_t size = BYTE;
	Operand* destination = nullptr;
	Operand* source = nullptr;

	if (operands >= 1) {
		if (tokens.empty())
			throw AssemblingException(line, "Missing first operand!");

		token = tokens.front();
		tokens.pop();

		TokenType tokenType = Utils::getTokenType(token, matches);

		switch (tokenType) {
		case SYMBOL:
			if (Utils::isJump(mnemonic))
				destination = new Operand(matches[0], "", WORD + BYTE, IMMED_SYMBOL, IMMED);
			else
				destination = new Operand(matches[0], "", WORD + BYTE, MEMORY_SYMBOL, MEMORY);
			size += WORD + BYTE;

			break;
		case SYMBOL_IMMED:
			if (mnemonic != "int" && mnemonic != "push")
				throw AssemblingException(line, "Immediate value is not allowed as a destination operand!");

			destination = new Operand(matches[1], "", operandSize + BYTE, IMMED_SYMBOL, IMMED);
			size += operandSize + BYTE;

			break;
		case SYMBOL_PCREL:
			destination = new Operand("r7", matches[1], WORD + BYTE, PCRELATIVE, REG_IND_16);
			size += WORD + BYTE;

			break;
		case OPERAND_REG: {
			if (Utils::isJump(mnemonic))
				throw AssemblingException(line, "Invalid addressing type for jump instructions!");

			string value = matches[1];

			if (matches[2].matched && operandSize != BYTE)
				throw AssemblingException(line, "Byte indicator isn't expected for WORD operand size!");

			if (value == "psw") {
				destination = new Operand(value, matches[2], BYTE, PSW, REG_DIR);
			}
			else {
				if (value == "sp") value = "r6";
				else if (value == "pc") value = "r7";
				
				if (value == "r5" && mnemonic == "div")
					throw AssemblingException(line, "Register R5 is reserved for the remainder of division!");

				destination = new Operand(value, matches[2], BYTE, REGISTER, REG_DIR);
			}

			size += BYTE;

			break;
		}
		case OPERAND_REGIND: {
			if (Utils::isJump(mnemonic))
				throw AssemblingException(line, "Invalid addressing type for jump instructions!");

			string value = matches[1];

			if (value == "psw") {
				destination = new Operand(value, "", BYTE, PSW, REG_IND);
			}
			else {
				if (value == "sp") value = "r6";
				else if (value == "pc") value = "r7";
				
				if (value == "r5" && mnemonic == "div")
					throw AssemblingException(line, "Register R5 is reserved for the remainder of division!");

				destination = new Operand(value, "", BYTE, REGISTER, REG_IND);
			}

			size += BYTE;

			break;
		}
		case OPERAND_REGINDDISP: {
			if (Utils::isJump(mnemonic))
				throw AssemblingException(line, "Invalid addressing type for jump instructions!");
			
			string value = matches[1];

			if (value == "sp") value = "r6";
			else if (value == "pc") value = "r7";
			else if (value == "r5" && mnemonic == "div")
				throw AssemblingException(line, "Register R5 is reserved for the remainder of division!");

			OperandType type;
			if (regex_match(matches[2].str(), regex("^[a-zA-Z_]\\w*$")))
				type = DISPL_SYMBOL;
			else
				type = DISPL_VALUE;

			AddresingType addressing = REG_IND_16;
			OperandSize displacementSize = WORD;

			if (type == DISPL_VALUE) {
				int16_t value = strtol(matches[2].str().c_str(), NULL, 0);

				if ((value & 0xFF00) == 0) {
					addressing = REG_IND_8;
					displacementSize = BYTE;
				}
			}

			destination = new Operand(value, matches[2], displacementSize + BYTE, type, addressing);
			size += displacementSize + BYTE;

			break;
		}
		case OPERAND_IMMED:
			if (mnemonic != "int" && mnemonic != "push")
				throw AssemblingException(line, "Immediate value is not allowed as a destination operand!");

			destination = new Operand(matches[0], "", operandSize + BYTE, IMMED_VALUE, IMMED);
			size += operandSize + BYTE;

			break;
		case OPERAND_MEMORY:
			if (Utils::isJump(mnemonic))
				throw AssemblingException(line, "Invalid addressing type for jump instructions!");

			destination = new Operand(matches[1], "", WORD + BYTE, MEMORY_VALUE, MEMORY);
			size += WORD + BYTE;

			break;
		default:
			throw AssemblingException(line, "Illegal operand format!");
			break;
		}
	}

	if (operands == 2) {
		if (tokens.empty())
			throw AssemblingException(line, "Missing second operand!");

		token = tokens.front();
		tokens.pop();

		TokenType tokenType = Utils::getTokenType(token, matches);

		switch (tokenType) {
		case SYMBOL:
			source = new Operand(matches[0], "", WORD + BYTE, MEMORY_SYMBOL, MEMORY);
			size += WORD + BYTE;

			break;
		case SYMBOL_IMMED:
			if (mnemonic == "xchg")
				throw AssemblingException(line, "Immediate value isn't legal operand for \"xchg\" instruction!");

			source = new Operand(matches[1], "", operandSize + BYTE, IMMED_SYMBOL, IMMED);
			size += operandSize + BYTE;

			break;
		case SYMBOL_PCREL:
			source = new Operand("r7", matches[1], WORD + BYTE, PCRELATIVE, REG_IND_16);
			size += WORD + BYTE;

			break;
		case OPERAND_REG: {
			string value = matches[1];

			if (matches[2].matched && operandSize == WORD)
				throw AssemblingException(line, "Byte indicator isn't expected for WORD operand size!");

			if (!matches[2].matched && operandSize == BYTE)
				throw AssemblingException(line, "Byte indicator must be specified for BYTE operand size!");

			if (value == "psw") {
				source = new Operand(value, matches[2], BYTE, PSW, REG_DIR);
			}
			else {
				if (value == "sp") value = "r6";
				else if (value == "pc") value = "r7";

				source = new Operand(value, matches[2], BYTE, REGISTER, REG_DIR);
			}

			size += BYTE;

			break;
		}
		case OPERAND_REGIND: {
			string value = matches[1];

			if (value == "psw") {
				source = new Operand(value, "", BYTE, PSW, REG_IND);
			}
			else {
				if (value == "sp") value = "r6";
				else if (value == "pc") value = "r7";

				source = new Operand(value, "", BYTE, REGISTER, REG_IND);
			}

			size += BYTE;

			break;
		}
		case OPERAND_REGINDDISP: {
			string value = matches[1];

			if (value == "sp") value = "r6";
			else if (value == "pc") value = "r7";
			
			OperandType type;
			if (regex_match(matches[2].str(), regex("^[a-zA-Z_]\\w*$")))
				type = DISPL_SYMBOL;
			else
				type = DISPL_VALUE;

			AddresingType addressing = REG_IND_16;
			OperandSize displacementSize = WORD;

			if (type == DISPL_VALUE) {
				int16_t value = strtol(matches[2].str().c_str(), NULL, 0);

				if ((value & 0xFF00) == 0) {
					addressing = REG_IND_8;
					displacementSize = BYTE;
				}
			}

			source = new Operand(value, matches[2], displacementSize + BYTE, type, addressing);
			size += displacementSize + BYTE;

			break;
		}
		case OPERAND_IMMED:
			if (mnemonic == "xchg")
				throw AssemblingException(line, "Immediate value isn't legal operand for \"xchg\" instruction!");

			source = new Operand(matches[0], "", operandSize + BYTE, IMMED_VALUE, IMMED);
			size += operandSize + BYTE;

			break;
		case OPERAND_MEMORY:
			source = new Operand(matches[1], "", WORD + BYTE, MEMORY_VALUE, MEMORY);
			size += WORD + BYTE;

			break;
		default:
			throw AssemblingException(line, "Illegal operand format!");
			break;
		}
	}

	return new Instruction(code, size, operandSize, destination, source);
}

Instruction* Instruction::extract(uint8_t* memory, uint16_t PC) {
	uint8_t byte = memory[PC++];

	size_t size = BYTE;

	InstructionCode code = (InstructionCode)(byte >> CODE_OFFSET);
	OperandSize operandSize = (OperandSize)((byte >> SIZE_OFFSET & 1) + 1);

	uint8_t operandNumber = Instruction::operands[code];

	Operand* operands[2]{ nullptr, nullptr };

	for (int i = 0; i < operandNumber; i++) {
		byte = memory[PC++];
		size += BYTE;

		AddresingType addressing = (AddresingType)(byte >> ADDR_OFFSET);
		
		switch (addressing) {
		case IMMED: {
			int16_t value = memory[PC++];

			if (operandSize == WORD)
				value |= memory[PC++] << 8;

			operands[i] = new CodedOperand(value, 0, addressing);

			size += operandSize;
			break;
		}
		case REG_DIR: {
			int16_t value = byte >> REGS_OFFSET & 0xF;

			operands[i] = new CodedOperand(value, byte & 1, addressing);

			break;
		}
		case REG_IND: {
			int16_t value = byte >> REGS_OFFSET & 0xF;

			operands[i] = new CodedOperand(value, 0, addressing);

			break;
		}
		case REG_IND_8: {
			int16_t value = byte >> REGS_OFFSET & 0xF;

			operands[i] = new CodedOperand(value, memory[PC++], addressing);

			size += BYTE;
			break;
		}
		case REG_IND_16: {
			int16_t value = byte >> REGS_OFFSET & 0xF;

			int16_t displacement = memory[PC++];
			displacement |= memory[PC++] << 8;

			operands[i] = new CodedOperand(value, displacement, addressing);

			size += WORD;
			break;
		}
		case MEMORY: {
			int16_t address = memory[PC++];
			address |= memory[PC++] << 8;

			operands[i] = new CodedOperand(address, 0, addressing);

			size += WORD;
			break;
		}
		default:
			throw EmulatingException("Unrecognized addressing type!");
			break;
		}
	}

	return new Instruction(code, size, operandSize, operands[0], operands[1]);
}