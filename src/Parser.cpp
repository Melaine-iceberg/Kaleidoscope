#include "Parser.h"

#include <cctype>
#include <cstdio>
#include <map>

#include "Debug.h"
#include "Lexer.h"

extern Token cur_tok;

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
std::map<Token, int> BinopPrecedence{};
std::unique_ptr<ExprAST> ParseIfExpr();
std::unique_ptr<ExprAST> ParseForExpr();
std::unique_ptr<ExprAST> ParseVarExpr();

static std::unique_ptr<ExprAST> ParseUnary();

/// expression
///   ::= primary binoprhs
///
std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParseUnary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

/// numberexpr ::= number
std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto result = std::make_unique<NumberExprAST>(CurLoc, num_val);
  get_next_token(); // consume the number
  return result;
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> ParseParenExpr() {
  get_next_token(); // eat (.
  auto expr = ParseExpression();
  if (!expr) {
    return nullptr;
  }

  if (get_current_token() != static_cast<Token>(')')) {
    return LogError("expected ')'");
  }
  get_next_token(); // eat ).
  return expr;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string id_name{identifier_str}; // Remember the identifier name

  get_next_token(); // eat identifier.

  if (get_current_token() != static_cast<Token>('(')) { // Simple variable ref.
    return std::make_unique<VariableExprAST>(CurLoc, std::move(id_name));
  }

  // Call.
  get_next_token(); // eat (
  // Parse the argument list.
  std::vector<std::unique_ptr<ExprAST>> args; // Argument expressions.

  if (get_current_token() !=
      static_cast<Token>(')')) { // If the next token is not ')', we have to
    while (true) {               // Parse the arguments.
      if (auto arg = ParseExpression()) {
        args.push_back(std::move(arg));
      } else {
        return nullptr;
      }

      if (get_current_token() == static_cast<Token>(')')) {
        break;
      }

      if (get_current_token() != static_cast<Token>(',')) {
        return LogError("Expected ')' or ',' in argument list");
      }
      get_next_token(); // eat the ','
    }
  }

  // Eat the ')'.
  get_next_token();

  return std::make_unique<CallExprAST>(CurLoc, std::move(id_name),
                                       std::move(args));
}

/// ParsePrimary - This function handles primary expressions, which can be
/// identifiers, numbers, or parenthesized expressions.
/// @return A unique pointer to the parsed expression AST, or nullptr on error.
std::unique_ptr<ExprAST> ParsePrimary() {
  switch (get_current_token()) {
  case Token::k_tok_identifier:
    return ParseIdentifierExpr();
  case Token::k_tok_number:
    return ParseNumberExpr();
  case static_cast<Token>('('):
    return ParseParenExpr();
  case Token::k_tok_if:
    return ParseIfExpr();
  case Token::k_tok_for:
    return ParseForExpr();
  case Token::k_tok_var:
    return ParseVarExpr();
  default:
    return LogError("unknown token when expecting an expression");
  }
}

/// ParseUnary - This function parses unary operators, which can be either a
/// single operator followed by a primary expression, or just a primary
/// expression if there is no unary operator.
/// @param ExprPrec The precedence of the unary operator, if present. This is
/// used to determine how to parse the expression correctly.
/// @param LHS The left-hand side expression, if the unary operator is present.
/// This is used to construct the AST for the unary expression.
/// @return A unique pointer to the parsed expression AST, or nullptr on error.
std::unique_ptr<ExprAST> ParseBinOpRHS(const int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    const int tok_prec =
        GetTokPrecedence(); // Get the precedence of the current token.

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (tok_prec < ExprPrec) {
      return LHS;
    }
    // Okay, we know this is a binop.
    Token bin_op = get_current_token();
    get_next_token(); // eat binop

    // Parse the primary expression after the binary operator.
    auto rhs{ParseUnary()};
    if (!rhs) {
      return nullptr;
    }
    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    if (const int next_prec{GetTokPrecedence()}; tok_prec < next_prec) {
      rhs = ParseBinOpRHS(tok_prec + 1, std::move(rhs));
      if (!rhs) {
        return nullptr;
      }
    }

    // Merge LHS/RHS.
    LHS = std::make_unique<BinaryExprAST>(CurLoc, bin_op, std::move(LHS),
                                          std::move(rhs));
  } // loop around to the top of the while loop.
}

