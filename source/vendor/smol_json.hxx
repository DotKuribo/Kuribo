/*
	smol_json
	* A single-header, tiny, non-allocating, streamed JSON parser library for modern C++
	* December 2019, riidefi

Defines:
	- SMOL_JSON_DEBUG:
		* Define this to enable assertions.

	- SMOL_JSON_ABORT(msg):
		* Define this for a user abort function.
		* Called when SMOL_JSON_DEBUG is defined and an assertion fails.

	- SMOL_JSON_LOG_FN(msg, ...):
		* Define this for a user log function.
		* Only called when SMOL_JSON_DEBUG is defined.
		* Defaults to printf with message prefix "[smol_json]".

	- SMOL_JSON_STRING_VIEW:
		* Define this to std::string_view/eastl::string_view.
		* Defaults to internal implementation smol_json::string_view


Examples:
	* Constructing a parser
		smol_json::Parser parser(std::string_view("[ 1, 2 ]"));

	* Parsing a string-keyed object:
		// { "integer_key": 42, "boolean_key": false }
		bool success = parser.stringKeyIter([&](auto key) {
			if (key == "integer_key" && parser.expectInt())
				printf("integer_key is %i\n", parser.getInt());
			else if (key == "boolean_key" && parser.expectBool())
				printf("boolean_key is %s\n", parser.getBool() ? "ON" : "OFF");
			else return false;

			return true;
		});

	* Parsing an array
		// [ -1, true, { ... } ]
		bool success = parser.arrayIter([&](int index) {
			if (!parser.readTok()) return false;

			printf("[%i] ", index);

			if (parser.isInt())
				printf("%i\n", parser.getInt());
			else if (parser.isBool())
				printf("%s\n", parser.getBool() ? "ON" : "OFF");
			else if (parser.isObject())
				return parse_some_object(parser);
			else
				return printf("Error.\n") && false;

			return true;
		});

	
Classes:
	- smol_json::string_view:
		* Simple string_view implementation for standalone.
		* Implements generic comparison operators against any class that implements data() and length().

	- smol_json::Stream:
		* Simple stream based on a string_view.

	- smol_json::Token:
		* A simple token type.
		* When read, values are not cleared immediately: reading a string and then a colon will still keep the string value.

	- smol_json::Lexer(smol_json::Token, smol_json::Stream):
		* Simple JSON lexer. Currently does not support floating point data.

	- smol_json::Parser(smol_json::Lexer):
		* Simple JSON parser interface.
		* Defines easy iterative APIs
*/

#pragma once

#include <stdint.h>
#include <cstring>

#ifndef SMOL_JSON_LOG_FN
#include <stdio.h>
#define SMOL_JSON_LOG_FN(msg, ...) printf("[smol_json] " msg, __VA_ARGS__)
#endif

#ifndef SMOL_JSON_STRING_VIEW

namespace smol_json {

struct string_view
{
	string_view(const char* s, uint32_t l)
		: str(s), len(l)
	{}
	string_view(const char* s)
		: str(s), len(strlen(s))
	{}

	inline const char* data() const { return str; }
	inline uint32_t length() const { return len; }
	
	template<typename T>
	bool operator==(const T& rhs) const
	{
		if (rhs.length() != length()) return false;
		return memcmp(rhs.data(), data(), length());
	}
	template<typename T>
	bool operator!=(const T& rhs) const
	{
		return !operator==(rhs);
	}
private:
	const char* str;
	uint32_t len;
};

}
#define SMOL_JSON_STRING_VIEW smol_json::string_view
#endif

#ifndef SMOL_JSON_ABORT
#ifdef _WIN32
#define SMOL_JSON_ABORT(at) throw at
#else
#define SMOL_JSON_ABORT(at)
#endif
#endif


#ifdef SMOL_JSON_DEBUG
#define SW_FAIL(str) doFail(sizeof(str) > 20 ? \
		"Line: " LINE_STRING : \
		"Line: " LINE_STRING " " str)
#else
#define SW_FAIL(...) false
#endif
#define SW_EXPECT(expr) (expr || SW_FAIL( #expr))
#define SW_ENSURE(expr) if (!SW_EXPECT(expr)) return false;

#ifdef SMOL_JSON_DEBUG
inline bool doFail(const char* at)
{
	SMOL_JSON_ABORT(at);
	SMOL_JSON_LOG_FN("SW_FAIL: %s\n", at);
	return false;
}
#endif

