#include "compiler.hpp"

#define EXPECT_FREE 0xffff

char *token_cstring(Token t)
{
    char *str = (char *)malloc(t.len + 1);
    for (size_t i = 0; i < t.len; i++)
        str[i] = t.str[i];
    str[t.len] = '\0';
    return str;
}

char *token_lstring(Token t)
{
    char *str = (char *)malloc(t.len + 1);
    for (size_t i = 0; i < t.len; i++)
        str[i] = t.str[i];
    str[t.len] = '\0';
    return str;
}

lnumber token_number(Token t)
{
    return 0;
}

void Lfunction::push(lbyte b)
{
    this->text.push_back(b);
}
lbyte Lfunction::opcode(size_t index)
{
    return this->text[index];
}
size_t Lfunction::clen()
{
    return this->text.size();
}
size_t Lfunction::number(lnumber n)
{
    size_t i = this->nconst.size();
    this->nconst.push_back(n);
    return i;
}
size_t Lfunction::cstr(const char *s)
{
    size_t i = this->sconst.size();
    this->sconst.push_back(s);
    return i;
}
Lfunction::~Lfunction()
{
    for (size_t i = 0; i < this->sconst.size(); i++)
        free((void *)this->sconst[i]);
}

vector<Lfunction> Compiler::compile(Ast ast)
{
    this->newf();
    this->compile_node(ast.root());
    this->endf();
    return std::move(this->funcs);
}

lbyte tkn_binops[] = {
    IAdd,
    IMult,
    IFltDiv,
    IFlrDiv,
    IMod,
    IPow,
    IBAnd,
    IBOr,
    ISHR,
    ISHL,
    IConcat,
    ILt,
    ILe,
    IGt,
    IGe,
    IEq,
    INe};

lbyte Compiler::translate_token(TokenKind kind, bool bin)
{
    if (TOKEN_IS_BINARY(kind))
    {
        if (TOKEN_IS_PREFIX(kind))
        {
            return kind == TokenKind::Minus ? (bin ? Instruction::ISub : Instruction::INegate)
                                            : (bin ? Instruction::IBXor : Instruction::IBNot);
        }
        else
        {
            return tkn_binops[kind - 0x0400];
        }
    }
    else // prefix
    {
        return kind == TokenKind::Not ? Instruction::INot : Instruction::ILength;
    }
}
void Compiler::compile_methcall(Noderef node, size_t expect)
{
    this->compile_exp(node->child(0));
    Noderef arglist = node->child(2);
    this->compile_explist(arglist, EXPECT_FREE);
    this->emit(Opcode(Instruction::IBLocal, arglist->child_count() + 1));

    size_t idx = this->const_string(token_cstring(node->child(1)->get_token()));
    this->emit(Opcode(Instruction::ISConst, idx));
    this->emit(Opcode(Instruction::ITGet));
    if (expect == EXPECT_FREE)
        this->emit(Opcode(Instruction::IFCall));
    else
        this->emit(Opcode(Instruction::ICall, expect));
}
void Compiler::compile_call(Noderef node, size_t expect)
{
    Noderef arglist = node->child(1);
    this->compile_explist(arglist, EXPECT_FREE);
    this->compile_exp(node->child(0));
    if (expect == EXPECT_FREE)
        this->emit(Opcode(Instruction::IFCall));
    else
        this->emit(Opcode(Instruction::ICall, expect));
}
void Compiler::compile_identifier(Noderef node)
{
    MetaDeclaration *md = (MetaDeclaration *)node->getannot(MetaKind::MDecl);
    if (!md)
    {
        size_t idx = this->const_string(token_cstring(node->get_token()));
        this->emit(Opcode(Instruction::ISConst, idx));
        this->emit(Instruction::IGGet);
    }
    else
    {
        Noderef decl = md->decnode;
        MetaMemory *dmd = (MetaMemory *)decl->getannot(MetaKind::MMemory);
        if (dmd->is_stack)
        {
            this->emit(Opcode(Instruction::ILocal, dmd->offset));
        }
        else
        {
            // upval
        }
    }
}

