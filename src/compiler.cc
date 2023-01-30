#include "compiler.h"

#define EXPECT_FREE 0xffff

int chex(char c)
{
    return c > 'a' ? (c - 'a' + 10) : (c - '0');
}
int cdec(char c)
{
    return (c <= '9' && c >= '0') ? (c - '0') : -1;
}

string scan_lua_multiline_string(Token t)
{
    string str;
    size_t level = 0;
    const char *ptr = t.str + 1;
    while (*ptr == '=')
    {
        ptr++;
        level++;
    }
    ptr++;
    if (*ptr == '\n')
        ptr++;
    for (; ptr != t.str + t.len - 2 - level; ptr++)
        str.push_back(*ptr);
    return str;
}

string scan_lua_singleline_string(Token t)
{
    string text = t.text();
    string str = "";
    bool escape = false;
    for (size_t i = 1; i < text.size() - 1; i++)
    {
        char c = text[i];
        if (escape)
        {
            escape = false;
            if (c == 'a')
                str.push_back('\a');
            else if (c == 'b')
                str.push_back('\b');
            else if (c == 'f')
                str.push_back('\f');
            else if (c == 'r')
                str.push_back('\r');
            else if (c == 't')
                str.push_back('\t');
            else if (c == 'n')
                str.push_back('\n');
            else if (c == '\n')
                str.push_back('\n');
            else if (c == '\v')
                str.push_back('\v');
            else if (c == '\\')
                str.push_back('\\');
            else if (c == '"')
                str.push_back('"');
            else if (c == '\'')
                str.push_back('\'');
            else if (c == 'x')
            {
                char c = chex(text[++i]) * 16;
                c += chex(text[++i]);
                str.push_back(c);
            }
            else
            {
                char v = 0;
                while (cdec(c) != -1)
                {
                    v *= 10;
                    v += c;
                    c = text[++i];
                }
                str.push_back(v);
            }
        }
        else
        {
            if (c == '\\')
                escape = true;
            else
                str.push_back(c);
        }
    }
    return str;
}

string scan_lua_string(Token t)
{
    if (*t.str == '[')
        return scan_lua_multiline_string(t);
    else
        return scan_lua_singleline_string(t);
}

lnumber token_number(Token t)
{
    char *str = new char[t.len + 1];
    for (size_t i = 0; i < t.len; i++)
        str[i] = t.str[i];
    str[t.len] = '\0';
    lnumber num = atof(str);
    delete[] str;
    return num;
}

fidx_t Compiler::compile(Noderef root, const char *chunckname)
{
    this->chunckname = chunckname;
    MetaScope *fnscp = (MetaScope *)root->getannot(MetaKind::MScope);
    fnscp->fidx = this->gen->pushf();
    if (root->get_kind() == NodeKind::Block)
    {
        this->compile_node(root);
        this->gen->meta_parcount(0);
    }
    else
    {
        Noderef params = root->child(0);
        size_t parcount = root->get_kind() == NodeKind::MethodBody;
        size_t upcount = 0;
        for (size_t i = 0; i < params->child_count(); i++)
        {
            Noderef par = params->child(i)->child(0);
            if (par->get_token().kind == TokenKind::DotDotDot)
                continue;
            parcount++;
            MetaMemory *md = (MetaMemory *)par->getannot(MetaKind::MMemory);
            md->offset = this->stack_offset++;
            if (md->is_upvalue)
            {
                md->upoffset = this->hooksize;
                this->emit(Opcode::IUPush);
                this->hookpush();
                upcount++;
            }
        }
        this->compile_node(root->child(1));
        while (upcount--)
            this->emit(Opcode::IUPop);

        this->gen->meta_parcount(parcount);
    }
    this->gen->meta_hookmax(this->hookmax);
    this->gen->meta_chunkname(chunckname);
    this->emit(Bytecode(Opcode::IRet, 0));
    this->gen->popf();
    return fnscp->fidx;
}

fidx_t Compiler::compile(Ast ast, const char *chunckname)
{
    return this->compile(ast.root(), chunckname);
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
            return kind == TokenKind::Minus ? (bin ? Opcode::ISub : Opcode::INegate)
                                            : (bin ? Opcode::IBXor : Opcode::IBNot);
        }
        else
        {
            return tkn_binops[kind - 0x0400];
        }
    }
    else // prefix
    {
        return kind == TokenKind::Not ? Opcode::INot : Opcode::ILength;
    }
}

