#include "lstrep.h"
#include "luabin.h"
#include <sstream>

using namespace luayed;

string luayed::to_string(const TokenKind &tk)
{
    if (tk == TokenKind::And)
        return "and";
    if (tk == TokenKind::BinAnd)
        return "binary and";
    if (tk == TokenKind::BinOr)
        return "binary or";
    if (tk == TokenKind::Break)
        return "break";
    if (tk == TokenKind::Colon)
        return "colon";
    if (tk == TokenKind::ColonColon)
        return "colon colon";
    if (tk == TokenKind::Comma)
        return "comma";
    if (tk == TokenKind::Do)
        return "do";
    if (tk == TokenKind::Dot)
        return "dot";
    if (tk == TokenKind::DotDot)
        return "dot dot";
    if (tk == TokenKind::DotDotDot)
        return "dot dot dot";
    if (tk == TokenKind::Else)
        return "else";
    if (tk == TokenKind::ElseIf)
        return "else if";
    if (tk == TokenKind::End)
        return "end";
    if (tk == TokenKind::Eof)
        return "EOF";
    if (tk == TokenKind::Equal)
        return "equal";
    if (tk == TokenKind::EqualEqual)
        return "equal equal";
    if (tk == TokenKind::Error)
        return "!error";
    if (tk == TokenKind::Empty)
        return "_empty_";
    if (tk == TokenKind::False)
        return "false";
    if (tk == TokenKind::FloatDivision)
        return "float division";
    if (tk == TokenKind::FloorDivision)
        return "floor division";
    if (tk == TokenKind::For)
        return "for";
    if (tk == TokenKind::Function)
        return "function";
    if (tk == TokenKind::Goto)
        return "goto";
    if (tk == TokenKind::Greater)
        return "greater";
    if (tk == TokenKind::GreaterEqual)
        return "greater equal";
    if (tk == TokenKind::Identifier)
        return "identifier";
    if (tk == TokenKind::If)
        return "if";
    if (tk == TokenKind::In)
        return "in";
    if (tk == TokenKind::LeftBrace)
        return "left brace";
    if (tk == TokenKind::LeftBracket)
        return "left bracket";
    if (tk == TokenKind::LeftParen)
        return "left parentheses";
    if (tk == TokenKind::LeftShift)
        return "left shift";
    if (tk == TokenKind::Length)
        return "length";
    if (tk == TokenKind::Less)
        return "less";
    if (tk == TokenKind::LessEqual)
        return "less equal";
    if (tk == TokenKind::Literal)
        return "literal";
    if (tk == TokenKind::Local)
        return "local";
    if (tk == TokenKind::Minus)
        return "minus";
    if (tk == TokenKind::Modulo)
        return "modulo";
    if (tk == TokenKind::Multiply)
        return "multiply";
    if (tk == TokenKind::Negate)
        return "negate";
    if (tk == TokenKind::Nil)
        return "nil";
    if (tk == TokenKind::None)
        return "-NONE-";
    if (tk == TokenKind::Not)
        return "not";
    if (tk == TokenKind::NotEqual)
        return "not equal";
    if (tk == TokenKind::Number)
        return "number";
    if (tk == TokenKind::Or)
        return "or";
    if (tk == TokenKind::Plus)
        return "plus";
    if (tk == TokenKind::Power)
        return "power";
    if (tk == TokenKind::Repeat)
        return "repeat";
    if (tk == TokenKind::Return)
        return "return";
    if (tk == TokenKind::RightBrace)
        return "right brace";
    if (tk == TokenKind::RightBracket)
        return "right bracket";
    if (tk == TokenKind::RightParen)
        return "right paren";
    if (tk == TokenKind::RightShift)
        return "right shift";
    if (tk == TokenKind::Semicolon)
        return "semicolon";
    if (tk == TokenKind::Then)
        return "then";
    if (tk == TokenKind::True)
        return "true";
    if (tk == TokenKind::Until)
        return "until";
    return "while";
}
string luayed::to_string(const Token &t, const char *source)
{
    string s;
    s += t.text(source).c_str();
    while (s.size() < 16)
        s.push_back(' ');
    s += " (";
    s += luayed::to_string(t.kind);
    s += ") [";
    s += std::to_string(t.line + 1);
    s += ",";
    s += std::to_string(t.offset + 1);
    s += "]";
    return s;
}

const char *node_names[34] = {
    "Primary",
    "Binary",
    "Unary",
    "IdField",
    "ExprField",
    "Table",
    "Property",
    "Index",
    "Call",
    "Explist",
    "CallStmt",
    "AssignStmt",
    "LabelStmt",
    "BreakStmt",
    "GotoStmt",
    "WhileStmt",
    "RepeatStmt",
    "IfStmt",
    "NumericFor",
    "GenericFor",
    "ReturnStmt",
    "FunctionBody",
    "Declaration",
    "MethodCall",
    "Block",
    "Name",
    "NameList",
    "IfClause",
    "ElseClause",
    "ElseIfClause",
    "VarDecl",
    "VarList",
    "MethodBody",
    "Operator",
};

