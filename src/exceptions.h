#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

#include <string>
#include <exception>

class AssemblingException : public std::exception {
public:
	AssemblingException(const std::string& error) {
		message = "ASSEMBLING ERROR: " + error;
	}

	AssemblingException(uint32_t line, const std::string& error) {
		message = "ASSEMBLING ERROR(line " + std::to_string(line) + "): " + error;
	}

	const char* what() const noexcept override {
		return message.c_str();
	}
private:
	std::string message;
};


class LinkingException : public std::exception {
public:
	LinkingException(const std::string& error) {
		message = "LINKING ERROR: " + error;
	}

	const char* what() const noexcept override {
		return message.c_str();
	}
private:
	std::string message;
};


class EmulatingException : public std::exception {
public:
	EmulatingException(const std::string& error) {
		message = "EMULATING ERROR: " + error;
	}

	const char* what() const noexcept override {
		return message.c_str();
	}
private:
	std::string message;
};

#endif