size_t Compiler::arglist_count(Noderef arglist)
{
    size_t chlen = arglist->child_count();
    if (!chlen)
        return 0;
    Noderef last = arglist->child(chlen - 1);
    if (last->get_kind() == NodeKind::MethodCall ||
        last->get_kind() == NodeKind::Call ||
        (last->get_kind() == NodeKind::Primary && last->get_token().kind == TokenKind::DotDotDot))
        chlen--;
    return chlen;
}
size_t Compiler::len()
{
    return this->gen->len();
}

void Compiler::compile_if(Noderef node)
{
    vector<size_t> jmps;
    size_t cjmp = 0;
    for (size_t i = 0; i < node->child_count(); i++)
    {
        Noderef cls = node->child(i);
        if (cls->get_kind() == NodeKind::ElseClause)
        {
            this->compile_block(cls->child(0));
        }
        else
        {
            this->compile_exp(cls->child(0));
            this->emit(Opcode::INot);
            cjmp = this->len();
            this->emit(Bytecode(Opcode::ICjmp, 0));
            this->compile_block(cls->child(1));
            jmps.push_back(this->len());
            this->emit(Bytecode(Opcode::IJmp, 0));
            size_t cjmp_idx = this->len();
            this->edit_jmp(cjmp, cjmp_idx);
        }
    }
    size_t jmp_idx = this->len();
    for (size_t i = 0; i < jmps.size(); i++)
    {
        this->edit_jmp(jmps[i], jmp_idx);
    }
}

void Compiler::edit_jmp(size_t opidx, size_t jmp_idx)
{
    this->seti(opidx + 1, jmp_idx % 256);
    this->seti(opidx + 2, jmp_idx >> 8);
}

void Compiler::compile_methcall(Noderef node, size_t expect)
{
    Noderef object = node->child(0);
    Token fname = node->child(1)->get_token();
    this->emit(Opcode::INil);
    this->compile_exp(object);
    this->emit(Bytecode(Opcode::IBLocal, 1));
    size_t idx = this->const_string(fname.text().c_str());
    this->emit(Bytecode(Opcode::IConst, idx));
    this->emit(Bytecode(Opcode::ITGet));
    this->emit(Bytecode(Opcode::IBLStore, 2));
    Noderef arglist = node->child(2);
    this->compile_explist(arglist, EXPECT_FREE);
    size_t argcount = this->arglist_count(arglist) + 1;
    this->debug_info(fname.line);
    if (expect == EXPECT_FREE)
        this->emit(Bytecode(Opcode::ICall, argcount, 0));
    else
        this->emit(Bytecode(Opcode::ICall, argcount, expect + 1));
}
void Compiler::compile_call(Noderef node, size_t expect)
{
    Noderef fn = node->child(0);
    Noderef arglist = node->child(1);
    this->compile_exp(fn);
    size_t argcount = this->arglist_count(arglist);
    this->compile_explist(arglist, EXPECT_FREE);
    this->debug_info(fn->line());
    if (expect == EXPECT_FREE)
        this->emit(Bytecode(Opcode::ICall, argcount, 0));
    else
        this->emit(Bytecode(Opcode::ICall, argcount, expect + 1));
}
void Compiler::compile_identifier(Noderef node)
{
    MetaDeclaration *md = (MetaDeclaration *)node->getannot(MetaKind::MDecl);
    Noderef dec = md ? md->decnode : node;
    MetaMemory *mm = (MetaMemory *)dec->getannot(MetaKind::MMemory);
    if (md)
    {
        if (md->is_upvalue)
        {
            MetaScope *sc = (MetaScope *)mm->scope->getannot(MetaKind::MScope);
            MetaScope *fnsc = (MetaScope *)sc->func->getannot(MetaKind::MScope);
            this->emit(Bytecode(Opcode::IUpvalue, this->upval(fnsc->fidx, mm->offset, mm->upoffset)));
        }
        else
        {
            this->emit(Bytecode(Opcode::ILocal, mm->offset));
        }
    }
    else if (node->getannot(MetaKind::MSelf))
    {
        this->emit(Bytecode(Opcode::ILocal, 0));
    }
    else
    {
        size_t idx = this->const_string(node->get_token().text().c_str());
        this->emit(Bytecode(Opcode::IConst, idx));
        this->emit(Opcode::IGGet);
    }
}

