#include "mpc.h"

int strlength(char* s) {
    return strlen(s) + 1;
}

#ifdef __WIN32
#define BUFFER_LENGTH 1024
static char buffer[BUFFER_LENGTH];
char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, BUFFER_LENGTH, stdin);
    char* cpy = malloc(strlength(buffer));
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}
void add_history(char* input) { }
#else
#include <editline/readline.h>
#endif

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };
char* ltype_name(int type) {
    switch (type) {
        case LVAL_ERR: return "<error>";
        case LVAL_NUM: return "<number>";
        case LVAL_SYM: return "<symbol>";
        case LVAL_SEXPR: return "<sexpr>";
        case LVAL_QEXPR: return "<qexpr>";
        default: return "<Unbound function!>";
    }
}
struct lval;
typedef struct lval lval;

struct lval {
    int type;

    long num;
    char* err;
    char* sym;

    int count;
    lval** cell;
};

#define NEWLVAL lval* v = malloc(sizeof(lval))
#define SMALLOC(s) malloc(sizeof(char)*strlength(s))
#define SMAL_CPY(sptr, s) sptr = SMALLOC(s); strcpy(sptr, s)
#define FORLESS(count) for (int i = 0; i < count; i++)
#define LASSERT(arg, cond, fmt, ...) if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(arg); return err; }
#define LASSERT_NUM(func, arg, expect) LASSERT(arg, arg->count == expect, "Function '%s' passed invalid count of Arguments." \
    "Got %i, Expect %i", func, arg->count, expect)
#define LASSERT_TYPE(func, arg, i, expect) LASSERT(arg, arg->cell[i]->type == expect, "Function '%s' passed invalid format type." \
    "Got %s, Expect %s", func, ltype_name(arg->cell[i]->type), ltype_name(expect))
#define LASSERT_NOT_EMPTY(func, arg, i) LASSERT(arg, arg->cell[i]->count != 0, "Function '%s' passed '{}' empty argument at index: %i!", func, i)

/////////////
void lval_print(lval* v);
void lval_println(lval* v);
lval* lval_eval(lval* v);
/////////////

lval* lval_num(long num) {
    NEWLVAL;
    v->type = LVAL_NUM;
    v->num = num;
    return v;
}
lval* lval_err(char* fmt, ...) {
    NEWLVAL;
    v->type = LVAL_ERR;
    va_list  va;
    va_start(va, fmt);
    static int ERR_LENGTH = 512;
    v->err = malloc(ERR_LENGTH);
    vsnprintf(v->err, ERR_LENGTH-1, fmt, va);
    v->err = realloc(v->err, strlength(v->err));
    va_end(va);
    return v;
}
lval* lval_sym(char* op) {
    NEWLVAL;
    v->type = LVAL_SYM;
    SMAL_CPY(v->sym, op);
    return v;
}
lval* lval_sexpr(void) {
    NEWLVAL;
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}
lval* lval_qexpr(void) {
    NEWLVAL;
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}
lval* lval_check_num(char* numstr) {
    errno = 0;
    long num = strtol(numstr, NULL, 10);
    return errno != 0 ? lval_err("invalid err!") : lval_num(num);
}
void lval_del(lval* v);
void lval_expr_del(lval* v) {
    FORLESS(v->count) {
        lval_del(v->cell[i]);
    }
    free(v->cell);
}
void lval_del(lval* v) {
    switch (v->type) {
        case LVAL_ERR: free(v->err);
            break;
        case LVAL_NUM:
            break;
        case LVAL_SYM: free(v->sym);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR: lval_expr_del(v);
            break;
    }
    free(v);
}
lval* lval_add(lval* v, lval* a) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*)*v->count);
    v->cell[v->count-1] = a;
    return v;
}
lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];
    lval** a = &v->cell[i];
    lval** b = &v->cell[i+1];
    memmove(a, b, sizeof(lval*)*(v->count-i-1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*)*v->count);
    return x;
}
lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}
lval* lval_read(mpc_ast_t* t);
lval* lval_expr_read(mpc_ast_t* t) {
    lval* x;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
    if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

    FORLESS(t->children_num) {
        if (strcmp(t->children[i]->contents, "(") == 0) continue;
        if (strcmp(t->children[i]->contents, ")") == 0) continue;
        if (strcmp(t->children[i]->contents, "{") == 0) continue;
        if (strcmp(t->children[i]->contents, "}") == 0) continue;
        if (strcmp(t->children[i]->tag, "regex") == 0) continue;
        x = lval_add(x, lval_read(t->children[i]));
    }
    return x;
}
lval* lval_read(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) { return lval_check_num(t->contents); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    return lval_expr_read(t);
}

