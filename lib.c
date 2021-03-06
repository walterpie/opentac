#include "include/opentac.h"
#include "grammar.tab.h"

extern FILE *yyin;
extern OpentacBuilder *opentac_b;

#define DEFAULT_BUILDER_CAP ((size_t) 32)
#define DEFAULT_FN_CAP ((size_t) 32)
#define DEFAULT_NAME_TABLE_CAP ((size_t) 32)
#define DEFAULT_PARAMS_CAP ((size_t) 4)

OpentacBuilder *opentac_parse(FILE *file) {
    opentac_assert(file);
    
    yyin = file;
    opentac_b = opentac_builderp();
    if (yyparse()) {
        return NULL;
    }
    return opentac_b;
}

void opentac_builder(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    opentac_builder_with_cap(builder, DEFAULT_BUILDER_CAP);
}

void opentac_builder_with_cap(OpentacBuilder *builder, size_t cap) {
    opentac_assert(builder);
    opentac_assert(cap > 0);
    
    builder->len = 0;
    builder->cap = cap;
    builder->items = malloc(cap * sizeof(OpentacItem *));
    builder->current = builder->items;
    builder->typeset.len = 0;
    builder->typeset.cap = cap;
    builder->typeset.types = malloc(cap * sizeof(OpentacType *));
}

OpentacBuilder *opentac_builderp() {
    return opentac_builderp_with_cap(DEFAULT_BUILDER_CAP);
}

OpentacBuilder *opentac_builderp_with_cap(size_t cap) {
    OpentacBuilder *builder = malloc(sizeof(OpentacBuilder));
    opentac_builder_with_cap(builder, cap);
    return builder;
}

static void opentac_grow_builder(OpentacBuilder *builder, size_t newcap) {
    opentac_assert(builder);
    opentac_assert(newcap >= builder->cap);
    
    builder->cap = newcap;
    builder->items = realloc(builder->items, builder->cap * sizeof(OpentacItem));
    builder->current = builder->items + builder->len;
}

static void opentac_grow_fn(OpentacBuilder *builder, size_t newcap) {
    opentac_assert(builder);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    
    opentac_assert(newcap >= fn->cap);
    
    fn->cap = newcap;
    fn->stmts = realloc(fn->stmts, fn->cap * sizeof(OpentacStmt));
    fn->current = fn->stmts + fn->len;
}

static void opentac_grow_name_table(OpentacFnBuilder *fn, size_t newcap) {
    opentac_assert(fn);
    opentac_assert(newcap >= fn->cap);
    
    fn->name_table.cap = newcap;
    fn->name_table.entries = realloc(fn->name_table.entries, fn->name_table.cap * sizeof(struct OpentacEntry));
}

static void opentac_grow_params(OpentacFnBuilder *fn, size_t newcap) {
    opentac_assert(fn);
    opentac_assert(newcap >= fn->cap);
    
    fn->params.cap = newcap;
    fn->params.params = realloc(fn->params.params, fn->params.cap * sizeof(OpentacType *));
}

static void opentac_grow_typeset(OpentacBuilder *builder, size_t newcap) {
    opentac_assert(builder);
    opentac_assert(newcap >= builder->typeset.cap);
    
    builder->typeset.cap = newcap;
    builder->typeset.types = realloc(builder->typeset.types, builder->typeset.cap * sizeof(OpentacType *));
}

void opentac_build_decl(OpentacBuilder *builder, OpentacString *name, OpentacType *type) {
    opentac_assert(builder);
    
    if ((size_t) (builder->current - builder->items) > builder->cap) {
        opentac_grow_builder(builder, builder->cap * 2);
    }
    
    OpentacItem *item = malloc(sizeof(OpentacItem));
    item->tag = OPENTAC_ITEM_DECL;
    item->decl.name = name;
    item->decl.type = type;
    *builder->current = item;
    ++builder->current;
    ++builder->len;
}

void opentac_build_function(OpentacBuilder *builder, OpentacString *name) {
    opentac_assert(builder);
    
    if ((size_t) (builder->current - builder->items) > builder->cap) {
        opentac_grow_builder(builder, builder->cap * 2);
    }

    size_t cap = DEFAULT_FN_CAP;
    OpentacItem *item = malloc(sizeof(OpentacItem));
    item->tag = OPENTAC_ITEM_FN;
    item->fn.name = name;
    item->fn.param = 0;
    item->fn.reg = 0;
    item->fn.len = 0;
    item->fn.cap = cap;
    item->fn.stmts = malloc(cap * sizeof(OpentacStmt));
    item->fn.current = item->fn.stmts;
    
    cap = DEFAULT_NAME_TABLE_CAP;
    item->fn.name_table.len = 0;
    item->fn.name_table.cap = cap;
    item->fn.name_table.entries = malloc(cap * sizeof(struct OpentacEntry));
    
    cap = DEFAULT_PARAMS_CAP;
    item->fn.params.len = 0;
    item->fn.params.cap = cap;
    item->fn.params.params = malloc(cap * sizeof(OpentacType *));
    
    *builder->current = item;
}

