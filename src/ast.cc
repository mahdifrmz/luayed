#include "ast.h"

using namespace ast;

Node::Node(Token token, NodeKind kind) : token(token),
                                         children(nullptr), count(0), kind(kind)
{
}

Node::Node(Noderef *children, size_t count, NodeKind kind)
    : token(token_none()), children(children), count(count), kind(kind)
{
}

NodeKind Node::get_kind()
{
    return this->kind;
}

Token Node::get_token()
{
    return this->token;
}

Noderef *Node::get_children()
{
    return this->children;
}
size_t Node::child_count()
{
    return this->count;
}

void Ast::destroy()
{
    if (this->tree)
        this->destroy_node(this->tree);
}
void Ast::destroy_node(Noderef node)
{
    while (node->meta)
    {
        MetaNode *md = node->meta;
        node->meta = md->next;
        delete md;
    }
    for (size_t i = 0; i < node->child_count(); i++)
    {
        Noderef ch = node->child(i);
        this->destroy_node(ch);
    }
    delete[] node->children;
    delete node;
}

Ast::Ast(Noderef tree) : tree(tree)
{
}

Noderef Ast::root()
{
    return this->tree;
}

void Node::annotate(MetaNode *md)
{
    md->next = this->meta;
    this->meta = md;
}

Noderef Node::child(size_t index)
{
    return this->children[index];
}

MetaNode *Node::getannot(MetaKind kind)
{
    MetaNode *tmp = this->meta;
    while (tmp != nullptr && tmp->kind != kind)
        tmp = tmp->next;
    return tmp;
}

Noderef Ast::make(NodeKind kind)
{
    return new Node(token_none(), kind);
}
Noderef Ast::make(vector<Noderef> &nodes, NodeKind kind)
{
    Noderef *children = new Noderef[nodes.size()];
    for (size_t i = 0; i < nodes.size(); i++)
        children[i] = nodes[i];
    return new Node(children, nodes.size(), kind);
}
Noderef Ast::make(Token token, NodeKind kind)
{
    return new Node(token, kind);
}
Noderef Ast::make(Noderef c1, NodeKind kind)
{
    Noderef *nodes = new Noderef[1];
    nodes[0] = c1;
    return new Node(nodes, 1, kind);
}
Noderef Ast::make(Noderef c1, Noderef c2, NodeKind kind)
{
    Noderef *nodes = new Noderef[2];
    nodes[0] = c1;
    nodes[1] = c2;
    return new Node(nodes, 2, kind);
}
Noderef Ast::make(Noderef c1, Noderef c2, Noderef c3, NodeKind kind)
{
    Noderef *nodes = new Noderef[3];
    nodes[0] = c1;
    nodes[1] = c2;
    nodes[2] = c3;
    return new Node(nodes, 3, kind);
}
Noderef Ast::make(Noderef c1, Noderef c2, Noderef c3, Noderef c4, NodeKind kind)
{
    Noderef *nodes = new Noderef[4];
    nodes[0] = c1;
    nodes[1] = c2;
    nodes[2] = c3;
    nodes[3] = c4;
    return new Node(nodes, 4, kind);
}
Noderef Ast::make(Noderef c1, Noderef c2, Noderef c3, Noderef c4, Noderef c5, NodeKind kind)
{
    Noderef *nodes = new Noderef[5];
    nodes[0] = c1;
    nodes[1] = c2;
    nodes[2] = c3;
    nodes[3] = c4;
    nodes[4] = c5;
    return new Node(nodes, 5, kind);
}
