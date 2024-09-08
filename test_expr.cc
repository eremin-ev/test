/*
 * TODO license
 *
 * Based on chapter 2 [1] of My First Language Frontend with LLVM Tutorial [2]:
 *
 * [1] 2. Kaleidoscope: Implementing a Parser and AST
 *     https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html
 *
 * [2] My First Language Frontend with LLVM Tutorial
 *     https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html
 */

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5
};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number

std::string show(const std::string &s)
{
    return s;
}

template <typename Ptr>
std::string show(const Ptr &p)
{
    return p->value();
}

template <typename Ptr>
std::string dump_list(const std::vector<Ptr> &input, char sep = ',')
{
    std::string output;
    bool first = true;
    for (const auto &i : input) {
        if (first) {
            output += '(';
        } else {
            output += sep;
        }
        output += show(i);
        first = false;
    }

    if (!first) {
        output += ')';
    }

    return output;
}

// gettok - Return the next token from standard input.
static int gettok()
{
    static int LastChar = ' ';

    // Skip any whitespace.
    while (isspace(LastChar)) {
        LastChar = getchar();
    }

    // identifier: [a-zA-Z][a-zA-Z0-9]*
    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;

        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;

        if (IdentifierStr == "def")
            return tok_def;

        if (IdentifierStr == "extern")
            return tok_extern;

        return tok_identifier;
    }

    // Number: [0-9.]+
    if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '#') {
        // Comment until end of line.
        do {
            LastChar = getchar();
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF) {
            return gettok();
        }
    }

    // Check for end of file.  Don't eat the EOF.
    if (LastChar == EOF) {
        return tok_eof;
    }

    // Otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    LastChar = getchar();

    return ThisChar;
}

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

namespace AST {

enum class ExpressionType {
    NumberExpr = 1,
    VariableExpr,
    VariableRef,
    BinaryExpr,
    CallExpr,
};

static std::string type_repr(ExpressionType type)
{
    switch (type) {
    case ExpressionType::NumberExpr:
        return "NumberExpr";
    case ExpressionType::VariableExpr:
        return "VariableExpr";
    case ExpressionType::VariableRef:
        return "VariableRef";
    case ExpressionType::BinaryExpr:
        return "BinaryExpr";
    case ExpressionType::CallExpr:
        return "CallExpr";
    }

    return "UnknownType";
}

static std::string func_signature(const std::string &name,
                                  const std::vector<std::string> &argv)
{
    std::string out = name;
    out += '(';
    bool first = true;
    char sep = ',';
    // Improvement: argument type
    char arg_type = 'd';
    for (unsigned i = 0; i < argv.size(); ++i) {
        if (!first) {
            out += sep;
        }
        out += arg_type;
        first = false;
    }
    out += ')';
    return out;
}

// Expression - Base class for all expression nodes.
class Expression {
public:
    virtual ~Expression()
    {}

    virtual ExpressionType type() const = 0;
    virtual std::string name() const = 0;
    virtual std::string value() const = 0;
};

// Prototype - This class represents the "prototype" for a function,
// which captures its name, and its argument names (thus implicitly the number
// of arguments the function takes).
class Prototype {
private:
    std::string m_name;
    std::vector<std::string> m_argv;

public:
    Prototype(const std::string &name,
              std::vector<std::string> argv)
    : m_name{std::move(name)}
    , m_argv{std::move(argv)}
    {}

    const std::string &name() const
    {
        return m_name;
    }

    const std::vector<std::string> argv() const
    {
        return m_argv;
    }
};

// Function - This class represents a function definition itself.
class Function {
private:
    std::unique_ptr<Prototype> m_proto;
    std::unique_ptr<Expression> m_body;

public:
    Function(std::unique_ptr<Prototype> proto,
             std::unique_ptr<Expression> body)
    : m_proto{std::move(proto)}
    , m_body{std::move(body)}
    {}

    std::string name() const
    {
        return func_signature(m_proto->name(), m_proto->argv());
    }

    const Prototype *proto() const
    {
        return m_proto.get();
    }

    std::string value() const
    {
        return m_proto->name() + dump_list(m_proto->argv())
               + " = "
               + type_repr(m_body->type()) + " " + m_body->value();
    }
};

static std::map<std::string, std::unique_ptr<Function>> FunctionTable;
static std::map<std::string, std::unique_ptr<Expression>> VariableTable;
static std::map<std::string, const Expression *> ArgTable;

// NumberExpr - Expression class for numeric literals like "1.0".
class NumberExpr : public Expression {
private:
    double m_value;

public:
    NumberExpr(double value)
    : m_value{value}
    {}