void Compiler::compile_primary(Noderef node, size_t expect)
{
    Token tkn = node->get_token();
    if (tkn.kind == TokenKind::True)
        this->emit(Opcode::ITrue);
    else if (tkn.kind == TokenKind::DotDotDot)
    {
        this->emit(Bytecode(Opcode::IVargs, expect == EXPECT_FREE ? 0 : expect + 1));
    }
    else if (tkn.kind == TokenKind::False)
        this->emit(Opcode::IFalse);
    else if (tkn.kind == TokenKind::Nil)
        this->emit(Opcode::INil);
    else if (tkn.kind == TokenKind::Number)
    {
        size_t idx = this->const_number(token_number(tkn));
        this->emit(Bytecode(Opcode::IConst, idx));
    }
    else if (tkn.kind == TokenKind::Literal)
    {
        size_t idx = this->const_string(scan_lua_string(tkn).c_str());
        this->emit(Bytecode(Opcode::IConst, idx));
    }
    else if (tkn.kind == TokenKind::Identifier)
    {
        this->compile_identifier(node);
    }
}
void Compiler::compile_name(Noderef node)
{
    Token tkn = node->get_token();
    size_t idx = this->const_string(tkn.text().c_str());
    this->emit(Bytecode(Opcode::IConst, idx));
}

void Compiler::compile_table(Noderef node)
{
    this->emit(Bytecode(Opcode::ITNew));
    size_t list_len = 0;
    if (!node->child_count())
        return;
    for (size_t i = 0; i < node->child_count(); i++)
    {
        Noderef ch = node->child(i);
        if (ch->get_kind() == NodeKind::IdField)
        {
            this->compile_name(ch->child(0));
            this->compile_exp(ch->child(1));
        }
        else if (ch->get_kind() == NodeKind::ExprField)
        {
            this->compile_exp(ch->child(0));
            this->compile_exp(ch->child(1));
        }
        else if (i == node->child_count() - 1 && (is_call(ch) || is_vargs(ch)))
        {
            this->compile_exp_e(ch, EXPECT_FREE);
            this->emit(Bytecode(Opcode::ITList, list_len));
            break;
        }
        else
        {
            size_t key = (list_len++) + 1;
            size_t keyidx = this->const_number(key);
            this->emit(Bytecode(Opcode::IConst, keyidx));
            this->compile_exp(ch);
        }
        if (ch->get_kind() == NodeKind::ExprField)
            this->debug_info(ch->child(0)->line());
        this->emit(Opcode::ITSet);
    }
}
void Compiler::debug_info(size_t line)
{
    this->gen->debug_info(line);
}

void Compiler::compile_exp_e(Noderef node, size_t expect)
{
    bool is_vargs = node->get_kind() == NodeKind::Primary && node->get_token().kind == TokenKind::DotDotDot;
    bool is_fncall = node->get_kind() == NodeKind::Call || node->get_kind() == NodeKind::MethodCall;

    if (!expect && !is_fncall)
        return;
    if (node->get_kind() == NodeKind::Binary)
    {
        TokenKind tk = node->child(1)->get_token().kind;
        if (tk == TokenKind::And || tk == TokenKind::Or)
            this->compile_logic(node);
        else
        {
            this->compile_exp(node->child(0));
            this->compile_exp(node->child(2));
            Token op = node->child(1)->get_token();
            this->debug_info(op.line);
            this->emit(this->translate_token(op.kind, true));
        }
    }
    else if (node->get_kind() == NodeKind::Unary)
    {
        this->compile_exp(node->child(1));
        Token op = node->child(0)->get_token();
        this->debug_info(op.line);
        this->emit(this->translate_token(op.kind, false));
    }
    else if (node->get_kind() == NodeKind::Property)
    {
        this->compile_exp(node->child(0));
        size_t idx = this->const_string(node->child(1)->get_token().text().c_str());
        this->emit(Bytecode(IConst, idx));
        this->emit(ITGet);
    }
    else if (node->get_kind() == NodeKind::Index)
    {
        this->compile_exp(node->child(0));
        this->compile_exp(node->child(1));
        this->emit(ITGet);
    }
    else if (node->get_kind() == NodeKind::FunctionBody || node->get_kind() == NodeKind::MethodBody)
        this->compile_function(node);
    else if (node->get_kind() == NodeKind::Primary)
        this->compile_primary(node, expect);
    else if (node->get_kind() == NodeKind::Table)
        this->compile_table(node);
    else if (node->get_kind() == NodeKind::Call)
        this->compile_call(node, expect);
    else if (node->get_kind() == NodeKind::MethodCall)
        this->compile_methcall(node, expect);
    if (!is_fncall && !is_vargs && expect != EXPECT_FREE)
        while (--expect)
            this->emit(Bytecode(Opcode::INil));
}