void Compiler::compile_primary(Noderef node)
{
    Token tkn = node->get_token();
    if (tkn.kind == TokenKind::True)
        this->emit(Instruction::ITrue);
    else if (tkn.kind == TokenKind::False)
        this->emit(Instruction::IFalse);
    else if (tkn.kind == TokenKind::Nil)
        this->emit(Instruction::INil);
    else if (tkn.kind == TokenKind::Number)
    {
        size_t idx = this->const_number(token_number(tkn));
        this->emit(Opcode(Instruction::INConst, idx));
    }
    else if (tkn.kind == TokenKind::Literal)
    {
        size_t idx = this->const_string(token_lstring(tkn));
        this->emit(Opcode(Instruction::ISConst, idx));
    }
    else if (tkn.kind == TokenKind::Identifier)
    {
        this->compile_identifier(node);
    }
}
void Compiler::compile_exp_e(Noderef node, size_t expect)
{
    if (!expect)
        return;
    if (node->get_kind() == NodeKind::Binary)
    {
        this->compile_exp(node->child(0));
        this->compile_exp(node->child(2));
        this->emit(this->translate_token(node->child(1)->get_token().kind, true));
    }
    else if (node->get_kind() == NodeKind::Unary)
    {
        this->compile_exp(node->child(1));
        this->emit(this->translate_token(node->child(0)->get_token().kind, false));
    }
    else if (node->get_kind() == NodeKind::Property)
    {
        this->compile_exp(node->child(0));
        size_t idx = this->const_string(token_cstring(node->child(1)->get_token()));
        this->emit(Opcode(ISConst, idx));
        this->emit(ITGet);
    }
    else if (node->get_kind() == NodeKind::Index)
    {
        this->compile_exp(node->child(0));
        this->compile_exp(node->child(1));
        this->emit(ITGet);
    }
    else if (node->get_kind() == NodeKind::Primary)
    {
        this->compile_primary(node);
    }
    else if (node->get_kind() == NodeKind::Call)
    {
        this->compile_call(node, expect);
    }
    else if (node->get_kind() == NodeKind::MethodCall)
    {
        this->compile_methcall(node, expect);
    }
    if (node->get_kind() != NodeKind::Call && expect != EXPECT_FREE)
        while (--expect)
            this->emit(Opcode(Instruction::INil));
}

void Compiler::compile_exp(Noderef node)
{
    this->compile_exp_e(node, 1);
}

MetaMemory *Compiler::varmem(Noderef lvalue)
{
    MetaDeclaration *md = (MetaDeclaration *)lvalue->getannot(MetaKind::MDecl);
    Noderef dec = md ? md->decnode : lvalue;
    return (MetaMemory *)dec->getannot(MetaKind::MMemory);
}

void Compiler::compile_lvalue_primary(Noderef node)
{
    MetaMemory *md = this->varmem(node);
    if (!md)
    {
        const char *str = token_cstring(node->get_token());
        size_t idx = this->const_string(str);
        this->emit(Opcode(Instruction::ISConst, idx));
        this->ops_push(Instruction::IGGet);
        this->vstack.push_back(1);
    }
    else
    {
        if (md->is_stack)
        {
            this->ops_push(Opcode(Instruction::ILStore, md->offset));
        }
        else
        {
            // upval
        }
    }
}

void Compiler::compile_lvalue(Noderef node)
{
    if (node->get_kind() == NodeKind::Primary || node->get_kind() == NodeKind::Name)
    {
        this->compile_lvalue_primary(node);
        return;
    }
    if (node->get_kind() == NodeKind::Property)
    {
        Noderef lexp = node->child(0);
        Noderef prop = node->child(1);
        this->compile_exp(lexp);
        const char *prop_str = token_cstring(prop->get_token());
        size_t idx = this->const_string(prop_str);
        this->emit(Opcode(Instruction::ISConst, idx));
        this->ops_push(Opcode(Instruction::ITSet));
        this->vstack.push_back(1);
        this->vstack.push_back(1);
    }
    if (node->get_kind() == NodeKind::Index)
    {
        Noderef lexp = node->child(0);
        Noderef iexp = node->child(1);
        this->compile_exp(lexp);
        this->compile_exp(iexp);
        this->ops_push(Opcode(Instruction::ITSet));
        this->vstack.push_back(1);
        this->vstack.push_back(1);
    }
}

void Compiler::compile_explist(Noderef node, size_t vcount)
{
    if (vcount == EXPECT_FREE)
    {
        if (!node || node->child_count() == 0)
            return;
        for (size_t i = 0; i < node->child_count() - 1; i++)
        {
            this->compile_exp(node->child(i));
        }
        this->compile_exp_e(node->child(node->child_count() - 1), vcount);
    }
    else
    {
        if (!node || node->child_count() == 0)
        {
            while (vcount--)
                this->emit(Opcode(Instruction::INil));
        }
        else
        {
            for (size_t i = 0; i < node->child_count() - 1; i++)
            {
                this->compile_exp_e(node->child(i), vcount ? 1 : 0);
                if (vcount)
                    vcount--;
            }
            this->compile_exp_e(node->child(node->child_count() - 1), vcount);
        }
    }
}