namespace smol_json {

// TStringView must implement data() and length() and be copyable
// Designed for eastl::string_view/std::string_view
// smol_json::string_view has been provided for standalone implementations
struct Stream
{
	Stream(SMOL_JSON_STRING_VIEW string) noexcept
		: base(string), cursor(string.data())
	{}
	
	inline const char* begin() noexcept { return base.data(); }
	inline const char* end() noexcept { return base.end(); }
	inline bool isEof(int ofs) noexcept { return cursor + ofs >= base.end(); }
	
	inline bool safeInc(int ofs) noexcept
	{
		if (isEof(ofs))
		{
			cursor = end();
			return false;
		}
		
		cursor += ofs; 
		return true;
	}
	inline char get()
	{
		if (isEof(0))
			return 0xff;
		
		const char c = *cursor;
		safeInc(1);
		return c;
	}
	
	inline const char* getPtr() { return cursor; }
	
	inline char peek(int ofs=1)
	{
		if (isEof(ofs)) return 0xff;

		return *(cursor + ofs);
	}

private:
	const char* cursor;
	SMOL_JSON_STRING_VIEW base;
};


enum class TokenType
{
	None,
	OpenObject,
	CloseObject,

	OpenArray,
	CloseArray,

	String,
	Int,
	Float, //!< Unsupported

	Comma,
	Colon,

	Bool,

	Max
};

class Token
{
	friend class Lexer;
public:
	inline bool isType(TokenType other) const
	{
		return t == other;
	}
	inline TokenType getType() const
	{
		return t;
	}
	inline bool isNone() const { return isType(TokenType::None); }
	inline bool isOpenObject() const { return isType(TokenType::OpenObject); }
	inline bool isCloseObject() const { return isType(TokenType::CloseObject); }
	inline bool isOpenArray() const { return isType(TokenType::OpenArray); }
	inline bool isCloseArray() const { return isType(TokenType::CloseArray); }
	inline bool isString() const { return isType(TokenType::String); }
	inline bool isInt() const { return isType(TokenType::Int); }
	inline bool isFloat() const { return isType(TokenType::Float); }
	inline bool isComma() const { return isType(TokenType::Comma); }
	inline bool isColon() const { return isType(TokenType::Colon); }
	inline bool isBool() const { return isType(TokenType::Bool); }

	inline SMOL_JSON_STRING_VIEW getString() const { return string; }
	inline int getInt() const { return i; }
	inline bool getBool() const { return b; }
protected:
	TokenType t;
	SMOL_JSON_STRING_VIEW string;

	int i;
	float f;
	bool b;
};

class Lexer : public Stream, public Token // Single token stream
{
public:
	constexpr bool isWhiteSpace(char c) const
	{
		return  c == ' '  ||
				c == '\n' ||
				c == '\t' ||
				c == '\r';
	}

	inline char skipWs() noexcept
	{
		if (isEof(0))
			return 0xff;
		
		char first = get();
		for (; isWhiteSpace(first); first = get())
			if (isEof(0))
				return 0xff;

		return first;
	}

	template<char c, TokenType type>
	inline bool expectSimple() noexcept
	{
		char first = skipWs();
		if (first == 0xff)
			return false;

		if (first == c)
		{
			t = type;
			return true;
		}

		return false;
	}

	inline bool expectOpenArray() noexcept { return expectSimple<'[', TokenType::OpenArray>(); }
	inline bool expectCloseArray() noexcept { return expectSimple<']', TokenType::CloseArray>(); }
	inline bool expectOpenObject() noexcept { return expectSimple<'{', TokenType::OpenObject>(); }
	inline bool expectCloseObject() noexcept { return expectSimple<'}', TokenType::CloseObject>(); }
	inline bool expectComma() noexcept { return expectSimple<',', TokenType::Comma>(); }
	inline bool expectColon() noexcept { return expectSimple<':', TokenType::Colon>(); }
	inline bool isInt(char first) noexcept { return (first >= '0' && first <= '9') || first == '-'; }
	
