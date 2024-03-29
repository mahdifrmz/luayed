#ifndef AST_h
#define AST_h

#include <stddef.h>
#include <vector>
#include "lexer.h"
#include <map>

#define is_call(N) ((N)->get_kind() == NodeKind::Call || (N)->get_kind() == NodeKind::MethodCall)
#define is_vargs(N) ((N)->get_kind() == NodeKind::Primary && (N)->get_token().kind == TokenKind::DotDotDot)

#define is_meth(N) (N->get_kind() == NodeKind::MethodBody)

#define foreach_node(PARENT, CHILD) for (ast::Noderef CHILD = PARENT->begin(); CHILD; CHILD = CHILD->next())
#define foreach_node_from(PARENT, CHILD, IDX) for (ast::Noderef CHILD = PARENT->child(IDX); CHILD; CHILD = CHILD->next())

namespace luayed
{
    namespace ast
    {
        class Node;
        typedef Node *Noderef;
        typedef std::map<std::string, Noderef> Varmap;

        enum NodeKind
        {
            Primary = 0, // token
            Binary = 1,
            Unary = 2,
            IdField = 3,
            ExprField = 4,
            Table = 5,
            Property = 6,
            Index = 7,
            Call = 8,
            Explist = 9,
            CallStmt = 10,
            AssignStmt = 11,
            LabelStmt = 12, // token
            BreakStmt = 13,
            GotoStmt = 14, // token
            WhileStmt = 15,
            RepeatStmt = 16,
            IfStmt = 17,
            NumericFor = 18,
            GenericFor = 19,
            ReturnStmt = 20,
            FunctionBody = 21,
            Declaration = 22,
            MethodCall = 23,
            Block = 24,
            Name = 25, // token
            NameList = 26,
            IfClause = 27,
            ElseClause = 28,
            ElseIfClause = 29,
            VarDecl = 30,
            VarList = 31,
            MethodBody = 32,
            Operator = 33, // token
            NEND = 34,     // ctrl
            TKN = 35,      // ctrl
        };

        enum MetaKind
        {
            MDecl = 55,
            MMemory = 1,
            MLabel = 2,
            MScope = 3,
            MSelf = 4,
            MGoto = 5,
            MTail = 6,
        };

        struct MetaData
        {
            MetaData *next = nullptr;
            virtual MetaKind kind() = 0;
            virtual ~MetaData();
        };

        struct MetaSelf : public MetaData
        {
            MetaKind kind();
        };

        struct MetaTail : public MetaData
        {
            MetaKind kind();
        };

        struct MetaDeclaration : public MetaData
        {
            MetaKind kind();

            Noderef decnode = nullptr;
            bool is_upvalue = false;
        };

        struct MetaLabel : public MetaData
        {
            MetaKind kind();
            size_t stack_size = 0;
            size_t upvalue_size = 0;
            Noderef go_to = nullptr;
            size_t address = 0;
            bool is_compiled = false;
        };

        struct MetaGoto : public MetaData
        {
            MetaKind kind();
            size_t stack_size = 0;
            size_t upvalue_size = 0;
            size_t address = 0;
            bool is_compiled = false;
            Noderef label = nullptr;
            Noderef next = nullptr;
        };

        struct MetaMemory : public MetaData
        {
            MetaKind kind();
            Noderef scope = nullptr;
            size_t offset = 0;
            bool is_upvalue = false;
            size_t upoffset = 0;
        };

        struct MetaScope : public MetaData
        {
            MetaKind kind();
            Noderef func = nullptr;
            Noderef parent = nullptr;
            size_t stack_size = 0;
            size_t upvalue_size = 0;
            bool variadic = false;
            Varmap map;
            Varmap lmap;
            Noderef gotolist = nullptr;
            fidx_t fidx = 0;
        };

        class Node
        {

            friend class Ast;

        private:
            Token token;
            NodeKind kind;
            size_t count = 0;
            MetaData *meta = nullptr;

            Noderef parent = nullptr;
            Noderef right_sib = nullptr;
            Noderef left_sib = nullptr;
            Noderef left_child = nullptr;
            Noderef right_child = nullptr;

            MetaData *getannot(MetaKind kind);

        public:
            Noderef next();
            NodeKind get_kind();
            Token get_token();
            Noderef child(size_t index);
            size_t child_count();
            Node(NodeKind kind, Token token = token_none());
            void annotate(MetaData *md);
            int line();

            MetaGoto *metadata_goto();
            MetaLabel *metadata_label();
            MetaDeclaration *metadata_decl();
            MetaMemory *metadata_memory();
            MetaScope *metadata_scope();
            MetaSelf *metadata_self();
            MetaTail *metadata_tail();

            static void
            sib_insert(Noderef l, Noderef r, Noderef s);
            void sib_insertl(Noderef node);
            void sib_insertr(Noderef node);
            void child_pushl(Noderef node);
            void child_pushr(Noderef node);
            Noderef begin();
            Noderef end();
            void pop();
            void replace(Noderef other);
        };

        class Ast
        {
        private:
            Noderef tree = nullptr;
            size_t *counter = nullptr;
            void destroy_node(Noderef node);
            void destroy();

        public:
            static Noderef make(NodeKind kind);
            static Noderef make(const vector<Noderef> &nodes, NodeKind kind);
            static Noderef make(vector<Noderef> &&nodes, NodeKind kind);
            static Noderef make(Token token, NodeKind kind);

            Ast &operator=(const Ast &other);
            Ast(const Ast &other);

            Ast &operator=(Ast &&other);
            Ast(Ast &&other);

            Noderef root();
            Ast(Noderef tree);
            ~Ast();
        };
    }
};

#endif