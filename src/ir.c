#if _XOPEN_SOURCE < 500
#  undef _XOPEN_SOURCE
#  define _XOPEN_SOURCE 500 /* snprintf */
#endif
#include "ir.h"
#include "error.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

DEFINE_LIST_IMPLEMENTATION(sym_list, struct symbol *)

const char *mklabel(void)
{
    static int n;
    char *name = malloc(sizeof(char) * 16);
    snprintf(name, 12, ".L%d", n++);
    return name;
}

struct decl *cfg_create(void)
{
    struct decl *decl = calloc(1, sizeof(*decl));
    assert(decl);
    return decl;
}

struct block *cfg_block_init(struct decl *decl)
{
    struct block *block;

    assert(decl);
    block = calloc(1, sizeof(*block));
    block->label = mklabel();

    if (decl->size == decl->capacity) {
        decl->capacity += 16;
        decl->nodes =
            realloc(decl->nodes, decl->capacity * sizeof(*decl->nodes));
    }
    decl->nodes[decl->size++] = block;
    return block;
}

void cfg_finalize(struct decl *decl)
{
    assert(decl);
    if (decl->capacity) {
        int i;
        for (i = 0; i < decl->size; ++i) {
            struct block *block = decl->nodes[i];
            if (block->n) free(block->code);
            if (block->label) free((void *) block->label);
            free(block);
        }
        free(decl->nodes);
    }
    free(decl);
}

void cfg_ir_append(struct block *block, struct op op)
{
    /* Current block can be NULL when parsing an expression that should not be
     * evaluated, for example argument to sizeof. */
    if (block) {
        block->n += 1;
        block->code = realloc(block->code, sizeof(*block->code) * block->n);
        block->code[block->n - 1] = op;
    }
}

struct var var_direct(const struct symbol *sym)
{
    struct var var = {0};

    var.type = sym->type;
    if (sym->symtype == SYM_ENUM_VALUE) {
        var.kind = IMMEDIATE;
        var.value.integer = sym->enum_value;
    } else {
        var.kind = DIRECT;
        var.symbol = sym;
        var.lvalue = sym->name[0] != '.';
    }

    return var;
}

struct var var_string(const char *str)
{
    struct var var = {0};
    unsigned int length = strlen(str) + 1;

    var.kind = IMMEDIATE;
    var.type = type_init_string(length);
    var.string = str;
    return var;
}

struct var var_int(int value)
{
    struct var var = {0};

    var.kind = IMMEDIATE;
    var.type = type_init_integer(4);
    var.value.integer = value;
    return var;
}

struct var var_zero(int size)
{
    struct var var = {0};

    var.kind = IMMEDIATE;
    var.type = type_init_integer(size);
    var.value.integer = 0;
    return var;
}

struct var var_void(void)
{
    struct var var = {0};

    var.kind = IMMEDIATE;
    var.type = type_init_void();
    return var;
}

struct var create_var(const struct typetree *type)
{
    extern struct decl *decl;

    struct symbol *temp = sym_temp(&ns_ident, type);
    struct var res = var_direct(temp);

    sym_list_push_back(&decl->locals, temp);
    res.lvalue = 1;
    return res;
}