string luayed::to_string(const ast::NodeKind &nk)
{
    return node_names[nk];
}

string node_to_string_at_depth(string text, int depth)
{
    string t = string(depth, '-');
    t += "> ";
    t += text;
    t += "\n";
    return t;
}

void node_to_string(ast::Noderef node, int depth, string &buffer, const char *source)
{
    buffer += node_to_string_at_depth(node_names[node->get_kind()], depth);
    if (node->get_token().kind != TokenKind::None)
    {
        buffer += node_to_string_at_depth(node->get_token().text(source), depth + 3);
    }
    if (node->child_count())
    {
        foreach_node(node, ch)
            node_to_string(ch, depth + 3, buffer, source);
    }
}

string luayed::to_string(const ast::Noderef &n, const char *source)
{
    string buffer;
    node_to_string(n, 1, buffer, source);
    return buffer;
}
string luayed::to_string(const Opcode &opcode)
{
    const char *opnames[256];
    opnames[IAdd] = "add";
    opnames[ISub] = "sub";
    opnames[IMult] = "mult";
    opnames[IFlrDiv] = "flrdiv";
    opnames[IFltDiv] = "fltdiv";
    opnames[IMod] = "mod";
    opnames[IPow] = "pow";
    opnames[IConcat] = "concat";
    opnames[IBOr] = "or";
    opnames[IBAnd] = "and";
    opnames[IBXor] = "xor";
    opnames[ISHR] = "shl";
    opnames[ISHL] = "shr";
    opnames[ILength] = "length";
    opnames[INegate] = "negate";
    opnames[INot] = "not";
    opnames[IBNot] = "bnot";
    opnames[IEq] = "eq";
    opnames[INe] = "ne";
    opnames[IGe] = "ge";
    opnames[IGt] = "gt";
    opnames[ILe] = "le";
    opnames[ILt] = "lt";
    opnames[ITGet] = "tget";
    opnames[ITSet] = "tset";
    opnames[ITNew] = "tnew";
    opnames[IGGet] = "gget";
    opnames[IGSet] = "gset";
    opnames[INil] = "nil";
    opnames[ITrue] = "true";
    opnames[IFalse] = "false";
    opnames[IUPush] = "upush";
    opnames[IUPop] = "upop";
    opnames[IRet] = "ret";
    opnames[IJmp] = "jmp";
    opnames[ICjmp] = "cjmp";
    opnames[ICall] = "call";
    opnames[IVargs] = "vargs";
    opnames[ITList] = "tlist";
    opnames[ICall] = "call";
    opnames[ITCall] = "tcall";
    opnames[IConst] = "C";
    opnames[IFConst] = "FC";
    opnames[ILocal] = "local";
    opnames[ILStore] = "lstore";
    opnames[IBLocal] = "blocal";
    opnames[IBLStore] = "blstore";
    opnames[IUpvalue] = "upvalue";
    opnames[IUStore] = "ustore";
    opnames[IPop] = "pop";
    return opnames[opcode];
}
string luayed::to_string(const vector<lbyte> &bin)
{
    return luayed::to_string(&bin.front(), bin.size());
}
string luayed::to_string(const lbyte *text, size_t codelen)
{
    string str;
    for (size_t i = 0; i < codelen;)
    {
        size_t rc;
        Instruction ins = Instruction::decode(text + i, &rc);
        i += rc;
        str.append(luayed::to_string(ins.op));
        if (ins.oprnd_count() > 0)
        {
            str.push_back(' ');
            str += std::to_string(ins.oprnd1);
        }
        if (ins.oprnd_count() > 1)
        {
            str.push_back(' ');
            str += std::to_string(ins.oprnd2);
        }
        str.push_back('\n');
    }
    return str;
}
string luayed::to_string(const LuaValue &lv)
{
    string s = luayed::to_string(lv.kind);
    if (lv.kind == LuaType::LVBool)
    {
        s += "(";
        s += (lv.data.b ? "true" : "false");
        s += ")";
    }
    else if (lv.kind == LuaType::LVNumber)
    {
        s += "(";
        s += std::to_string(lv.data.n);
        s += ")";
    }
    else if (lv.kind == LuaType::LVString)
    {
        s += "(";
        s += (const char *)lv.data.ptr;
        s += ")";
    }
    return s;
}

string luayed::to_string(lnumber n)
{
    string str = std::to_string(n);
    str.erase(str.find_last_not_of('0') + 1);
    str.erase(str.find_last_not_of('.') + 1);
    return str;
}

string luayed::to_string(const LuaType &lt)
{
    if (lt == LuaType::LVNil)
        return "nil";
    else if (lt == LuaType::LVBool)
        return "boolean";
    if (lt == LuaType::LVNumber)
        return "number";
    if (lt == LuaType::LVTable)
        return "table";
    if (lt == LuaType::LVString)
        return "string";
    if (lt == LuaType::LVFunction)
        return "function";
    return "";
}

