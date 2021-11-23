
#ifndef QBN_QBN_H
#define QBN_QBN_H

#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#include "op.h"
#include "limits.h"
#include "util/vector.h"

void qbn_error(char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

#define QBN_UNREACHABLE qbn_error("You shouldn't be here...");
#define QBN_NOT_IMPLEMENTED qbn_error("Work in progress!\n");

typedef unsigned long QbnRef;
typedef struct QbnDataItem QbnDataItem;
typedef struct QbnTemp QbnTemp;
typedef struct QbnConst QbnConst;
typedef struct QbnPhi QbnPhi;
typedef struct QbnInstr QbnInstr;
typedef struct QbnBlock QbnBlock;
typedef struct QbnFn QbnFn;
typedef struct QbnContext QbnContext;

typedef enum {
    QBN_JUMP_RET_NONE = 0,
    QBN_JUMP_RET_BASE = 1,
    QBN_JUMP_RET_STRUCT = 2,
    QBN_JUMP_RET_END = 3,  // upper boundary to do is_return check
    QBN_JUMP_UNCONDITIONAL,
    QBN_JUMP_NZ,
    QBN_JUMP_FI_EQ,
    QBN_JUMP_FI_NE,
    QBN_JUMP_IS_GE,
    QBN_JUMP_IS_GT,
    QBN_JUMP_IS_LE,
    QBN_JUMP_IS_LT,
    QBN_JUMP_IU_GE,
    QBN_JUMP_IU_GT,
    QBN_JUMP_IU_LE,
    QBN_JUMP_IU_LT,
    QBN_JUMP_FF_EQ,
    QBN_JUMP_FF_GE,
    QBN_JUMP_FF_GT,
    QBN_JUMP_FF_LE,
    QBN_JUMP_FF_LT,
    QBN_JUMP_FF_NE,
    QBN_JUMP_FF_O,   // ordered, both not NaN
    QBN_JUMP_FF_UO   // unordered, at least one NaN
} QbnJumpType;

#define QBN_IS_RETURN(jmp_type) ((jmp_type) < QBN_JUMP_RET_END)

typedef enum {
    QBN_SEC_NONE, QBN_SEC_DATA, QBN_SEC_TEXT
} QbnSection;

typedef enum {
    QBN_REF_NONE,
    QBN_REF_REG,
    QBN_REF_TEMP,
    QBN_REF_CONST,
} QbnRefType;

// QbnRef
#define QBN_REF_TYPE(ref) ((QbnRefType) ((ref) >> 32))
#define QBN_REF_TYPE_SET(ref, type) (((unsigned long)(ref) & 0x00000000FFFFFFFF) | ((unsigned long)(type) << 32))
#define QBN_REF_INDEX(ref) ((unsigned int) (ref))
#define QBN_REF_INDEX_SET(ref, index) (((unsigned long)(ref) & 0xFFFFFFFF00000000) | (index))

#define QBN_REF0 QBN_REF_TYPE_SET(0, QBN_REF_NONE)
#define QBN_TEMP_REF(index) QBN_REF_INDEX_SET(QBN_REF_TYPE_SET(0, QBN_REF_TEMP), (index))
#define QBN_CONST_REF(index) QBN_REF_INDEX_SET(QBN_REF_TYPE_SET(0, QBN_REF_CONST), (index))
#define QBN_REG_REF(reg) QBN_REF_INDEX_SET(QBN_REF_TYPE_SET(0, QBN_REF_REG), (reg))

struct QbnDataItem {
    union {
        struct {
            const char* name;
            char export;  // bool in union is bad
        } start;

        struct {
            // TODO: maybe refactor to Reference? ref_type checking needed then
            const char* name;
            long offset;
            QbnType ext_type;
        } global_ref;

        struct {
            union {
                long i;
                float f32;
                double f64;
            } value;
            QbnType ext_type;
        } number;

        long align_length;
        long zero_length;
        const char* string;
        QbnDataItem* next;
    } value;

    enum {
        QBN_DATA_START,
        QBN_DATA_ALIGN,        // for opaque types
        QBN_DATA_ZERO,         // zero init
        QBN_DATA_REF_DATA,
        QBN_DATA_REF_FUNC,
        QBN_DATA_STRING,
        QBN_DATA_CONSTANT,
        QBN_DATA_END,
        QBN_DATA_NEXT_VEC_BLOCK
    } type;
};

struct QbnFn {
    QbnContext* context;
    QbnBaseType return_type;
    char* name;
    QbnBlock* start;
    UtilVector* vec_temps;
    QbnTemp* temps;
    UtilVector* vec_blocks;
    QbnBlock** blocks;
    int n_float_regs_used;
    int n_int_regs_used;
    unsigned long frame_size;
    unsigned char stack_alignment;
    bool export;
};

struct QbnInstr {
    QbnRef arg0;
    QbnRef arg1;
    QbnRef to;
    QbnOp op;
    QbnBaseType type;
};

struct QbnBlock {
    QbnPhi* phi;
    QbnInstr* instr;
    QbnJumpType jmp_type;
    union {
        struct {
            QbnBlock* True;
            QbnBlock* False;
        } dest;
        struct {
            QbnBaseType type;
            QbnRef value;
        } ret;
    } jmp;
};

struct QbnPhi {

};

struct QbnTemp {
    QbnExtType type;
    QbnRef slot;  // TODO either a register or slot on stack
};

struct QbnConst {
    enum {
        QBN_CONST_NUMBER,
        QBN_CONST_F32,
        QBN_CONST_F64,
        QBN_CONST_ADDR,
        QBN_CONST_NAME,
    } type;
    union {
        long number;
        float f32;
        double f64;
        char* label;
    } value;
};

struct QbnContext {
    QbnType size_type;
    QbnSection current_section;
    bool data_is_aligned;
    QbnInstr* instr_cache;
    QbnInstr* current_instr;
    QbnDataItem* data;
    QbnDataItem* data_next;
    int data_count;
    UtilVector* vec_functions;
    QbnFn** functions;
    UtilVector* vec_consts;
    QbnConst* consts;
};

const char* qbn_type2s[] = {
        [QBN_TYPE_I32] = "i32",
        [QBN_TYPE_I64] = "i64",
        [QBN_TYPE_F32] = "f32",
        [QBN_TYPE_F64] = "f64",
        [QBN_TYPE_I8] = "i8",
        [QBN_TYPE_I16] = "i16",
};

void run_example();

QbnInstr* qbn_block_last_instr(QbnBlock* block) {
    // returns the block's last instruction which should be empty
    QbnInstr* instr = block->instr;
    while (instr->op != QBN_OP_BLOCK_END) {
        instr++;
    }
    instr--;
    // TODO: remove
    assert(instr > block->instr);
    return instr;
}

unsigned int qbn_context_add_const(QbnContext* context, QbnConst con) {
    size_t i = context->vec_consts->length;
    util_vector_grow(context->vec_consts, 1);
    context->consts[i] = con;
    return (unsigned int) i;
}

QbnRef qbn_context_new_label(QbnContext* context, char* label, int const_type) {
    // constant containing a string label that
    // may or may not refer to another object (data, function, ...)
    unsigned int index = qbn_context_add_const(context, (QbnConst) {.type = const_type, .value.label = label});
    return QBN_REF_TYPE_SET(QBN_REF_INDEX_SET(QBN_REF0, index), QBN_REF_CONST);
}

QbnRef qbn_context_new_data_ref(QbnContext* context, char* label) {
    return qbn_context_new_label(context, label, QBN_CONST_ADDR);
}

QbnRef qbn_context_new_name_ref(QbnContext* context, char* label) {
    return qbn_context_new_label(context, label, QBN_CONST_NAME);
}

QbnRef qbn_context_new_const_number(QbnContext* context, long number) {
    unsigned int index = qbn_context_add_const(context, (QbnConst) {QBN_CONST_NUMBER, .value.number = number});
    return QBN_REF_TYPE_SET(QBN_REF_INDEX_SET(QBN_REF0, index), QBN_REF_CONST);
}

void qbn_data_next_block(QbnContext* context) {
    QbnDataItem* block = malloc(sizeof(QbnDataItem) * 64);
    for (int i=0; i<63; i++) {
        block[i].type = QBN_DATA_END;
    }
    block[63].type = QBN_DATA_NEXT_VEC_BLOCK;
    block[63].value.next = NULL;
    if (context->data_next != NULL) {
        assert(context->data_next->type == QBN_DATA_NEXT_VEC_BLOCK);
        context->data_next->value.next = block;
    } else {
        context->data = block;
    }
    context->data_next = block;
}

void qbn_data_add_item(QbnContext* context, QbnDataItem item) {
    if (context->data_next->type == QBN_DATA_NEXT_VEC_BLOCK) {
        qbn_data_next_block(context);
    }
    *context->data_next = item;
    context->data_next++;
}

void qbn_data_new(QbnContext* context, const char* name, char export) {
    qbn_data_add_item(context, (QbnDataItem){.value.start = {name, export}, .type = QBN_DATA_START});
}

void qbn_data_end(QbnContext* context) {
    qbn_data_add_item(context, (QbnDataItem){.type = QBN_DATA_END});
    context->data_count++;
}

void qbn_data_new_cstring(QbnContext* context, const char* name, const char* string, char export) {
    qbn_data_new(context, name, export);
    qbn_data_add_item(context, (QbnDataItem){.type = QBN_DATA_STRING, .value.string = string});
    qbn_data_add_item(context, (QbnDataItem){.type = QBN_DATA_CONSTANT, .value.number = {.ext_type = QBN_TYPE_I8, .value.i = 0}});
    qbn_data_end(context);
}

void qbn_context_add_instr(QbnContext* context, QbnOp op, QbnRef arg0, QbnRef arg1, QbnRef to, QbnBaseType type) {
    *context->current_instr = (QbnInstr) {
            .to = to,
            .type = type,
            .op = op,
            .arg0 = arg0,
            .arg1 = arg1
    };
    context->current_instr++;
}

void qbn_context_copy_instr(QbnContext* context, QbnInstr instr) {
    *context->current_instr = instr;
    context->current_instr++;
}

void qbn_context_block_end(QbnContext* context) {
    context->current_instr->op = QBN_OP_BLOCK_END;
    context->current_instr++;
}

void qbn_fn_add_parameter(QbnFn* fn, QbnBaseType type) {
    // TODO
}

void qbn_fn_add_instr(QbnFn* fn, QbnOp op, QbnRef arg0, QbnRef arg1, QbnRef to, QbnBaseType type) {
    qbn_context_add_instr(fn->context, op, arg0, arg1, to, type);
}

QbnTemp* qbn_fn_new_temp(QbnFn* fn, QbnExtType type) {
    util_vector_grow(fn->vec_temps, 1);
    QbnTemp* temp = &fn->temps[fn->vec_temps->length - 1];
    *temp = (QbnTemp){
            .type = type,
            .slot = 0
    };
    return temp;
}

QbnBlock* qbn_fn_next_block(QbnFn* fn) {
    QbnBlock* block = malloc(sizeof(QbnBlock));
    block->instr = fn->context->current_instr;
    block->phi = NULL;
    util_vector_grow(fn->vec_blocks, 1);
    fn->blocks[fn->vec_blocks->length-1] = block;
    if (!fn->start) {
        fn->start = block;
    }
    return block;
}

void qbn_fn_close_block(QbnFn* fn) {
    qbn_context_block_end(fn->context);
}

void qbn_block_jump(QbnContext* context, QbnBlock* block, QbnJumpType jump_type, QbnBlock* True, QbnBlock* False) {
    QBN_NOT_IMPLEMENTED
}

void qbn_fn_block_return(QbnFn* fn, QbnBlock* block, QbnBaseType type, QbnRef value) {
    qbn_fn_close_block(fn);
    block->jmp_type = value ? QBN_JUMP_RET_BASE : QBN_JUMP_RET_NONE;
    block->jmp.ret.type = type;
    block->jmp.ret.value = value;
}

QbnFn* qbn_context_new_fn(QbnContext* context, QbnBaseType return_type, char* name, bool export) {
    QbnFn* fn = malloc(sizeof(QbnFn));
    fn->context = context;
    fn->export = export;
    fn->return_type = return_type;
    fn->name = name;
    fn->vec_temps = util_vector_new(sizeof(QbnTemp), 0, (void**) &fn->temps);
    fn->vec_blocks = util_vector_new(sizeof(QbnBlock*), 20, (void**) &fn->blocks);
    util_vector_grow(context->vec_functions, 1);
    context->functions[context->vec_functions->length-1] = fn;
    fn->start = NULL;
    fn->n_int_regs_used = 0;
    fn->n_float_regs_used = 0;
    return fn;
}

QbnContext* qbn_context_new() {
    QbnContext* context = malloc(sizeof(QbnContext));
    context->size_type = QBN_TYPE_I64;
    context->current_section = QBN_SEC_NONE;
    context->data_is_aligned = false;
    context->instr_cache = malloc(sizeof(QbnInstr) * QBN_LIMIT_INSTR_CACHE);
    context->current_instr = context->instr_cache;
    context->data = NULL;
    context->data_next = NULL;
    context->data_count = 0;
    qbn_data_next_block(context);
    context->vec_functions = util_vector_new(sizeof(QbnFn*), 20, (void**) &context->functions);
    context->vec_consts = util_vector_new(sizeof(QbnConst), 0, (void**) &context->consts);
    return context;
}

void qbn_context_reset(QbnContext* context) {
    context->current_section = QBN_SEC_NONE;
    context->data_is_aligned = false;
    context->current_instr = context->instr_cache;

    QbnDataItem* block = context->data;
    QbnDataItem* current_data_item = context->data;
    while (current_data_item != context->data_next) {
        if (current_data_item->type == QBN_DATA_NEXT_VEC_BLOCK) {
            current_data_item = current_data_item->value.next;
            free(block);
            block = current_data_item;
        } else {
            current_data_item++;
        }
    }
    context->data = NULL;
    context->data_next = NULL;
    context->data_count = 0;
    qbn_data_next_block(context);

    for (int i=0; i<context->vec_functions->length; i++) {
        free(context->functions[i]);
    }
    util_vector_clear(context->vec_functions);
    util_vector_clear(context->vec_consts);
}

#endif //QBN_QBN_H
