#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include <queue>
#include <regex>
#include <utility>
#include <unordered_map>

#include "types.h"
#include "section.h"

class Instruction {
public:
	~Instruction();

	static Instruction* extract(std::queue<std::string>& tokens, std::smatch& matches, uint16_t line);

	static Instruction* extract(uint8_t* memory, uint16_t PC);

	friend class Emulator;
	friend class Assembler;
private:
	// used when extracting from assembly
	class Operand {
	public:
		Operand(size_t t_size,
				OperandType t_type,
				AddresingType t_addressing);

		Operand(const std::string& t_value,
				const std::string& t_displacement,
				size_t t_size,
				OperandType t_type,
				AddresingType t_addressing);

		virtual ~Operand() = default;

		std::string value;
		std::string displacement;

		size_t size;

		OperandType type;
		AddresingType addressing;
	};

	// used when extracting from executable
	class CodedOperand : public Operand {
	public:
		CodedOperand(int16_t t_value,
					 int16_t t_displacement,
					 AddresingType t_addressing);

		int16_t value;
		int16_t displacement;
	};

	Instruction(InstructionCode t_code,
				size_t t_size,
				OperandSize t_operandSize,
				Operand* t_destination,
				Operand* t_source);

	static uint8_t operands[];

	static std::unordered_map<std::string, std::pair<InstructionCode, uint8_t>> instructionMap;

	InstructionCode code;
	size_t size;
	OperandSize operandSize;
	Operand* destination;
	Operand* source;
};

#endif