Compiler::Compiler(IGenerator *gen) : gen(gen)
{
}

void Compiler::compile_function(Noderef node)
{
    MetaScope *fnscp = (MetaScope *)node->getannot(MetaKind::MScope);
    Compiler compiler(this->gen);
    compiler.stack_offset = node->get_kind() == NodeKind::MethodBody ? 1 : 0;
    compiler.compile(node, this->chunckname);
    compiler.emit(Bytecode(Opcode::IFConst, fnscp->fidx));
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
    MetaDeclaration *md = (MetaDeclaration *)node->getannot(MetaKind::MDecl);
    Noderef dec = md ? md->decnode : node;
    MetaMemory *mm = (MetaMemory *)dec->getannot(MetaKind::MMemory);
    if (mm)
    {
        if (md && md->is_upvalue)
        {
            MetaScope *sc = (MetaScope *)mm->scope->getannot(MetaKind::MScope);
            MetaScope *fnsc = (MetaScope *)sc->func->getannot(MetaKind::MScope);
            this->ops_push(Bytecode(Opcode::IUStore, this->upval(fnsc->fidx, mm->offset, mm->upoffset)));
        }
        else
        {
            this->ops_push(Bytecode(Opcode::ILStore, mm->offset));
        }
    }
    else if (node->getannot(MetaKind::MSelf))
    {
        this->ops_push(Bytecode(Opcode::ILStore, 0));
    }
    else
    {
        const char *str = node->get_token().text().c_str();
        size_t idx = this->const_string(str);
        this->emit(Bytecode(Opcode::IConst, idx));
        this->ops_push(Opcode::IGSet);
        this->vstack.push_back(1);
    }
}