	inline bool parseInt(char first, Token* tok = nullptr)
	{
		if (!tok) tok = this;

		if (!isInt(first))
			return false;

		tok->t = TokenType::Int;
		tok->i = 0;

		for (const char* c = getPtr() + (first == '-' ? 0 : -1); c < end(); ++c)
		{
			if (*c >= '0' && *c <= '9')
			{
				tok->i = tok->i * 10 + *c - '0';
			}
			else
			{
				// Not a digit, but still in stream
				SW_EXPECT(safeInc(c - getPtr()));

				if (first == '-') tok->i *= -1;

				return true;
			}
		}
		// Ended by EOF: Assume truncation

		return false;
	}
	inline bool parseString(char first, Token* tok = nullptr) noexcept
	{
		if (!tok) tok = this;

		if (first != '\"')
			return false;
		
		tok->t = TokenType::String;
		const char* begin = getPtr();

		for (char c = get(); c != 0xff; c = get())
		{
			if (c == '\"')
			{
				tok->string = { begin, u32(getPtr() - begin - 1) };
				return true;
			}
		}
		return false;
	}
	inline bool parseBool(char first, Token* tok=nullptr) noexcept
	{
		if (!tok) tok = this;

		if (first == 't')
		{
			if (peek(0) == 'r' &&
				peek(1) == 'u' &&
				peek(2) == 'e')
			{
				safeInc(3);
				tok->t = TokenType::Bool;
				tok->b = true;

				return true;
			}
		}
		if (first == 'f')
		{
			if (peek(0) == 'a' &&
				peek(1) == 'l' &&
				peek(2) == 's' &&
				peek(3) == 'e')
			{
				safeInc(4);
				t = TokenType::Bool;
				b = false;

				return true;
			}
		}
		return false;
	}

	template<typename T>
	inline bool expectComplex(T f)
	{
		char first = skipWs();
		if (first == 0xff)
			return false;
		return f(first);
	}

	inline bool expectInt() { return expectComplex([&](char c) { return parseInt(c); }); }
	inline bool expectString() { return expectComplex([&](char c) { return parseString(c); }); }
	inline bool expectBool() { return expectComplex([&](char c) { return parseBool(c); }); }

	inline bool expectStringKey() { return expectString() && expectColon(); }
	
	bool readTok(Token* tok = nullptr, bool seekback = false)
	{
		if (!tok) tok = this;

		struct OptSeekBack
		{
			OptSeekBack(Stream& s, bool d)
				: backup(src), src(s), doIt(d)
			{}
			~OptSeekBack()
			{
				if (doIt)
					src = backup;
			}
			bool doIt;
			Stream& src;
			Stream backup;
		};
		OptSeekBack sb(*this, seekback);

		char first = skipWs();

		SW_ENSURE(first != 0xff);

		switch (first)
		{
		case ',':
			tok->t = TokenType::Comma;
			return true;
		case '[':
			tok->t = TokenType::OpenArray;
			return true;
		case ']':
			tok->t = TokenType::CloseArray;
			return true;
		case '{':
			tok->t = TokenType::OpenObject;
			return true;
		case '}':
			tok->t = TokenType::CloseObject;
			return true;
		case ':':
			tok->t = TokenType::Colon;
			return true;
		case 't':
		case 'f':
			return parseBool(first, tok);
		case '\"':
			return parseString(first, tok);
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
			return parseInt(first, tok);
		default:
			break;
		}

		return SW_FAIL("Can't read token.");
	}

	Lexer(SMOL_JSON_STRING_VIEW d)
		: Stream(d)
	{}
};

struct Parser : public Lexer
{
	Parser(SMOL_JSON_STRING_VIEW s)
		: Lexer(s)
	{}

	// Formerly JSONStringKeyIter
	template<typename T>
	bool stringKeyIter(T f)
	{
		SW_ENSURE(expectOpenObject());
		while (readTok())
		{
			switch (getType())
			{
			case TokenType::String:
				if (!expectColon())
					return SW_FAIL("Expected a colon.");

				if (!f(getString()))
					return SW_FAIL("User action failed.");

				break;
			case TokenType::CloseObject:
				return true;
			case TokenType::Comma:
				continue;
			default:
				return SW_FAIL("Invalid token.");
			}
		}

		return SW_FAIL("Unable to read token");
	}
	

	template<typename T>
	bool arrayIter(T f)
	{
		SW_ENSURE(expectOpenArray());

		for (int i = 0; f(i); ++i)
		{
			SW_ENSURE(readTok());

			switch (getType())
			{
			case TokenType::Comma:
				continue;
			case TokenType::CloseArray:
				return true;
			default:
				return SW_FAIL("Unexpected token.");
			}
		}
		return SW_FAIL("User action failed.");
	}
};

} // namespace smol_json