void opentac_finish_function(OpentacBuilder *builder) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    ++builder->current;
    ++builder->len;
}

void opentac_build_function_param(OpentacBuilder *builder, OpentacString *name, OpentacType *type) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    uint32_t param = --fn->param;
    opentac_fn_bind_int(builder, name, param);
    
    if (fn->params.len == fn->params.cap) {
        opentac_grow_params(fn, fn->params.cap * 2);
    }
    
    fn->params.params[fn->params.len++] = type;
}

void opentac_builder_insert(OpentacBuilder *builder, size_t index) {
    opentac_assert(builder);
    
    size_t remaining_len = builder->len - index;
    memmove(builder->items + index + 1, builder->items + index, sizeof(OpentacItem) * remaining_len);
    opentac_builder_goto(builder, index);
}

void opentac_builder_goto(OpentacBuilder *builder, size_t index) {
    opentac_assert(builder);
    
    builder->current = builder->items + index;
}

void opentac_builder_goto_end(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    opentac_builder_goto(builder, builder->len);
}

OpentacStmt *opentac_stmt_ptr(OpentacBuilder *builder) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);

    OpentacFnBuilder *fn = &(*builder->current)->fn;

    return fn->current;
}

OpentacValue opentac_build_binary(OpentacBuilder *builder, int opcode, OpentacValue left, OpentacValue right) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if ((size_t) (fn->current - fn->stmts) > fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    OpentacRegister target = fn->reg++;

    fn->current->tag.opcode = opcode;
    fn->current->tag.left = left.tag;
    fn->current->tag.right = right.tag;
    fn->current->left = left.val;
    fn->current->right = right.val;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;

    OpentacValue result;
    result.tag = OPENTAC_VAL_REG;
    result.val.regval = target;

    return result;
}

OpentacValue opentac_build_unary(OpentacBuilder *builder, int opcode, OpentacValue value) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if ((size_t) (fn->current - fn->stmts) > fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    OpentacRegister target = fn->reg++;

    fn->current->tag.opcode = opcode;
    fn->current->tag.left = value.tag;
    fn->current->left = value.val;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;

    OpentacValue result;
    result.tag = OPENTAC_VAL_REG;
    result.val.regval = target;

    return result;
}

void opentac_build_index_assign(OpentacBuilder *builder, OpentacRegister target, OpentacValue offset, OpentacValue value) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if ((size_t) (fn->current - fn->stmts) > fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    fn->current->tag.opcode = OPENTAC_OP_INDEX_ASSIGN;
    fn->current->tag.left = offset.tag;
    fn->current->tag.right = value.tag;
    fn->current->left = offset.val;
    fn->current->right = value.val;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;
}

OpentacValue opentac_build_assign_index(OpentacBuilder *builder, OpentacValue value, OpentacValue offset) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if ((size_t) (fn->current - fn->stmts) > fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    OpentacRegister target = fn->reg++;

    fn->current->tag.opcode = OPENTAC_OP_ASSIGN_INDEX;
    fn->current->tag.left = value.tag;
    fn->current->tag.right = offset.tag;
    fn->current->left = value.val;
    fn->current->right = offset.val;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;

    OpentacValue result;
    result.tag = OPENTAC_VAL_REG;
    result.val.regval = target;

    return result;
}

void opentac_build_param(OpentacBuilder *builder, OpentacValue value) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if ((size_t) (fn->current - fn->stmts) > fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    fn->current->tag.opcode = OPENTAC_OP_PARAM;
    fn->current->tag.left = value.tag;
    fn->current->left = value.val;
    ++fn->len;
    ++fn->current;
}

OpentacValue opentac_build_call(OpentacBuilder *builder, OpentacValue func, uint64_t nparams) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if ((size_t) (fn->current - fn->stmts) > fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    OpentacRegister target = fn->reg++;

    fn->current->tag.opcode = OPENTAC_OP_CALL;
    fn->current->tag.left = func.tag;
    fn->current->tag.right = OPENTAC_VAL_UI64;
    fn->current->left = func.val;
    fn->current->right.ui64val = nparams;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;

    OpentacValue result;
    result.tag = OPENTAC_VAL_REG;
    result.val.regval = target;

    return result;
}

