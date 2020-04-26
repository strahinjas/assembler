#include <iostream>

#include "types.h"


std::ostream& operator<<(std::ostream& out, ScopeType scopeType) {
	switch (scopeType) {
	case GLOBAL:
		return out << "Global";

	case LOCAL:
		return out << "Local";
	}
}


std::ostream& operator<<(std::ostream& out, SymbolType symbolType) {
	switch (symbolType) {
	case SymbolType::LABEL:
		return out << "Label";

	case SymbolType::SECTION:
		return out << "Section";

	case SymbolType::CONSTANT:
		return out << "Constant";

	case SymbolType::ALIAS:
		return out << "Alias";

	case SymbolType::EXTERN:
		return out << "Extern";

	case SymbolType::UNRESOLVED:
		return out << "Unresolved";
	}
}


std::ostream& operator<<(std::ostream& out, RelocationType relocationType) {
	switch (relocationType) {
	case R_386_8:
		return out << "R_386_8";

	case R_386_SUB_8:
		return out << "R_386_SUB_8";

	case R_386_16:
		return out << "R_386_16";

	case R_386_SUB_16:
		return out << "R_386_SUB_16";

	case R_386_PC16:
		return out << "R_386_PC16";

	case R_386_SUB_PC16:
		return out << "R_386_SUB_PC16";
	}
}