    ExpressionType type() const override
    {
        return ExpressionType::NumberExpr;
    }

    std::string name() const override
    {
        return "<unnamed>";
    }

    std::string value() const override
    {
        return std::to_string(m_value);
    }
};

// VariableExpr - class to store a variable expression, like "a = b + 1".
class VariableExpr : public Expression {
private:
    std::string m_name;
    std::unique_ptr<Expression> m_rhs;

public:
    VariableExpr(const std::string &name, std::unique_ptr<Expression> rhs)
    : m_name{std::move(name)}
    , m_rhs{std::move(rhs)}
    {}

    ExpressionType type() const override
    {
        return ExpressionType::VariableExpr;
    }

    std::string name() const override
    {
        return m_name;
    }

    std::string value() const override
    {
        return " [" + name() + " = " + m_rhs->value() + "] ";
    }

    Expression *expr() const
    {
        return m_rhs.get();
    }
};

// VariableRef - Expression class for referencing a variable, like "a".
class VariableRef : public Expression {
private:
    std::string m_name;

public:
    VariableRef(const std::string &name)
    : m_name{std::move(name)}
    {}

    ExpressionType type() const override
    {
        return ExpressionType::VariableRef;
    }

    std::string name() const override
    {
        return m_name;
    }

    std::string value() const override
    {
        bool have_var = false;
        const auto v = VariableTable.find(m_name);
        if (v != VariableTable.cend()) {
            have_var = true;
        }

        bool have_arg = false;
        const auto a = ArgTable.find(m_name);
        if (a != ArgTable.cend()) {
            have_arg = true;
        }

        const Expression *expr = have_arg ? a->second
                                          : (have_var ? v->second.get() : nullptr);

        if (!expr) {
            return "undefined variable " + m_name;
        }

        return expr->value();
    }
};

// BinaryExpr - Expression class for a binary operator.
class BinaryExpr : public Expression {
private:
    char m_op;
    std::unique_ptr<Expression> m_lhs;
    std::unique_ptr<Expression> m_rhs;

public:
    BinaryExpr(char op,
               std::unique_ptr<Expression> lhs,
               std::unique_ptr<Expression> rhs)
      : m_op{op}
      , m_lhs{std::move(lhs)}
      , m_rhs{std::move(rhs)}
    {}

    ExpressionType type() const override
    {
        return ExpressionType::BinaryExpr;
    }

    std::string name() const override
    {
        return "<unnamed>";
    }

    std::string value() const override
    {
        return " (" + m_lhs->value() + " " + m_op + " " + m_rhs->value() + " )";
    }
};

// CallExpr - Expression class for function calls.
//
// Quick test:
//
//    echo 'def f(x, y) y + x + 1; f(43); f(1,3); f(4.55, 5.667)' | ./test_expr
//
class CallExpr : public Expression {
private:
    std::string m_callee;
    std::vector<std::unique_ptr<Expression>> m_argv;
    std::vector<std::string> m_argv_list;

public:
    CallExpr(const std::string &callee,
             std::vector<std::unique_ptr<Expression>> argv)
    : m_callee{std::move(callee)}
    , m_argv{std::move(argv)}
    {
        for (unsigned i = 0; i < m_argv.size(); ++i) {
            // Improvement: argument type
            m_argv_list.push_back("d");
        }
    }

    ExpressionType type() const override
    {
        return ExpressionType::CallExpr;
    }

    std::string name() const override
    {
        return func_signature(m_callee, m_argv_list);
    }

    std::string value() const override
    {
        const auto f = AST::FunctionTable.find(name());
        if (f == AST::FunctionTable.cend()) {
            //printf("%s func '%s' not found\n", __func__, name().c_str());
            return "func " + name() + " not found";
        }

        const AST::Function *func = f->second.get();
        const AST::Prototype *proto = func->proto();
        const std::vector<std::string> &argv = proto->argv();
        //printf("%s func '%s', proto %p, m_argv.size() %lu\n", __func__,
        //       func->name().c_str(), proto, m_argv.size());

        if (m_argv.size() != argv.size()) {
            return "func " + name() + " argv mismatch";
        }

        // Store the function argument values in the ArgTable
        ArgTable.clear();
        auto arg_name = argv.cbegin();
        auto arg_expr = m_argv.cbegin();
        for (; arg_name != argv.cend() &&
               arg_expr != m_argv.cend(); ++arg_name, ++arg_expr) {
            //printf("%s arg_name '%s'\n", __func__, arg_name->c_str());
            ArgTable[*arg_name] = (*arg_expr).get();
        }

        // + func->name() + " "
        return "call " + func->value();
    }
};

} // end AST namespace

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
// token the parser is looking at.  getNextToken reads another token from the
// lexer and updates CurTok with its results.
static int CurTok;