void opentac_build_return(OpentacBuilder *builder, OpentacValue value) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if ((size_t) (fn->current - fn->stmts) > fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    fn->current->tag.opcode = OPENTAC_OP_RETURN;
    fn->current->tag.left = value.tag;
    fn->current->left = value.val;
    ++fn->len;
    ++fn->current;
}

void opentac_build_if_branch(OpentacBuilder *builder, int relop, OpentacValue left, OpentacValue right, OpentacLabel label) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    opentac_assert(relop >= OPENTAC_OP_LT && relop <= OPENTAC_OP_GE);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if ((size_t) (fn->current - fn->stmts) > fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    fn->current->tag.opcode = OPENTAC_OP_BRANCH | relop;
    fn->current->tag.left = left.tag;
    fn->current->tag.right = right.tag;
    fn->current->left = left.val;
    fn->current->right = right.val;
    fn->current->label = label;
    ++fn->len;
    ++fn->current;
}

void opentac_build_branch(OpentacBuilder *builder, OpentacValue value) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if ((size_t) (fn->current - fn->stmts) > fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    fn->current->tag.opcode = OPENTAC_OP_BRANCH | OPENTAC_OP_NOP;
    fn->current->tag.left = value.tag;
    fn->current->left = value.val;
    ++fn->len;
    ++fn->current;
}

void opentac_fn_insert(OpentacBuilder *builder, size_t index) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    size_t remaining_len = fn->len - index;
    memmove(fn->stmts + index + 1, fn->stmts + index, sizeof(OpentacStmt) * remaining_len);
    opentac_fn_goto(builder, index);
}

void opentac_fn_goto(OpentacBuilder *builder, size_t index) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    fn->current = fn->stmts + index;
}

void opentac_fn_goto_end(OpentacBuilder *builder) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    size_t len = (*builder->current)->fn.len;
    opentac_fn_goto(builder, len);
}

void opentac_fn_bind_int(OpentacBuilder *builder, OpentacString *name, uint32_t val) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->name_table.len == fn->name_table.cap) {
        opentac_grow_name_table(fn, fn->name_table.cap * 2);
    }
    
    fn->name_table.entries[fn->name_table.len].key = name;
    fn->name_table.entries[fn->name_table.len++].ival = val;
}

void opentac_fn_bind_ptr(OpentacBuilder *builder, OpentacString *name, void *val) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->name_table.len == fn->name_table.cap) {
        opentac_grow_name_table(fn, fn->name_table.cap * 2);
    }
    
    fn->name_table.entries[fn->name_table.len].key = name;
    fn->name_table.entries[fn->name_table.len++].pval = val;
}

uint32_t opentac_fn_get_int(OpentacBuilder *builder, OpentacString *name) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    for (size_t i = 0; i < fn->name_table.len; i++) {
        if (strcmp(fn->name_table.entries[i].key->data, name->data) == 0) {
            return fn->name_table.entries[i].ival;
        }
    }

    return -1;
}

void *opentac_fn_get_ptr(OpentacBuilder *builder, OpentacString *name) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    for (size_t i = 0; i < fn->name_table.len; i++) {
        if (strcmp(fn->name_table.entries[i].key->data, name->data) == 0) {
            return fn->name_table.entries[i].pval;
        }
    }

    return NULL;
}

#define BASIC_TYPE_FN(t) \
    for (size_t i = 0; i < builder->typeset.len; i++) { \
        OpentacType *type = builder->typeset.types[i]; \
        if (type->tag == t) { \
            return type; \
        } \
    } \
\
    if (builder->typeset.len == builder->typeset.cap) { \
        opentac_grow_typeset(builder, builder->typeset.cap * 2); \
    } \
\
    OpentacType *type = malloc(sizeof(OpentacType)); \
    type->tag = t; \
    builder->typeset.types[builder->typeset.len++] = type; \
    return type;

OpentacType *opentac_type_unit(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_UNIT)
}

OpentacType *opentac_type_never(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_NEVER)
}

OpentacType *opentac_type_bool(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_BOOL)
}

OpentacType *opentac_type_i8(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_I8)
}

OpentacType *opentac_type_i16(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_I16)
}

OpentacType *opentac_type_i32(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_I32)
}

OpentacType *opentac_type_i64(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_I64)
}

OpentacType *opentac_type_ui8(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_UI8)
}

OpentacType *opentac_type_ui16(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_UI16)
}

OpentacType *opentac_type_ui32(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_UI32)
}

OpentacType *opentac_type_ui64(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_UI64)
}

OpentacType *opentac_type_f32(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_F32)
}

OpentacType *opentac_type_f64(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_F64)
}