bool Compiler::compile_lvalue(Noderef node)
{
    if (node->get_kind() == NodeKind::Primary || node->get_kind() == NodeKind::Name)
    {
        this->compile_lvalue_primary(node);
        return false;
    }
    if (node->get_kind() == NodeKind::Property)
    {
        Noderef lexp = node->child(0);
        Noderef prop = node->child(1);
        this->compile_exp(lexp);
        Token prop_tkn = prop->get_token();
        const char *prop_str = prop_tkn.text().c_str();
        size_t idx = this->const_string(prop_str);
        this->emit(Bytecode(Opcode::IConst, idx));
        this->ops_push(Bytecode(Opcode::ITSet), prop_tkn.line);
        this->vstack.push_back(1);
        this->vstack.push_back(1);
        return true;
    }
    if (node->get_kind() == NodeKind::Index)
    {
        Noderef lexp = node->child(0);
        Noderef iexp = node->child(1);
        this->compile_exp(lexp);
        this->compile_exp(iexp);
        this->ops_push(Bytecode(Opcode::ITSet), iexp->line());
        this->vstack.push_back(1);
        this->vstack.push_back(1);
        return true;
    }
    return false; // never reaches
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
                this->emit(Bytecode(Opcode::INil));
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

size_t Compiler::compile_varlist(Noderef node)
{
    if (node->child_count() == 1)
    {
        return this->compile_lvalue(node->child(0)) ? 1 : 0;
    }
    else
    {
        size_t varlc = 0;
        for (size_t i = 0; i < node->child_count(); i++)
        {
            varlc += this->compile_lvalue(node->child(i)) ? 1 : 0;
            this->emit(Bytecode(Opcode::INil));
            this->vstack.push_back(0);
        }
        return varlc;
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

void Compiler::compile_generic_for_swap_pair(size_t back_offset1, size_t back_offset2)
{
    this->emit(Bytecode(Opcode::IBLocal, back_offset1));
    this->emit(Bytecode(Opcode::IBLocal, back_offset2 + 1));
    this->emit(Bytecode(Opcode::IBLStore, back_offset1 + 1));
    this->emit(Bytecode(Opcode::IBLStore, back_offset2));
}

void Compiler::compile_generic_for_swap(size_t varcount)
{
    // I
    // S
    // P
    //-----
    // P
    // S
    // I
    if (varcount == 1)
    {
        this->compile_generic_for_swap_pair(1, 3); // I <-> P
    }
    // I
    // S
    // P
    // nil
    //-----
    // P
    // nil
    // S
    // I
    else if (varcount == 2)
    {
        this->compile_generic_for_swap_pair(2, 4);
        this->compile_generic_for_swap_pair(2, 1);
        this->compile_generic_for_swap_pair(2, 3);
    }
    // I
    // S
    // P
    // nil
    // ...
    // nil
    //-----
    // P
    // nil
    // ...
    // nil
    // S
    // I
    else
    {
        this->compile_generic_for_swap_pair(1, varcount);
        this->compile_generic_for_swap_pair(2, varcount - 1);
        this->compile_generic_for_swap_pair(varcount, varcount - 2);
    }
}

void Compiler::compile_generic_for(Noderef node)
{
    Noderef varlist = node->child(0);
    Noderef lvalue = varlist->child(0)->child(0);
    Noderef arglist = node->child(1);
    size_t varcount = varlist->child_count();
    size_t upcount = 0;
    for (size_t i = 0; i < varcount; i++)
    {
        Noderef var = varlist->child(i)->child(0);
        this->varmem(var)->offset = this->stack_offset++;
        MetaMemory *mm = this->varmem(var);
        if (mm->is_upvalue)
        {
            upcount++;
        }
    }
    this->stack_offset += 2;
    //-- push args
    this->compile_explist(arglist, 3);
    for (size_t i = 0; i < varcount - 1; i++)
        this->emit(Opcode::INil);
    //-- push hooks
    for (size_t i = 0; i < upcount; i++)
    {
        this->hookpush();
        this->emit(Opcode::IUPush);
    }
    //-- swap args
    this->compile_generic_for_swap(varcount);
    //-- loop start
    size_t loop_beg = this->len();
    this->emit(Bytecode(Opcode::IBLocal, 1));                           // iterator
    this->emit(Bytecode(Opcode::IBLocal, 3));                           // state
    this->emit(Bytecode(Opcode::ILocal, this->varmem(lvalue)->offset)); // prev
    this->debug_info(arglist->line());
    this->emit(Bytecode(Opcode::ICall, 2, varcount + 1));
    for (size_t i = 0; i < varcount; i++)
        this->emit(Bytecode(Opcode::IBLStore, varcount + 2));
    //-- loop check
    this->emit(Bytecode(Opcode::ILocal, this->varmem(lvalue)->offset));
    this->emit(Opcode::INil);
    this->emit(Opcode::IEq);
    size_t cjmp = this->len();
    this->emit(Bytecode(Opcode::ICjmp, 0x00)); // cjmp to loop end
    //-- block
    this->loop_start();
    this->compile_node(node->child(2));
    this->emit(Bytecode(Opcode::IJmp, loop_beg));
    this->loop_end();
    size_t loop_end = this->len();
    //-- loop end
    this->emit(Bytecode(Opcode::IPop, varcount + 2));
    this->stack_offset -= (varcount + 2);
    this->edit_jmp(cjmp, loop_end);
    for (size_t i = 0; i < upcount; i++)
    {
        this->hookpop();
        this->emit(Opcode::IUPop);
    }
}
void Compiler::seti(size_t idx, lbyte b)
{
    this->gen->seti(idx, b);
}
size_t Compiler::upval(fidx_t fidx, size_t offset, size_t hidx)
{
    return this->gen->upval(fidx, offset, hidx);
}

void Compiler::compile_numeric_for(Noderef node)
{
    Noderef lvalue = node->child(0)->child(0);
    MetaMemory *md = (MetaMemory *)lvalue->getannot(MetaKind::MMemory);
    md->offset = this->stack_offset++;
    this->stack_offset += 2;
    Noderef from = node->child(1);
    Noderef to = node->child(2);
    this->compile_exp(from);
    if (md->is_upvalue)
    {
        this->hookpush();
        this->emit(Opcode::IUPush);
    }
    this->compile_exp(to);
    size_t blk_idx = 3;
    if (node->child_count() == 5)
    {
        blk_idx++;
        this->compile_exp(node->child(3));
    }
    else
        this->emit(Bytecode(Opcode::IConst, this->const_number(1)));
    size_t loop_start = this->len();
    this->loop_start();
    // condition
    this->emit(Bytecode(Opcode::IBLocal, 3)); // index
    this->emit(Bytecode(Opcode::IBLocal, 3)); // limit
    this->emit(Opcode::IGt);
    // jmp to end
    size_t jmp = this->len();
    this->emit(Bytecode(Opcode::ICjmp, 0));
    // block
    this->compile_block(node->child(blk_idx));
    // increment
    this->emit(Bytecode(Opcode::IBLocal, 3)); // index
    this->emit(Bytecode(Opcode::IBLocal, 2)); // step
    this->emit(Opcode::IAdd);
    this->emit(Bytecode(Opcode::IBLStore, 3));
    this->emit(Bytecode(Opcode::IJmp, loop_start));
    this->loop_end();
    this->edit_jmp(jmp, this->len());
    if (md->is_upvalue)
    {
        this->hookpop();
        this->emit(Opcode::IUPop);
    }
    this->emit(Bytecode(Opcode::IPop, 3));
    this->stack_offset -= 3;
}

void Compiler::compile_assignment(Noderef node)
{
    size_t vcount = node->child(0)->child_count();
    size_t varlc = this->compile_varlist(node->child(0));
    this->compile_explist(node->child(1), vcount);

    if (vcount > 1)
    {
        size_t v = vcount;
        while (v)
        {
            this->emit(Bytecode(Opcode::IBLStore, v + this->vstack_nearest_nil()));
            v--;
        }
    }
    this->vstack.clear();
    this->ops_flush();
    if (varlc)
        this->emit(Bytecode(Opcode::IPop, varlc));
}

void Compiler::compile_logic(Noderef node)
{
    this->compile_exp(node->child(0));
    this->emit(Bytecode(Opcode::IBLocal, 1));
    if (node->child(1)->get_token().kind == TokenKind::And)
        this->emit(Opcode::INot);
    size_t cjmp = this->len();
    this->emit(Bytecode(Opcode::ICjmp, 0));
    this->emit(Bytecode(Opcode::IPop, 1));
    this->compile_exp(node->child(2));
    this->edit_jmp(cjmp, this->len());
}

void Compiler::compile_block(Noderef node)
{
    MetaScope *md = (MetaScope *)node->getannot(MetaKind::MScope);
    for (size_t i = 0; i < node->child_count(); i++)
        this->compile_node(node->child(i));
    for (size_t i = 0; i < md->upvalue_size; i++)
    {
        this->hookpop();
        this->emit(Opcode::IUPop);
    }
    if (md->stack_size)
    {
        this->emit(Bytecode(Opcode::IPop, md->stack_size));
        this->stack_offset -= md->stack_size;
    }
}
void Compiler::compile_decl_var(Noderef node)
{
    Noderef varlist = node->child(0);
    size_t upcount = 0;
    for (size_t i = 0; i < varlist->child_count(); i++)
    {
        Noderef var = varlist->child(i)->child(0);
        MetaMemory *mm = (MetaMemory *)var->getannot(MetaKind::MMemory);
        mm->offset = this->stack_offset++;
        if (mm->is_upvalue)
        {
            mm->upoffset = this->hooksize;
            this->hookpush();
            upcount++;
        }
    }
    if (node->child_count() == 2)
        this->compile_explist(node->child(1), varlist->child_count());
    else
        for (size_t i = 0; i < varlist->child_count(); i++)
            this->emit(Bytecode(Opcode::INil));
    while (upcount--)
    {
        this->emit(Opcode::IUPush);
    }
}
void Compiler::compile_decl_func(Noderef node)
{
    Noderef var = node->child(0)->child(0);
    MetaMemory *mm = (MetaMemory *)var->getannot(MetaKind::MMemory);
    mm->offset = this->stack_offset++;
    if (mm->is_upvalue)
    {
        mm->upoffset = this->hooksize;
        this->hookpush();
        this->emit(Opcode::IUPush);
    }
    this->compile_exp(node->child(1));
}

void Compiler::compile_decl(Noderef node)
{
    if (node->child(0)->get_kind() == NodeKind::VarList)
        this->compile_decl_var(node);
    else
        this->compile_decl_func(node);
}
void Compiler::compile_ret(Noderef node)
{
    if (node->child_count())
    {
        Noderef vals = node->child(0);
        this->compile_explist(vals, EXPECT_FREE);
        this->emit(Bytecode(Opcode::IRet, this->arglist_count(vals)));
    }
    else
    {
        this->emit(Bytecode(Opcode::IRet, 0));
    }
}

void Compiler::compile_while(Noderef node)
{
    size_t jmp_idx = this->len();
    this->compile_exp(node->child(0));
    this->emit(Opcode::INot);
    size_t cjmp = this->len();
    this->emit(Bytecode(Opcode::ICjmp, 0));
    this->loop_start();
    this->compile_block(node->child(1));
    this->emit(Bytecode(Opcode::IJmp, jmp_idx));
    this->loop_end();
    this->edit_jmp(cjmp, this->len());
}

void Compiler::compile_repeat(Noderef node)
{
    size_t cjmp_idx = this->len();
    this->loop_start();
    this->compile_block(node->child(0));
    this->compile_exp(node->child(1));
    this->emit(Opcode::INot);
    this->emit(Bytecode(Opcode::ICjmp, cjmp_idx));
    this->loop_end();
}

void Compiler::compile_break()
{
    this->breaks.push_back(this->len());
    this->emit(Bytecode(Opcode::IJmp, 0));
}

void Compiler::compile_goto(Noderef node)
{
    MetaGoto *gtmd = mdgoto(node);
    Noderef label = gtmd->label;
    MetaLabel *lmd = mdlabel(label);
    gtmd->is_compiled = true;
    gtmd->address = this->len();
    this->emit(Bytecode(Opcode::IJmp, lmd->is_compiled ? lmd->address : 0));
}
void Compiler::compile_label(Noderef node)
{
    MetaLabel *lmd = mdlabel(node);
    lmd->is_compiled = true;
    lmd->address = this->len();
    Noderef go_to = lmd->go_to;
    while (go_to)
    {
        MetaGoto *gtmd = mdgoto(go_to);
        if (gtmd->is_compiled)
            this->edit_jmp(gtmd->address, lmd->address);
        go_to = gtmd->next;
    }
}

void Compiler::compile_node(Noderef node)
{
    if (node->get_kind() == NodeKind::AssignStmt)
        this->compile_assignment(node);
    else if (node->get_kind() == NodeKind::Block)
        this->compile_block(node);
    else if (node->get_kind() == NodeKind::Declaration)
        this->compile_decl(node);
    else if (node->get_kind() == NodeKind::ReturnStmt)
        this->compile_ret(node);
    else if (node->get_kind() == NodeKind::CallStmt)
        this->compile_exp_e(node->child(0), 0);
    else if (node->get_kind() == NodeKind::IfStmt)
        this->compile_if(node);
    else if (node->get_kind() == NodeKind::WhileStmt)
        this->compile_while(node);
    else if (node->get_kind() == NodeKind::RepeatStmt)
        this->compile_repeat(node);
    else if (node->get_kind() == NodeKind::NumericFor)
        this->compile_numeric_for(node);
    else if (node->get_kind() == NodeKind::GenericFor)
        this->compile_generic_for(node);
    else if (node->get_kind() == NodeKind::BreakStmt)
        this->compile_break();
    else if (node->get_kind() == NodeKind::GotoStmt)
        this->compile_goto(node);
    else if (node->get_kind() == NodeKind::LabelStmt)
        this->compile_label(node);
    else
        crash("can't compile node");
}

size_t Compiler::const_number(lnumber n)
{
    return this->gen->const_number(n);
}

size_t Compiler::const_string(const char *s)
{
    return this->gen->const_string(s);
}

void Compiler::emit(Bytecode op)
{
    this->gen->emit(op);
}

void Compiler::ops_flush()
{
    while (this->ops.size())
    {
        int line = this->lines.back();
        if (line != -1)
            this->debug_info(line);
        this->emit(this->ops.back());
        this->ops.pop_back();
        this->lines.pop_back();
    }
}

void Compiler::ops_push(Bytecode op)
{
    this->ops_push(op, -1);
}
void Compiler::ops_push(Bytecode op, int line)
{
    this->ops.push_back(op);
    this->lines.push_back(line);
}

void Compiler::loop_start()
{
    this->breaks.push_back(0);
}
void Compiler::loop_end()
{
    size_t idx = this->len();
    while (this->breaks.back())
    {
        size_t brk = this->breaks.back();
        this->edit_jmp(brk, idx);
        this->breaks.pop_back();
    }
    this->breaks.pop_back();
}

void Compiler::hookpush()
{
    this->hooksize++;
    if (this->hookmax < this->hooksize)
        this->hookmax = this->hooksize;
}
void Compiler::hookpop()
{
    this->hooksize--;
}