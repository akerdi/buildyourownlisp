#include "mpc.h"

int strlength(char *s)
{
    return strlen(s) + 1;
}

#ifdef __WIN32
#define BUFFER_LENGTH 1024
static char buffer[BUFFER_LENGTH];
char *readline(char *prompt)
{
    fputs(prompt, stdout);
    fgets(buffer, BUFFER_LENGTH, stdin);
    char *cpy = malloc(strlength(buffer));
    strcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}
void add_history(char *input) {}
#else
#include <editline/readline.h>
#endif

mpc_parser_t *Number;
mpc_parser_t *Symbol;
mpc_parser_t *String;
mpc_parser_t *Comment;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *Lispy;

enum
{
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_STR,
    LVAL_SEXPR,
    LVAL_QEXPR,
    LVAL_FUN
};
char *ltype_name(int type)
{
    switch (type)
    {
    case LVAL_ERR:
        return "<error>";
    case LVAL_NUM:
        return "<number>";
    case LVAL_SYM:
        return "<symbol>";
    case LVAL_STR:
        return "<string>";
    case LVAL_SEXPR:
        return "<sexpr>";
    case LVAL_QEXPR:
        return "<qexpr>";
    default:
        return "<Unbound function!>";
    }
}
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;
typedef lval *(*lbuildin)(lenv *e, lval *v);

struct lval
{
    int type;

    long num;
    char *err;
    char *sym;
    char *str;
    //    lbuildin fun;
    lbuildin buildin;

    lenv *env;
    lval *formals;
    lval *body;

    int count;
    lval **cell;
};

#define NEWLVAL lval *v = malloc(sizeof(lval))
#define StringNew(s) malloc(sizeof(char) * strlength(s))
#define StringNewCpy(sptr, s) \
    sptr = StringNew(s);      \
    strcpy(sptr, s)
#define FORLESS(count) for (int i = 0; i < count; i++)
#define LASSERT(arg, cond, fmt, ...)              \
    if (!(cond))                                  \
    {                                             \
        lval *err = lval_err(fmt, ##__VA_ARGS__); \
        lval_del(arg);                            \
        return err;                               \
    }
#define LASSERT_NUM(func, arg, expect) LASSERT(arg, arg->count == expect, "Function '%s' passed invalid count of Arguments." \
                                                                          "Got %i, Expect %i",                               \
                                               func, arg->count, expect)
#define LASSERT_TYPE(func, arg, i, expect) LASSERT(arg, arg->cell[i]->type == expect, "Function '%s' passed invalid format type." \
                                                                                      "Got %s, Expect %s",                        \
                                                   func, ltype_name(arg->cell[i]->type), ltype_name(expect))
#define LASSERT_NOT_EMPTY(func, arg, i) LASSERT(arg, arg->cell[i]->count != 0, "Function '%s' passed '{}' empty argument at index: %i!", func, i)

/////////////
void lval_print(lval *v);
void lval_println(lval *v);
lval *lval_eval(lenv *e, lval *v);
lenv *lenv_new();
void lenv_del(lenv *e);
lenv *lenv_copy(lenv *);
/////////////