OpentacType *opentac_type_named(OpentacBuilder *builder, int tag, OpentacString *name) {
    opentac_assert(builder);
    opentac_assert(name);
    opentac_assert(tag == OPENTAC_TYPE_STRUCT || tag == OPENTAC_TYPE_UNION);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType *type = builder->typeset.types[i];
        if (type->tag == tag && strcmp(type->struc.name->data, name->data)) {
            opentac_del_string(name);
            return type;
        }
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = tag;
    type->struc.name = name;
    type->struc.len = 0;
    type->struc.elems = NULL;
    builder->typeset.types[builder->typeset.len++] = type;

    return type;
}

OpentacType *opentac_type_ptr(OpentacBuilder *builder, OpentacType *pointee) {
    opentac_assert(builder);
    opentac_assert(pointee);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType *type = builder->typeset.types[i];
        if (type->tag == OPENTAC_TYPE_PTR && type->ptr.pointee == pointee) {
            return type;
        }
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_PTR;
    type->ptr.pointee = pointee;
    builder->typeset.types[builder->typeset.len++] = type;

    return type;
}

OpentacType *opentac_type_fn(OpentacBuilder *builder, size_t len, OpentacType **params, OpentacType *result) {
    opentac_assert(builder);
    opentac_assert(params);
    opentac_assert(result);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType *type = builder->typeset.types[i];
        if (type->tag == OPENTAC_TYPE_FN && type->fn.result == result && type->fn.len == len) {
            for (size_t j = 0; j < type->fn.len; j++) {
                OpentacType *param = type->fn.params[j];
                if (param != params[j]) {
                    goto cont;
                }
            }
            free(params);
            return type;
        }
cont:
        (void) 0;
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_FN;
    type->fn.result = result;
    type->fn.len = len;
    type->fn.params = params;
    builder->typeset.types[builder->typeset.len++] = type;

    return type;
}

OpentacType *opentac_type_tuple(OpentacBuilder *builder, size_t len, OpentacType **elems) {
    opentac_assert(builder);
    opentac_assert(elems);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType *type = builder->typeset.types[i];
        if (type->tag == OPENTAC_TYPE_TUPLE && type->tuple.len == len) {
            for (size_t j = 0; j < type->tuple.len; j++) {
                OpentacType *elem = type->tuple.elems[j];
                if (elem != elems[j]) {
                    goto cont;
                }
            }
            free(elems);
            return type;
        }
cont:
        (void) 0;
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_TUPLE;
    type->tuple.len = len;
    type->tuple.elems = elems;
    builder->typeset.types[builder->typeset.len++] = type;

    return type;
}

OpentacType *opentac_type_struct(OpentacBuilder *builder, OpentacString *name, size_t len, OpentacType **elems) {
    opentac_assert(builder);
    opentac_assert(name);
    opentac_assert(elems);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType *type = builder->typeset.types[i];
        if (type->tag == OPENTAC_TYPE_STRUCT && strcmp(type->struc.name->data, name->data)) {
            opentac_del_string(name);
            type->struc.len = len;
            type->struc.elems = elems;
            return type;
        }
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_STRUCT;
    type->struc.name = name;
    type->struc.len = len;
    type->struc.elems = elems;
    builder->typeset.types[builder->typeset.len++] = type;

    return type;
}

OpentacType *opentac_type_union(OpentacBuilder *builder, OpentacString *name, size_t len, OpentacType **elems) {
    opentac_assert(builder);
    opentac_assert(name);
    opentac_assert(elems);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType *type = builder->typeset.types[i];
        if (type->tag == OPENTAC_TYPE_UNION && strcmp(type->struc.name->data, name->data)) {
            opentac_del_string(name);
            type->struc.len = len;
            type->struc.elems = elems;
            return type;
        }
    }
    
    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_UNION;
    type->struc.name = name;
    type->struc.len = len;
    type->struc.elems = elems;
    builder->typeset.types[builder->typeset.len++] = type;

    return type;
}

OpentacType *opentac_type_array(OpentacBuilder *builder, OpentacType *elem_type, uint64_t len) {
    opentac_assert(builder);
    opentac_assert(elem_type);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType *type = builder->typeset.types[i];
        if (type->tag == OPENTAC_TYPE_ARRAY && type->array.elem_type == elem_type && type->array.len == len) {
            return type;
        }
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_ARRAY;
    type->array.elem_type = elem_type;
    type->array.len = len;
    builder->typeset.types[builder->typeset.len++] = type;

    return type;
}

OpentacString *opentac_string(const char *str) {
    opentac_assert(str);
    
    size_t len = strlen(str);
    size_t cap = len + 1;
    OpentacString *string = malloc(sizeof(OpentacString) + cap);
    string->len = len;
    string->cap = cap;
    strcpy(string->data, str);
    return string;
}

void opentac_del_string(OpentacString *str) {
    free(str);
}