static int getNextToken()
{
    CurTok = gettok();

    return CurTok;
}

// BinopPrecedence - This holds the precedence for each binary operator that is
// defined.
static std::map<char, int> BinopPrecedence;

// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence(int CurTok)
{
    if (!isascii(CurTok)) {
        return -1;
    }

    // Make sure it's a declared binop.
    const auto i = BinopPrecedence.find(CurTok);
    if (i == BinopPrecedence.cend()) {
        return -1;
    }

    int TokPrec = i->second;

    return TokPrec;
}

// LogError* - These are little helper functions for error handling.
std::unique_ptr<AST::Expression> LogError(const char *str)
{
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

std::unique_ptr<AST::Prototype> LogErrorP(const char *str)
{
    LogError(str);
    return nullptr;
}

static std::unique_ptr<AST::Expression> ParseExpression();

// numberexpr ::= number
static std::unique_ptr<AST::Expression> ParseNumberExpr()
{
    auto result = std::make_unique<AST::NumberExpr>(NumVal);

    // consume the number
    getNextToken();

    //return std::move(Result);
    // This generates a warning:
    // warning: redundant move in return statement [-Wredundant-move]
    //  226 |     return std::move(Result);
    //      |            ~~~~~~~~~^~~~~~~~
    // test_expr.cc:226:21: note: remove ‘std::move’ call
    return result;
}

// parenexpr ::= '(' expression ')'
static std::unique_ptr<AST::Expression> ParseParenExpr()
{
    // eat '('
    getNextToken();

    auto expr = ParseExpression();
    if (!expr) {
        return nullptr;
    }

    if (CurTok != ')') {
        return LogError("expected ')'");
    }

    // eat ).
    getNextToken();

    return expr;
}

// identifierexpr
//   ::= identifier
//   ::= identifier = expression
//   ::= identifier '(' expression* ')'
static std::unique_ptr<AST::Expression> ParseIdentifierExpr()
{
    std::string id_name = IdentifierStr;

    // eat identifier.
    getNextToken();

    // assignment
    if (CurTok == '=') {
        // eat '='
        getNextToken();

        auto expr = ParseExpression();
        if (!expr) {
            printf("%s cannot parse expression for %s\n", __func__, id_name.c_str());
            return nullptr;
        }

        return std::make_unique<AST::VariableExpr>(std::move(id_name), std::move(expr));
    }

    // Simple variable ref.
    if (CurTok != '(') {
        return std::make_unique<AST::VariableRef>(id_name);
    }

    // Function call.  eat '('
    getNextToken();
    std::vector<std::unique_ptr<AST::Expression>> argv;
    if (CurTok != ')') {
        while (true) {
            auto arg = ParseExpression();
            if (!arg) {
                printf("%s cannot parse function args for %s\n", __func__, id_name.c_str());
                return nullptr;
            }

            argv.push_back(std::move(arg));

            if (CurTok == ')') {
                break;
            }

            if (CurTok != ',') {
                return LogError("Expected ')' or ',' in argument list");
            }

            getNextToken();
        }
    }

    // Eat the ')'.
    getNextToken();

    return std::make_unique<AST::CallExpr>(id_name, std::move(argv));
}

// primary
//   ::= identifierexpr
//   ::= numberexpr
//   ::= parenexpr
static std::unique_ptr<AST::Expression> ParsePrimary()
{
    switch (CurTok) {
    case tok_identifier:
        return ParseIdentifierExpr();

    case tok_number:
        return ParseNumberExpr();

    case '(':
        return ParseParenExpr();

    default:
        return LogError("unknown token when expecting an expression");
    }
}

// binoprhs
//   ::= ('+' primary)*
static std::unique_ptr<AST::Expression>
ParseBinOpRHS(int ExprPrec, std::unique_ptr<AST::Expression> lhs)
{
    // If this is a binop, find its precedence.
    while (true) {
        int TokPrec = GetTokPrecedence(CurTok);

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (TokPrec < ExprPrec) {
            return lhs;
        }

        // Okay, we know this is a binop.
        int BinOp = CurTok;

        // eat binop
        getNextToken();

        // Parse the primary expression after the binary operator.
        auto rhs = ParsePrimary();
        if (!rhs) {
            return nullptr;
        }

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedence(CurTok);
        if (TokPrec < NextPrec) {
            rhs = ParseBinOpRHS(TokPrec + 1, std::move(rhs));
            if (!rhs) {
                return nullptr;
            }
        }

        // Merge lhs/rhs.
        lhs = std::make_unique<AST::BinaryExpr>(BinOp, std::move(lhs), std::move(rhs));
    }
}

// expression
//   ::= primary binoprhs
//
static std::unique_ptr<AST::Expression> ParseExpression()
{
    auto lhs = ParsePrimary();
    if (!lhs) {
        return nullptr;
    }

    return ParseBinOpRHS(0, std::move(lhs));
}

// prototype
//   ::= id '(' id* ')'
static std::unique_ptr<AST::Prototype> ParsePrototype()
{
    if (CurTok != tok_identifier) {
        return LogErrorP("Expected function name in prototype");
    }

    std::string func_name = IdentifierStr;

    // eat func name
    getNextToken();

    if (CurTok != '(') {
        return LogErrorP("Expected '(' in prototype");
    }

    std::vector<std::string> argv;
    getNextToken();
    while (CurTok == tok_identifier) {
        argv.push_back(IdentifierStr);

        // eat identifier
        getNextToken();

        if (CurTok == ',') {
            // eat ','
            getNextToken();
        }
    }

    if (CurTok != ')') {
        return LogErrorP("Expected ')' in prototype");
    }

    // success. eat ')'.
    getNextToken();

    return std::make_unique<AST::Prototype>(func_name, std::move(argv));
}

// definition ::= 'def' prototype expression
static std::unique_ptr<AST::Function> ParseDefinition()
{
    // eat def.
    getNextToken();

    auto proto = ParsePrototype();
    if (!proto) {
        return nullptr;
    }

    auto expr = ParseExpression();
    if (!expr) {
        return nullptr;
    }

    return std::make_unique<AST::Function>(std::move(proto), std::move(expr));
}

// toplevelexpr ::= expression
static std::unique_ptr<AST::Function> ParseTopLevelExpr()
{
    std::unique_ptr<AST::Expression> expr = ParseExpression();
    if (!expr) {
        return nullptr;
    }

    using namespace AST;

    // Make an anonymous proto.
    auto proto = std::make_unique<Prototype>("__anon_expr", std::vector<std::string>());
    return std::make_unique<Function>(std::move(proto), std::move(expr));
}

// external ::= 'extern' prototype
static std::unique_ptr<AST::Prototype> ParseExtern()
{
    // eat extern.
    getNextToken();

    return ParsePrototype();
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void HandleDefinition()
{
    std::unique_ptr<AST::Function> func = ParseDefinition();
    if (!func) {
        // Skip token for error recovery.
        getNextToken();
        return;
    }

    printf("Parsed a function definition '%s' => '%s'\n",
           func->name().c_str(),
           func->value().c_str());

    AST::FunctionTable[func->name()] = std::move(func);
}

static void handle_extern()
{
    if (ParseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
        return;
    }

    // Skip token for error recovery.
    getNextToken();
}

static void handle_identifier()
{
    auto id = ParseIdentifierExpr();
    if (!id) {
        printf("%s cannot parse identifier expr\n", __func__);
        return;
    }

    printf("%s var name '%s', type '%d', value '%s'\n", __func__,
           id->name().c_str(), static_cast<int>(id->type()), id->value().c_str());

    if (id->type() == AST::ExpressionType::VariableExpr) {
        AST::VariableTable[id->name()] = std::move(id);
    }
}

static void HandleTopLevelExpression()
{
    // Evaluate a top-level expression into an anonymous function.
    std::unique_ptr<AST::Function> func = ParseTopLevelExpr();
    if (!func) {
        // Skip token for error recovery.
        getNextToken();
        return;
    }

    printf("Parsed a top-level expr '%s'\n", func->value().c_str());
}

// top ::= definition | external | expression | ';'
static void MainLoop()
{
    printf("ready> ");

    getNextToken();

    while (true) {
        switch (CurTok) {
        case tok_eof:
            return;

        case ';': // ignore top-level semicolons.
            getNextToken();
            break;

        case tok_def:
            HandleDefinition();
            break;

        case tok_extern:
            handle_extern();
            break;

        case tok_identifier:
            handle_identifier();
            break;

        default:
            HandleTopLevelExpression();
            break;
        }
    }
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main()
{
    // Install standard binary operators.
    // 1 is lowest precedence.
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40; // highest.

    // Run the main "interpreter loop" now.
    MainLoop();

    return 0;
}