string luayed::to_string(const Lerror &err)
{
    return luayed::to_string(err, false);
}

string luayed::to_string(const Lerror &err, bool pure)
{
    std::stringstream os;
    if (err.kind == Lerror::LE_OK)
        return os.str();
    if (!pure)
    {
        os << "lua: error(line: "
           << err.line + 1
           << ", offset: "
           << err.offset + 1
           << "): ";
    }
    if (err.kind == Lerror::LE_MissingEndOfComment)
    {
        os << "missing symbol ']";
        for (size_t i = 0; i < err.as.missing_end_of_comment.level; i++)
            os << '=';
        os << "]'";
    }
    else if (err.kind == Lerror::LE_MisingEndOfString)
    {
        os << "missing symbol ']";
        for (size_t i = 0; i < err.as.missing_end_of_string.level; i++)
            os << '=';
        os << "]'";
    }
    else if (err.kind == Lerror::LE_MissingChar)
    {
        os << "missing character '" << err.as.missing_char.c << "'";
    }
    else if (err.kind == Lerror::LE_InvalidChar)
    {
        os << "invalid character '" << err.as.invalid_char.c << "'";
    }
    else if (err.kind == Lerror::LE_InvalidEscape)
    {
        os << "invalid escape";
    }
    else if (err.kind == Lerror::LE_MalformedNumber)
    {
        os << "malformed number";
    }
    else if (err.kind == Lerror::LE_ExpectedToken)
    {
        os << "expected token '" << luayed::to_string(err.as.expected_token.token_kind) << "'";
    }
    else if (err.kind == Lerror::LE_ExpectedVariable)
    {
        os << "expected variable";
    }
    else if (err.kind == Lerror::LE_ExpectedExpression)
    {
        os << "expected expression";
    }
    else if (err.kind == Lerror::LE_InvalidOperand)
    {
        LuaType t = err.as.invalid_operand.t;
        os << "invalid operation on type [" << t << "]";
    }
    else if (err.kind == Lerror::LE_InvalidComparison)
    {
        LuaType t1 = err.as.invalid_comparison.t1;
        LuaType t2 = err.as.invalid_comparison.t2;
        os << "attemp to compare " << t1 << " with " << t2;
    }
    else if (err.kind == Lerror::LE_VargsOutsideFunction)
    {
        os << "can not use '...' outside a vararg function";
    }
    else if (err.kind == Lerror::LE_BreakOutsideLoop)
    {
        os << "break not inside a loop";
    }
    else if (err.kind == Lerror::LE_LabelUndefined)
    {
        os << "label not visible";
    }
    else if (err.kind == Lerror::LE_NotEnoughArgs)
    {
        size_t available = err.as.not_enough_args.available;
        size_t expected = err.as.not_enough_args.expected;
        os << "expected " << expected << " values, while there are " << available << "on the stack";
    }
    else if (err.kind == Lerror::LE_CallNonFunction)
    {
        LuaType t = err.as.call_non_function.t;
        os << "attemp to call a " << t << " value";
    }
    else if (err.kind == Lerror::LE_LabelRedefined)
    {
        size_t line = err.as.label_redefined.line;
        size_t offset = err.as.label_redefined.offset;
        os << "label previously defined at (line: " << line << ", offset: " << offset << ")";
    }
    else if (err.kind == Lerror::LE_NilIndex)
    {
        os << "table index is nil";
    }
    else if (err.kind == Lerror::LE_IllegalIndex)
    {
        LuaType t = err.as.illegal_index.t;
        os << "attemp to index a " << t << " value";
    }
    else if (err.kind == Lerror::LE_IntegerRepresentation)
    {
        os << "number has no integer representation";
    }
    else
    {
        os << "FAULT: THIS ERROR CAN'T BE DISPLAYED";
    }
    return os.str();
}

string luayed::to_string(const vector<LuaValue> &vv)
{
    string text;
    for (size_t i = 0; i < vv.size(); i++)
    {
        text.append(luayed::to_string(vv[i]));
        text.push_back('\n');
    }
    return text;
}

#define WRITE_TO_STREAM_OPERATOR(TYPE)                        \
    std::ostream &operator<<(std::ostream &os, const TYPE &o) \
    {                                                         \
        os << luayed::to_string(o);                           \
        return os;                                            \
    }

WRITE_TO_STREAM_OPERATOR(vector<lbyte>)
WRITE_TO_STREAM_OPERATOR(ast::NodeKind)
WRITE_TO_STREAM_OPERATOR(TokenKind)
WRITE_TO_STREAM_OPERATOR(LuaType)
WRITE_TO_STREAM_OPERATOR(LuaValue)
WRITE_TO_STREAM_OPERATOR(Lerror)
WRITE_TO_STREAM_OPERATOR(vector<LuaValue>)
WRITE_TO_STREAM_OPERATOR(Opcode)