lval *lval_num(long num)
{
    NEWLVAL;
    v->type = LVAL_NUM;
    v->num = num;
    return v;
}
lval *lval_err(char *fmt, ...)
{
    NEWLVAL;
    v->type = LVAL_ERR;
    va_list va;
    va_start(va, fmt);
    static int ERR_LENGTH = 512;
    v->err = malloc(ERR_LENGTH);
    vsnprintf(v->err, ERR_LENGTH - 1, fmt, va);
    v->err = realloc(v->err, strlength(v->err));
    va_end(va);
    return v;
}
lval *lval_sym(char *op)
{
    NEWLVAL;
    v->type = LVAL_SYM;
    StringNewCpy(v->sym, op);
    return v;
}
lval *lval_str(char *str)
{
    NEWLVAL;
    v->type = LVAL_STR;
    StringNewCpy(v->str, str);
    return v;
}
lval *lval_fun(lbuildin fun)
{
    NEWLVAL;
    v->type = LVAL_FUN;
    v->buildin = fun;
    return v;
}
lval *lval_lambda(lval *formals, lval *body)
{
    NEWLVAL;
    v->type = LVAL_FUN;
    v->buildin = NULL;
    v->formals = formals;
    v->body = body;
    v->env = lenv_new();
    return v;
}
lval *lval_sexpr(void)
{
    NEWLVAL;
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}
lval *lval_qexpr(void)
{
    NEWLVAL;
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}
lval *lval_check_num(char *numstr)
{
    errno = 0;
    long num = strtol(numstr, NULL, 10);
    return errno != 0 ? lval_err("invalid err!") : lval_num(num);
}
lval *lval_check_string(char *content)
{
    content[strlen(content) - 1] = '\0';
    char *cpy = malloc(strlen(content));
    strcpy(cpy, content + 1);
    lval *strVal = lval_str(cpy);
    free(cpy);
    return strVal;
}
void lval_del(lval *v);
void lval_expr_del(lval *v)
{
    FORLESS(v->count)
    {
        lval_del(v->cell[i]);
    }
    free(v->cell);
}
void lval_del(lval *v)
{
    switch (v->type)
    {
    case LVAL_ERR:
        free(v->err);
        break;
    case LVAL_FUN:
        if (!v->buildin)
        {
            lval_del(v->formals);
            lval_del(v->body);
            lenv_del(v->env);
        }
        break;
    case LVAL_NUM:
        break;
    case LVAL_SYM:
        free(v->sym);
        break;
    case LVAL_STR:
        free(v->str);
        break;
    case LVAL_QEXPR:
    case LVAL_SEXPR:
        lval_expr_del(v);
        break;
    }
    free(v);
}
lval *lval_add(lval *v, lval *a)
{
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    v->cell[v->count - 1] = a;
    return v;
}
lval *lval_pop(lval *v, int i)
{
    lval *x = v->cell[i];
    lval **a = &v->cell[i];
    lval **b = &v->cell[i + 1];
    memmove(a, b, sizeof(lval *) * (v->count - i - 1));
    //    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*)*(v->count-i-1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    return x;
}
lval *lval_take(lval *v, int i)
{
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}
lval *lval_copy(lval *a)
{
    NEWLVAL;
    v->type = a->type;
    switch (v->type)
    {
    case LVAL_NUM:
        v->num = a->num;
        break;
    case LVAL_FUN:
    {
        if (a->buildin)
        {
            v->buildin = a->buildin;
        }
        else
        {
            v->buildin = NULL;
            v->env = lenv_copy(a->env);
            v->formals = lval_copy(a->formals);
            v->body = lval_copy(a->body);
        }
    }
    break;
    case LVAL_ERR:
        StringNewCpy(v->err, a->err);
        break;
    case LVAL_SYM:
        StringNewCpy(v->sym, a->sym);
        break;
    case LVAL_STR:
        StringNewCpy(v->str, a->str);
        break;
    case LVAL_SEXPR:
    case LVAL_QEXPR:
        v->count = a->count;
        v->cell = malloc(sizeof(lval *) * v->count);
        FORLESS(v->count)
        {
            v->cell[i] = lval_copy(a->cell[i]);
        }
        break;
    }
    return v;
}
struct lenv
{
    lenv *pair; // 装父节点

    int count;
    char **syms;
    lval **vals;
};
lenv *lenv_new()
{
    lenv *e = malloc(sizeof(lenv));
    e->count = 0;
    e->pair = NULL;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}