void Compiler::compile_varlist(Noderef node, bool attrib)
{
    if (node->child_count() == 1)
    {
        this->compile_lvalue(attrib ? node->child(0)->child(0) : node->child(0));
    }
    else
    {
        for (size_t i = 0; i < node->child_count(); i++)
        {
            this->compile_lvalue(attrib ? node->child(i)->child(0) : node->child(i));
            this->emit(Opcode(Instruction::INil));
            this->vstack.push_back(0);
        }
    }
}

size_t Compiler::vstack_nearest_nil()
{
    size_t i = this->vstack.size() - 1;
    while (this->vstack[i])
        i--;
    this->vstack[i] = 1;
    return this->vstack.size() - 1 - i;
}

void Compiler::compile_assignment(Noderef node, bool attrib)
{
    size_t vcount = node->child(0)->child_count();
    this->compile_varlist(node->child(0), attrib);
    this->compile_explist(node->child(1), vcount);

    if (vcount > 1)
    {
        size_t v = vcount;
        while (v)
        {
            this->emit(Opcode(Instruction::IBLStore, v + this->vstack_nearest_nil() + 1));
            v--;
        }
    }
    this->vstack.clear();
    this->ops_flush();
}

void Compiler::compile_block(Noderef node)
{
    MetaScope *md = (MetaScope *)node->getannot(MetaKind::MScope);
    if (md->size)
        this->emit(Opcode(Instruction::IPush, md->size));
    for (size_t i = 0; i < node->child_count(); i++)
        this->compile_node(node->child(i));
    if (md->size)
        this->emit(Opcode(Instruction::IPop, md->size));
}

void Compiler::compile_decl(Noderef node)
{
    this->compile_assignment(node, true);
}

void Compiler::compile_node(Noderef node)
{
    if (node->get_kind() == NodeKind::AssignStmt)
        this->compile_assignment(node, false);
    else if (node->get_kind() == NodeKind::Block)
        this->compile_block(node);
    else if (node->get_kind() == NodeKind::Declaration)
        this->compile_decl(node);
    else
        exit(4);
}

Lfunction &Compiler::cur()
{
    return this->current.back();
}

void Compiler::newf()
{
    this->current.push_back(std::move(Lfunction()));
}

void Compiler::endf()
{
    this->funcs.push_back(std::move(this->cur()));
    this->current.pop_back();
}

size_t Compiler::const_number(lnumber n)
{
    return this->cur().number(n);
}

size_t Compiler::const_string(const char *s)
{
    return this->cur().cstr(s);
}

void Compiler::emit(Opcode op)
{
    for (int i = 0; i < op.count; i++)
        this->cur().push(op.bytes[i]);
}

void Compiler::ops_flush()
{
    while (this->ops.size())
    {
        this->emit(this->ops.back());
        this->ops.pop_back();
    }
}

void Compiler::ops_push(Opcode op)
{
    this->ops.push_back(op);
}

Opcode::Opcode(lbyte op, size_t idx)
{
    if (idx >= 256)
    {
        this->count = 3;
        this->bytes[0] = op | 0x1;
        this->bytes[1] = idx % 256;
        this->bytes[2] = idx >> 8;
    }
    else
    {
        this->count = 2;
        this->bytes[0] = op;
        this->bytes[1] = idx;
    }
}
Opcode::Opcode(lbyte op)
{
    this->bytes[0] = op;
    this->count = 1;
}

string Lfunction::stringify()
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
    opnames[ICall] = "call";
    opnames[IFCall] = "fcall";
    opnames[IJmp] = "jmp";
    opnames[ICjmp] = "cjmp";
    opnames[Iret] = "ret";

    opnames[INConst] = opnames[INConst + 1] = "NC";
    opnames[ISConst] = opnames[ISConst + 1] = "SC";
    opnames[IFConst] = opnames[IFConst + 1] = "FC";
    opnames[ILocal] = opnames[ILocal + 1] = "local";
    opnames[ILStore] = opnames[ILStore + 1] = "lstore";
    opnames[IBLocal] = opnames[IBLocal + 1] = "blocal";
    opnames[IBLStore] = opnames[IBLStore + 1] = "blstore";
    opnames[IUpvalue] = opnames[IUpvalue + 1] = "upvalue";
    opnames[IUStore] = opnames[IUStore + 1] = "ustore";
    opnames[IPush] = opnames[IPush + 1] = "push";
    opnames[IPop] = opnames[IPop + 1] = "pop";

    string text;
    for (size_t i = 0; i < this->clen(); i++)
    {
        lbyte op = this->opcode(i);
        text.append(opnames[op]);
        if (op >> 7)
        {
            int opr = 0;
            int ocount = 1 + op & 0x01;
            while (ocount--)
            {
                opr <<= 8;
                opr += this->opcode(++i);
            }
            text.push_back(' ');
            text += std::to_string(opr);
        }
        text.push_back('\n');
    }

    return text;
}