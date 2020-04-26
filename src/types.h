#ifndef _TYPES_H_
#define _TYPES_H_

#include <regex>
#include <iostream>

constexpr uint8_t PSW_CODE = 0xF;

constexpr uint8_t CODE_OFFSET = 3;
constexpr uint8_t SIZE_OFFSET = 2;
constexpr uint8_t ADDR_OFFSET = 5;
constexpr uint8_t REGS_OFFSET = 1;

constexpr uint8_t WIDTH = 15;

constexpr auto UNDEFINED = "N/A";


const std::regex objectFile("\\.o$");
const std::regex assemblyFile("\\.s$");


enum ScopeType : uint8_t { GLOBAL, LOCAL };
std::ostream& operator<<(std::ostream& out, ScopeType scopeType);


enum TokenType : uint8_t {
	GLOBAL_EXTERN,
	LABEL,
	SECTION,
	SECTION_FLAGS,
	SECTION_NAME,
	DIRECTIVE,
	INSTRUCTION,
	SYMBOL,
	SYMBOL_IMMED,
	SYMBOL_PCREL,
	OPERAND_REG,
	OPERAND_REGIND,
	OPERAND_REGINDDISP,
	OPERAND_IMMED,
	OPERAND_MEMORY,
	EXPRESSION,
	INVALID
};


// W (write), A (alloc), X (execute), M (merge), S (strings)
// I (info), L (link order), G (group), T (TLS), E (exclude)
enum Flag : uint8_t { W, A, X, M, S, I, L, G, T, E };


// made as "strong enum"
// to differ from TokenType
enum class SymbolType : uint8_t {
	LABEL,
	SECTION,
	CONSTANT,
	ALIAS,
	EXTERN,
	UNRESOLVED
};
std::ostream& operator<<(std::ostream& out, SymbolType symbolType);


enum AddresingType : uint8_t {
	IMMED,
	REG_DIR,
	REG_IND,
	REG_IND_8,
	REG_IND_16,
	MEMORY
};


enum OperandSize : uint8_t { BYTE = 1, WORD = 2 };


enum OperandType : uint8_t {
	IMMED_VALUE,
	IMMED_SYMBOL,
	REGISTER,
	DISPL_VALUE,
	DISPL_SYMBOL,
	MEMORY_VALUE,
	MEMORY_SYMBOL,
	PCRELATIVE,
	PSW
};


enum InstructionCode : uint8_t {
	HALT = 1,
	XCHG, INT, MOV,
	ADD,  SUB, MUL, DIV, CMP,
	NOT,  AND, OR,  XOR, TEST,
	SHL,  SHR,
	PUSH, POP,
	JMP,  JEQ, JNE, JGT,
	CALL, RET, IRET
};


enum RelocationType : uint8_t {
	R_386_8,
	R_386_SUB_8,
	R_386_16,
	R_386_SUB_16,
	R_386_PC16,
	R_386_SUB_PC16
};
std::ostream& operator<<(std::ostream& out, RelocationType relocationType);


enum InterruptType : uint8_t {
	INITIALIZATION,
	INSTRUCTION_ERROR,
	TIMER,
	TERMINAL
};

#endif