void lenv_del(lenv *e)
{
    FORLESS(e->count)
    {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}
lenv *lenv_copy(lenv *e)
{
    lenv *n = malloc(sizeof(lenv));
    n->count = e->count;
    n->pair = e->pair;
    n->syms = malloc(sizeof(char *) * n->count);
    n->vals = malloc(sizeof(lval *) * n->count);
    FORLESS(n->count)
    {
        StringNewCpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}
lval *lenv_get(lenv *e, lval *k)
{
    FORLESS(e->count)
    {
        if (strcmp(e->syms[i], k->sym) == 0)
        {
            return lval_copy(e->vals[i]);
        }
    }
    if (e->pair)
    {
        return lenv_get(e->pair, k);
    }
    return lval_err("Unbound Function: %s", k->sym);
}
void lenv_put(lenv *e, lval *k, lval *v)
{
    FORLESS(e->count)
    {
        if (strcmp(e->syms[i], k->sym) == 0)
        {
            lval_del(e->vals[i]); // 删除之前节点
            e->vals[i] = lval_copy(v);
            return;
        }
    }
    e->count++;
    e->syms = realloc(e->syms, sizeof(char *) * e->count);
    e->vals = reallocf(e->vals, sizeof(lval *) * e->count);
    e->vals[e->count - 1] = lval_copy(v);
    StringNewCpy(e->syms[e->count - 1], k->sym);
}
void lenv_def(lenv *e, lval *k, lval *v)
{
    while (e->pair)
    {
        e = e->pair;
    }
    lenv_put(e, k, v);
}

lval *lval_read(mpc_ast_t *t);
lval *lval_expr_read(mpc_ast_t *t)
{
    lval *x;
    if (strcmp(t->tag, ">") == 0)
    {
        x = lval_sexpr();
    }
    if (strstr(t->tag, "sexpr"))
    {
        x = lval_sexpr();
    }
    if (strstr(t->tag, "qexpr"))
    {
        x = lval_qexpr();
    }

    FORLESS(t->children_num)
    {
        if (strcmp(t->children[i]->contents, "(") == 0)
            continue;
        if (strcmp(t->children[i]->contents, ")") == 0)
            continue;
        if (strcmp(t->children[i]->contents, "{") == 0)
            continue;
        if (strcmp(t->children[i]->contents, "}") == 0)
            continue;
        if (strcmp(t->children[i]->tag, "regex") == 0)
            continue;
        if (strstr(t->children[i]->tag, "comment"))
            continue;
        x = lval_add(x, lval_read(t->children[i]));
    }
    return x;
}
lval *lval_read(mpc_ast_t *t)
{
    if (strstr(t->tag, "number"))
    {
        return lval_check_num(t->contents);
    }
    if (strstr(t->tag, "symbol"))
    {
        return lval_sym(t->contents);
    }
    if (strstr(t->tag, "string"))
    {
        return lval_check_string(t->contents);
    }

    return lval_expr_read(t);
}

lval *buildin_op(lenv *e, lval *v, char *op)
{
    FORLESS(v->count)
    {
        if (v->cell[i]->type != LVAL_NUM)
        {
            lval_del(v);
            return lval_err("buildin op operate on non-number!");
        }
    }
    lval *x = lval_pop(v, 0);
    if (v->count == 0 && strcmp(op, "-") == 0)
    {
        x->num = -x->num;
    }
    while (v->count)
    {
        lval *y = lval_pop(v, 0);
        if (strcmp(op, "+") == 0)
        {
            x->num += y->num;
        }
        if (strcmp(op, "-") == 0)
        {
            x->num -= y->num;
        }
        if (strcmp(op, "*") == 0)
        {
            x->num *= y->num;
        }
        if (strcmp(op, "/") == 0)
        {
            if (y->num == 0)
            {
                lval_del(x);
                x = lval_err("Division By Zero!");
                break;
            }
            x->num /= y->num;
        }
        lval_del(y);
    }
    lval_del(v);
    return x;
}
lval *buildin_head(lenv *e, lval *v)
{
    LASSERT_NUM("head", v, 1);
    LASSERT_TYPE("head", v, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", v, 0);
    lval *x = lval_take(v, 0);
    while (x->count > 1)
    {
        lval_del(lval_pop(x, 1));
    }
    return x;
}
lval *buildin_tail(lenv *e, lval *v)
{
    LASSERT_NUM("tail", v, 1);
    LASSERT_TYPE("tail", v, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", v, 0);
    lval *x = lval_take(v, 0);
    lval_del(lval_pop(x, 0));
    return x;
}
lval *buildin_list(lenv *e, lval *v)
{
    v->type = LVAL_QEXPR;
    return v;
}
lval *buildin_eval(lenv *e, lval *v)
{
    LASSERT_NUM("eval", v, 1);
    LASSERT_TYPE("eval", v, 0, LVAL_QEXPR);
    lval *x = lval_take(v, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}
lval *buildin_join(lenv *e, lval *v)
{
    FORLESS(v->count)
    {
        LASSERT_TYPE("join", v, i, LVAL_QEXPR);
    }
    lval *x = lval_pop(v, 0);
    while (v->count)
    {
        lval *y = lval_pop(v, 0);
        FORLESS(y->count)
        {
            //            x = lval_add(x, lval_pop(y, 0));
            x = lval_add(x, y->cell[i]);
        }
        free(y->cell);
        lval_del(y);
    }
    lval_del(v);
    return x;
}
lval *buildin_add(lenv *e, lval *v)
{
    return buildin_op(e, v, "+");
}
lval *buildin_sub(lenv *e, lval *v)
{
    return buildin_op(e, v, "-");
}
lval *buildin_mul(lenv *e, lval *v)
{
    return buildin_op(e, v, "*");
}
lval *buildin_div(lenv *e, lval *v)
{
    return buildin_op(e, v, "/");
}
lval *buildin_var(lenv *e, lval *v, char *op)
{
    LASSERT_TYPE("def", v, 0, LVAL_QEXPR);
    lval *syms = v->cell[0];
    LASSERT(v, syms->count == v->count - 1, "Function '%s' passed invalid argument!"
                                            "params count %i, argument count %i",
            "def", syms->count, (v->count - 1));
    FORLESS(syms->count)
    {
        if (strcmp(op, "def") == 0)
        {
            lenv_def(e, syms->cell[i], v->cell[i + 1]);
        }
        else
        {
            lenv_put(e, syms->cell[i], v->cell[i + 1]);
        }
    }
    return lval_sexpr();
}
lval *buildin_def(lenv *e, lval *v)
{
    return buildin_var(e, v, "def");
}
lval *buildin_put(lenv *e, lval *v)
{
    return buildin_var(e, v, "put");
}

lval *buildin_lambda(lenv *e, lval *v)
{
    LASSERT_NUM("\\", v, 2);
    LASSERT_TYPE("\\", v, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", v, 1, LVAL_QEXPR);
    lval *formals = lval_pop(v, 0);
    lval *body = lval_pop(v, 0);
    lval_del(v);
    return lval_lambda(formals, body);
}
lval *buildin_ord(lenv *e, lval *v, char *op)
{
    LASSERT_NUM(op, v, 2);
    LASSERT_TYPE(op, v, 0, LVAL_NUM);
    LASSERT_TYPE(op, v, 1, LVAL_NUM);
    int res = 0;
    if (strcmp(op, ">") == 0)
    {
        res = v->cell[0]->num > v->cell[1]->num;
    }
    if (strcmp(op, ">=") == 0)
    {
        res = v->cell[0]->num >= v->cell[1]->num;
    }
    if (strcmp(op, "<") == 0)
    {
        res = v->cell[0]->num < v->cell[1]->num;
    }
    if (strcmp(op, "<=") == 0)
    {
        res = v->cell[0]->num <= v->cell[1]->num;
    }
    lval_del(v);
    return lval_num(res);
}
lval *buildin_gt(lenv *e, lval *v)
{
    return buildin_ord(e, v, ">");
}
lval *buildin_ge(lenv *e, lval *v)
{
    return buildin_ord(e, v, ">=");
}
lval *buildin_lt(lenv *e, lval *v)
{
    return buildin_ord(e, v, "<");
}
lval *buildin_le(lenv *e, lval *v)
{
    return buildin_ord(e, v, "<=");
}
int lval_compare(lval *x, lval *y)
{
    if (x->type != y->type)
        return 0;
    switch (x->type)
    {
    case LVAL_NUM:
        return x->num == y->num;
    case LVAL_SYM:
        return strcmp(x->sym, y->sym) == 0;
    case LVAL_ERR:
        return strcmp(x->err, y->err) == 0;
    case LVAL_STR:
        return strcmp(x->str, y->str) == 0;
    case LVAL_FUN:
        if (x->buildin)
            return x->buildin == y->buildin;
        return lval_compare(x->formals, y->formals) && lval_compare(x->body, y->body);
    case LVAL_SEXPR:
    case LVAL_QEXPR:
        if (x->count != y->count)
            return 0;
        FORLESS(x->count)
        {
            if (lval_compare(x->cell[i], y->cell[i]) == 0)
                return 0;
        }
        return 1;
    }
    return 1;
}
lval *buildin_com(lenv *e, lval *v, char *op)
{
    LASSERT_NUM(op, v, 2);

    lval *a = lval_pop(v, 0);
    lval *b = lval_pop(v, 0);
    int x = 0;
    if (strcmp(op, "==") == 0)
    {
        x = lval_compare(a, b);
    }
    else
    {
        x = !lval_compare(a, b);
    }
    lval_del(v);
    return lval_num(x);
}
lval *buildin_eq(lenv *e, lval *v)
{
    return buildin_com(e, v, "==");
}
lval *buildin_ne(lenv *e, lval *v)
{
    return buildin_com(e, v, "!=");
}
lval *buildin_if(lenv *e, lval *v)
{
    LASSERT_NUM("if", v, 3);
    LASSERT_TYPE("if", v, 0, LVAL_NUM);
    LASSERT_TYPE("if", v, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", v, 2, LVAL_QEXPR);
    v->cell[1]->type = LVAL_SEXPR;
    v->cell[2]->type = LVAL_SEXPR;
    lval *x;
    if (v->cell[0]->num >= 1)
    {
        // lval_eval 是直接执行语句
        // buildin_eval 支持完整program, 所以其子对象必须是Q-Expression
        // 所以这里直接把 元素从v 中pop出来成为独立对象(其在执行时被释放)
        x = lval_eval(e, lval_pop(v, 1));
    }
    else
    {
        x = lval_eval(e, lval_pop(v, 2));
    }
    lval_del(v);
    return x;
}
// load "lispy.lsp"
lval *buildin_load(lenv *e, lval *v)
{
    LASSERT_NUM("load", v, 1);
    LASSERT_TYPE("load", v, 0, LVAL_STR);
    mpc_result_t r;
    if (mpc_parse_contents(v->cell[0]->str, Lispy, &r))
    {
        lval *expr = lval_read(r.output);
        mpc_ast_delete(r.output);
        int i = 0;
        // 逐个运行
        while (expr->count)
        {
            lval *x = lval_eval(e, lval_pop(expr, 0));
            if (x->type == LVAL_ERR)
            {
                lval_println(x);
            }
            lval_del(x);
            i++;
        }
        lval_del(expr);
        lval_del(v);

        return lval_sexpr();
    }
    else
    {
        lval_del(v);
        char *err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        lval *err = lval_err(err_msg);
        free(err_msg);

        return err;
    }
}
lval *buildin_print(lenv *e, lval *v)
{
    FORLESS(v->count)
    {
        lval_print(v->cell[i]);
        putchar(' ');
    }
    putchar('\n');
    lval_del(v);
    return lval_sexpr();
}
lval *lval_call(lenv *e, lval *v, lval *f)
{
    if (f->buildin)
        return f->buildin(e, v);

    int count = v->count;
    int total = f->formals->count;
    while (v->count)
    {
        if (f->formals->count == 0)
        {
            lval_del(v);
            return lval_err("Function '%s' passedd too many argument!"
                            "Got %i, Expect %i",
                            "call", count, total);
        }
        // 此处将fun中的形参拿出，同时将参数对应上
        // 然后保存到f->env 环境变量中
        lval *sym = lval_pop(f->formals, 0);
        // def { addCurry } (\ { x y & last } { eval (join (list + x y) (head last) ) })
        // addCurry 10 20 40 30 -> 10 + 20 + 40 = 70
        if (strcmp(sym->sym, "&") == 0)
        {
            if (f->formals->count != 1)
            {
                lval_del(sym);
                lval_del(v);
                return lval_err("Function '%s' pass invalid format argument!"
                                "Symbol '&' must followed by only one symbol!"
                                "Got %i, Expect %i",
                                "call", f->formals->count, 1);
            }
            lval *syms = lval_pop(f->formals, 0);
            lval *vals = buildin_list(f->env, v);
            lenv_put(f->env, syms, vals);
            lval_del(sym);
            lval_del(syms);
            break;
        }
        lval *val = lval_pop(v, 0);
        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }
    lval_del(v);
    // 这里的作用，是在上面执行后，仍然还有 `& x`
    // 其实参数已经写完，只剩复数参数，那么直接给其复制空参数即可执行了
    if (f->formals->count != 0 && strcmp(f->formals->cell[0]->sym, "&") == 0)
    {
        if (f->formals->count != 2)
        {
            return lval_err("Function '%s' pass invalid format argument!"
                            "Symbol '&' must followed by only one symbol!"
                            "Got %i, Expect %i",
                            "call", f->formals->count, 1);
        }
        lval_del(lval_pop(f->formals, 0));
        lval *syms = lval_pop(f->formals, 0);
        lval *vals = lval_qexpr(); // 空参数
        lenv_put(f->env, syms, vals);
        lval_del(syms);
        lval_del(vals);
    }
    // 说明参数全，开始运行
    if (f->formals->count == 0)
    {
        // 为当前fun的局部环境变量增加父级环境变量
        f->env->pair = e;
        // 执行时，将环境变量传入，相当于提前有了相关的环境变量
        // 执行语句是body，拷贝一份给 buildin_eval 去执行
        return buildin_eval(f->env, lval_add(lval_qexpr(), lval_copy(f->body)));
    }
    else
    {
        return lval_copy(f);
    }
}
lval *lval_expr_eval(lenv *e, lval *v)
{
    FORLESS(v->count)
    {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }
    FORLESS(v->count)
    {
        if (v->cell[i]->type == LVAL_ERR)
        {
            return lval_take(v, i);
        }
    }
    if (v->count == 0)
        return v;
    if (v->count == 1)
        return lval_take(v, 0);

    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUN)
    {
        lval_del(f);
        lval_del(v);
        return lval_err("S-Expression not start with Function!");
    }
    lval *res = lval_call(e, v, f);
    //    lval* res = f->fun(e, v);
    //    lval* res = buildin(v, f->sym);
    //    lval* res = buildin_op(v, f->sym);
    lval_del(f);
    return res;
}
lval *lval_eval(lenv *e, lval *v)
{
    if (v->type == LVAL_SYM)
    {
        lval *x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if (v->type == LVAL_SEXPR)
        return lval_expr_eval(e, v);
    return v;
}
void lenv_add_buildin(lenv *e, char *sym, lbuildin fun)
{
    lval *symVal = lval_sym(sym);
    lval *funVal = lval_fun(fun);
    lenv_put(e, symVal, funVal);
    lval_del(symVal);
    lval_del(funVal);
}
void lenv_add_buildins(lenv *e)
{
    lenv_add_buildin(e, "head", buildin_head);
    lenv_add_buildin(e, "tail", buildin_tail);
    lenv_add_buildin(e, "list", buildin_list);
    lenv_add_buildin(e, "eval", buildin_eval);
    lenv_add_buildin(e, "join", buildin_join);
    lenv_add_buildin(e, "def", buildin_def);
    lenv_add_buildin(e, "=", buildin_put);
    lenv_add_buildin(e, "\\", buildin_lambda);
    lenv_add_buildin(e, "load", buildin_load);
    lenv_add_buildin(e, "print", buildin_print);

    lenv_add_buildin(e, ">", buildin_gt);
    lenv_add_buildin(e, ">=", buildin_ge);
    lenv_add_buildin(e, "<", buildin_lt);
    lenv_add_buildin(e, "<=", buildin_le);
    lenv_add_buildin(e, "==", buildin_eq);
    lenv_add_buildin(e, "!=", buildin_ne);
    lenv_add_buildin(e, "if", buildin_if);

    lenv_add_buildin(e, "+", buildin_add);
    lenv_add_buildin(e, "-", buildin_sub);
    lenv_add_buildin(e, "*", buildin_mul);
    lenv_add_buildin(e, "/", buildin_div);
}

int main(int argc, char **argv)
{
    Number = mpc_new("number");
    Symbol = mpc_new("symbol");
    String = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Lispy = mpc_new("lispy");
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                     \
      number : /-?[0-9]+/ ;                               \
      symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;        \
      string : /\\\"(\\\\.|[^\"])*\"/ ;                   \
      comment : /;[^\\r\\n]*/ ;                           \
      sexpr  : '(' <expr>* ')' ;                          \
      qexpr  : '{' <expr>* '}' ;                          \
      expr   : <number> | <symbol> | <string> | <comment> | <sexpr> | <qexpr> ;  \
      lispy  : /^/ <expr>* /$/ ;                          \
    ",
              Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);
    puts("Lispy Version 0.0.0.0.14");
    puts("Press Ctrl+c to Exit\n");

    lenv *e = lenv_new();
    lenv_add_buildins(e);

    while (1)
    {
        char *input = readline("lispy> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r))
        {
            lval *res = lval_read(r.output);
            lval *x = lval_eval(e, res);
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        }
        else
        {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    lenv_del(e);

    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}

void lval_expr_print(lval *v, char open, char close)
{
    putchar(open);
    FORLESS(v->count)
    {
        lval_print(v->cell[i]);
        if (i != (v->count - 1))
        {
            putchar(' ');
        }
    }
    putchar(close);
}
void lval_print(lval *v)
{
    switch (v->type)
    {
    case LVAL_ERR:
        printf("%s", v->err);
        break;
    case LVAL_NUM:
        printf("%i", v->num);
        break;
    case LVAL_FUN:
    {
        if (v->buildin)
        {
            printf("<Function>");
        }
        else
        {
            putchar('\\');
            putchar(' ');
            lval_expr_print(v->formals, '{', '}');
            putchar(' ');
            lval_expr_print(v->body, '{', '}');
        }
    }
    break;
    case LVAL_SYM:
        printf("%s", v->sym);
        break;
    case LVAL_STR:
        printf("\"%s\"", v->str);
        break;
    case LVAL_SEXPR:
        lval_expr_print(v, '(', ')');
        break;
    case LVAL_QEXPR:
        lval_expr_print(v, '{', '}');
        break;
    }
}
void lval_println(lval *v)
{
    lval_print(v);
    putchar('\n');
}