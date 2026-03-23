#include "Lexer.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include "Debug.h"

// Global lexer state
std::string identifier_str; // Filled in if tok_identifier
double num_val;             // Filled in if tok_number

// Current token
Token cur_tok;

namespace {

int last_char = ' ';
bool lexer_initialized = false;
bool last_char_is_crlf_lf = false;

int advance() {
  const int previous_char = last_char;
  const bool was_initialized = lexer_initialized;
  const bool previous_was_crlf_lf = last_char_is_crlf_lf;

  last_char = std::getchar();
  lexer_initialized = true;
  last_char_is_crlf_lf = false;

  if (last_char == EOF) {
    return last_char;
  }

  if (!was_initialized) {
    LexLoc = {1, 1};
    return last_char;
  }

  if (previous_was_crlf_lf) {
    LexLoc.Col = 1;
    return last_char;
  }

  if (previous_char == '\r') {
    ++LexLoc.Line;
    LexLoc.Col = 1;
    last_char_is_crlf_lf = (last_char == '\n');
    return last_char;
  }

  if (previous_char == '\n') {
    ++LexLoc.Line;
    LexLoc.Col = 1;
    return last_char;
  }

  ++LexLoc.Col;
  return last_char;
}

bool is_space(const int ch) {
  return ch != EOF && std::isspace(static_cast<unsigned char>(ch));
}

bool is_alpha(const int ch) {
  return ch != EOF && std::isalpha(static_cast<unsigned char>(ch));
}

bool is_alnum(const int ch) {
  return ch != EOF && std::isalnum(static_cast<unsigned char>(ch));
}

bool is_digit(const int ch) {
  return ch != EOF && std::isdigit(static_cast<unsigned char>(ch));
}

} // namespace

/// get_tok - Return the next token from standard input.
/// @return The next token from standard input, or Token::k_tok_eof on end of
/// file, or Token::k_tok_error on invalid input.
Token get_tok() {
  if (!lexer_initialized) {
    advance();
  }

  // Skip any whitespace.
  while (is_space(last_char)) {
    advance();
  }

  if (last_char == EOF)
    return Token::k_tok_eof;

  CurLoc = LexLoc;

  if (is_alpha(last_char)) {
    // identifier: [a-zA-Z][a-zA-Z0-9]*
    identifier_str.assign(1, static_cast<char>(last_char));
    while (is_alnum(advance())) {
      identifier_str += static_cast<char>(last_char);
    }

    if (identifier_str == "def") {
      return Token::k_tok_def;
    }
    if (identifier_str == "extern") {
      return Token::k_tok_extern;
    }
    if (identifier_str == "if") {
      return Token::k_tok_if;
    }
    if (identifier_str == "then") {
      return Token::k_tok_then;
    }
    if (identifier_str == "else") {
      return Token::k_tok_else;
    }
    if (identifier_str == "for") {
      return Token::k_tok_for;
    }
    if (identifier_str == "in") {
      return Token::k_tok_in;
    }
    if (identifier_str == "binary") {
      return Token::k_tok_binary;
    }
    if (identifier_str == "unary") {
      return Token::k_tok_unary;
    }
    if (identifier_str == "var") {
      return Token::k_tok_var;
    }
    return Token::k_tok_identifier;
  }
  if (is_digit(last_char) || last_char == '.') {
    // Number: [0-9.]+
    std::string NumStr;
    do {
      NumStr += static_cast<char>(last_char);
      advance();
    } while (is_digit(last_char) || last_char == '.');

    num_val = std::strtod(NumStr.c_str(), nullptr);
    return Token::k_tok_number;
  }

  if (last_char == '#') {
    // Comment until end of line.
    do {
      last_char = advance();
    } while (last_char != EOF && last_char != '\n' && last_char != '\r');

    if (last_char != EOF) {
      return get_tok();
    }
  }

  // Otherwise, just return the character as its ascii value.
  int ThisChar = last_char;
  advance();
  return static_cast<Token>(ThisChar);
}

Token get_next_token() { return cur_tok = get_tok(); }

Token get_current_token() { return cur_tok; }