/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER number? '(' id id ')'
///   ::= unary LETTER '(' id ')'
/// @return A unique pointer to the parsed PrototypeAst, or nullptr on error.
std::unique_ptr<PrototypeAST> ParsePrototype() {
  std::string fn_name;

  unsigned Kind = 0; // 0 = identifier, 1 = unary, 2 = binary.
  unsigned BinaryPrecedence = 30;

  switch (cur_tok) {
  default:
    return LogErrorP("Expected function name in prototype");
  case Token::k_tok_identifier:
    fn_name = identifier_str;
    Kind = 0;
    get_next_token();
    break;
  case Token::k_tok_unary:
    get_next_token();
    if (!isascii(static_cast<int>(cur_tok))) {
      return LogErrorP("Expected unary operator");
    }
    fn_name = "unary";
    fn_name += static_cast<char>(cur_tok);
    Kind = 1;
    get_next_token();
    break;
  case Token::k_tok_binary:
    get_next_token();
    if (!isascii(static_cast<int>(cur_tok))) {
      return LogErrorP("Expected binary operator");
    }
    fn_name = "binary";
    fn_name += static_cast<char>(cur_tok);
    Kind = 2;
    get_next_token();

    // Read the precedence if present.
    if (cur_tok == Token::k_tok_number) {
      if (num_val < 1 || num_val > 100) {
        return LogErrorP("Invalid precedence: must be 1..100");
      }
      BinaryPrecedence = static_cast<unsigned>(num_val);
      get_next_token();
    }
    break;
  }

  if (cur_tok != static_cast<Token>('(')) {
    return LogErrorP("Expected '(' in prototype");
  }

  std::vector<std::string> ArgNames;
  get_next_token(); // eat '('
  while (cur_tok == Token::k_tok_identifier) {
    ArgNames.push_back(identifier_str);
    get_next_token();
  }
  if (cur_tok != static_cast<Token>(')')) {
    return LogErrorP("Expected ')' in prototype");
  }

  // success.
  get_next_token(); // eat ')'.

  // Verify right number of names for operator.
  if (Kind && ArgNames.size() != Kind) {
    return LogErrorP("Invalid number of operands for operator");
  }

  return std::make_unique<PrototypeAST>(fn_name, std::move(ArgNames), Kind != 0,
                                        BinaryPrecedence);
}

/// definition ::= 'def' prototype expression
std::expected<std::unique_ptr<FunctionAST>, std::string_view>
ParseDefinition() {
  get_next_token();

  auto proto = ParsePrototype();
  if (!proto) {
    return std::unexpected("Failed to parse function prototype");
  }

  auto body = ParseExpression();
  if (!body) {
    return std::unexpected("Failed to parse function body");
  }

  return std::make_unique<FunctionAST>(std::move(proto), std::move(body));
}

/// external ::= 'extern' prototype
std::expected<std::unique_ptr<PrototypeAST>, std::string_view> ParseExtern() {
  get_next_token(); // eat extern.

  auto proto = ParsePrototype();
  if (!proto) [[unlikely]] {
    return std::unexpected("Failed to parse extern prototype");
  }
  return proto;
}

/// toplevelexpr ::= expression
template <typename ExprType>
  requires std::derived_from<ExprType, ExprAST>
std::unique_ptr<FunctionAST> WrapAsTopLevel(std::unique_ptr<ExprType> expr) {
  auto proto = std::make_unique<PrototypeAST>(
      "__anon_expr", std::vector<std::string>{}, false, 0);
  return std::make_unique<FunctionAST>(std::move(proto), std::move(expr));
}

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(std::string_view str) {
  std::fprintf(stderr, "Error: %.*s\n", static_cast<int>(str.size()),
               str.data());
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(std::string_view str) {
  LogError(str);
  return nullptr;
}

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
int GetTokPrecedence() {
  Token cur_tok = get_current_token();
  if (!isascii(static_cast<int>(cur_tok)))
    return -1;

  // Make sure it's a declared binop.
  const int tok_precedence = BinopPrecedence[cur_tok];
  if (tok_precedence <= 0) {
    return -1;
  }
  return tok_precedence;
}

// Explicit template instantiation for ExprAST
template std::unique_ptr<FunctionAST>
WrapAsTopLevel<ExprAST>(std::unique_ptr<ExprAST> expr);

// Initialize binary operator precedence
void InitializeBinopPrecedence() {
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence[static_cast<Token>('=')] = 2;
  BinopPrecedence[static_cast<Token>('<')] = 10;
  BinopPrecedence[static_cast<Token>('+')] = 20;
  BinopPrecedence[static_cast<Token>('-')] = 20;
  BinopPrecedence[static_cast<Token>('*')] = 40; // highest.
}

/// unary
///   ::= primary
///   ::= '!' unary
static std::unique_ptr<ExprAST> ParseUnary() {
  // If the current token is not an operator, it must be a primary expr.
  if (!isascii(static_cast<int>(cur_tok)) ||
      cur_tok == static_cast<Token>('(') ||
      cur_tok == Token::k_tok_identifier || cur_tok == Token::k_tok_number)
    return ParsePrimary();

  // If this is a unary operator, read it.
  int Opc = static_cast<int>(cur_tok);
  get_next_token();
  if (auto Operand = ParseUnary())
    return std::make_unique<UnaryExprAST>(CurLoc, Opc, std::move(Operand));
  return nullptr;
}