lval* buildin_op(lval* v, char* op) {
    FORLESS(v->count) {
        if (v->cell[i]->type != LVAL_NUM) {
            lval_del(v); return lval_err("buildin op operate on non-number!");
        }
    }
    lval* x = lval_pop(v, 0);
    if (v->count == 0 && strcmp(op, "-") == 0) {
        x->num = -x->num;
    }
    while (v->count) {
        lval* y = lval_pop(v, 0);
        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
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
lval* buildin_head(lval* v) {
    LASSERT_NUM("head", v, 1);
    LASSERT_TYPE("head", v, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", v, 0);
    lval* x = lval_take(v, 0);
    while (x->count > 1) { lval_del(lval_pop(x, 1));}
    return x;
}
lval* buildin_tail(lval* v) {
    LASSERT_NUM("tail", v, 1);
    LASSERT_TYPE("tail", v, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", v, 0);
    lval* x = lval_take(v, 0);
    lval_del(lval_pop(x, 0));
    return x;
}
lval* buildin_list(lval* v) {
    v->type = LVAL_QEXPR;
    return v;
}
lval* buildin_eval(lval* v) {
    LASSERT_NUM("eval", v, 1);
    LASSERT_TYPE("eval", v, 0, LVAL_QEXPR);
    lval* x = lval_take(v, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(x);
}
lval* buildin_join(lval* v) {
    FORLESS(v->count) {
        LASSERT_TYPE("join", v, i, LVAL_QEXPR);
    }
    lval* x = lval_pop(v, 0);
    while (v->count) {
        lval* y = lval_pop(v, 0);
        while (y->count) {
            x = lval_add(x, lval_pop(y, 0));
        }
        lval_del(y);
    }
    lval_del(v);
    return x;
}
lval* buildin(lval* v, char* op) {
    if (strcmp(op, "head") == 0) return buildin_head(v);
    if (strcmp(op, "tail") == 0) return buildin_tail(v);
    if (strcmp(op, "list") == 0) return buildin_list(v);
    if (strcmp(op, "eval") == 0) return buildin_eval(v);
    if (strcmp(op, "join") == 0) return buildin_join(v);
    if (strstr("+-*/", op)) return buildin_op(v, op);
    lval_del(v);
    return lval_err("Unknown function: %s", op);
}
lval* lval_expr_eval(lval* v) {
    FORLESS(v->count) {
        v->cell[i] = lval_eval(v->cell[i]);
    }
    FORLESS(v->count) {
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }
    if (v->count == 0) return v;
    if (v->count == 1) return lval_take(v, 0);

    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f); lval_del(v);
        return lval_err("S-Expression not start with Symbol!");
    }
    lval* res = buildin(v, f->sym);
//    lval* res = buildin_op(v, f->sym);
    lval_del(f);
    return res;
}
lval* lval_eval(lval* v) {
    if (v->type == LVAL_SEXPR) return lval_expr_eval(v);

    return v;
}


int main(int argc, char** argv) {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr  = mpc_new("sexpr");
    mpc_parser_t* Qexpr  = mpc_new("qexpr");
    mpc_parser_t* Expr   = mpc_new("expr");
    mpc_parser_t* Lispy  = mpc_new("lispy");
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                     \
      number : /-?[0-9]+/ ;                               \
      symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;        \
      sexpr  : '(' <expr>* ')' ;                          \
      qexpr  : '{' <expr>* '}' ;                          \
      expr   : <number> | <symbol> | <sexpr> | <qexpr> ;  \
      lispy  : /^/ <expr>* /$/ ;                          \
    ",
              Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
    puts("Lispy Version 0.0.0.0.8");
    puts("Press Ctrl+c to Exit\n");

//    lenv* e = lenv_new();
//    lenv_add_builtins(e);

    while (1) {

        char* input = readline("lispy> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval* res = lval_read(r.output);
//            lval* x = lval_eval(e, res);
            lval* x = lval_eval(res);
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);

    }

//    lenv_del(e);

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    FORLESS(v->count) {
        lval_print(v->cell[i]);
        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}
void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_ERR: printf("%s", v->err);
            break;
        case LVAL_NUM: printf("%i", v->num);
            break;
        case LVAL_SYM: printf("%s", v->sym);
            break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')');
            break;
        case LVAL_QEXPR: lval_expr_print(v, '{', '}');
            break;
    }
}
void lval_println(lval* v) { lval_print(v); putchar('\n'); }