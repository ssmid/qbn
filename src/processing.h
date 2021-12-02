
#ifndef QBN_PROCESSING_H
#define QBN_PROCESSING_H

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include "qbn.h"
#include "util/std.h"

typedef enum {
    QBN_RAX = 1, /* caller-saved */
    QBN_RCX,
    QBN_RDX,
    QBN_RSI,
    QBN_RDI,
    QBN_R8,
    QBN_R9,
    QBN_R10,
    QBN_R11,

    QBN_RBX, /* callee-saved */
    QBN_R12,
    QBN_R13,
    QBN_R14,
    QBN_R15,

    QBN_RBP, /* globally live */
    QBN_RSP,

    QBN_XMM0, /* sse */
    QBN_XMM1,
    QBN_XMM2,
    QBN_XMM3,
    QBN_XMM4,
    QBN_XMM5,
    QBN_XMM6,
    QBN_XMM7,

    QBN_XMM8,
    QBN_XMM9,
    QBN_XMM10,
    QBN_XMM11,
    QBN_XMM12,
    QBN_XMM13,
    QBN_XMM14,
    QBN_XMM15,

    QBN_REG_COUNT

} QbnAmd64Register;

const int QBN_TEMP0 = QBN_REG_COUNT;
const int QBN_NFPR = QBN_XMM14 - QBN_XMM0 + 1; /* reserve XMM15 */
const int QBN_NGPR = QBN_RSP - QBN_RAX + 1;
const int QBN_NGPS = QBN_R11 - QBN_RAX + 1;
const int QBN_NFPS = QBN_NFPR;
const int QBN_NCLR = QBN_R15 - QBN_RBX + 1;

const QbnAmd64Register QBN_REG_INT[] = {
        QBN_RDI, QBN_RSI, QBN_RDX, QBN_RCX, QBN_R8, QBN_R9, QBN_RAX, QBN_R10, QBN_R11,
        QBN_RBX, QBN_R12, QBN_R13, QBN_R14, QBN_R15
};
const int QBN_REG_INT_COUNT = 14;
const int QBN_REG_CALLER_SAVED_START = 0;
const int QBN_REG_CALLER_SAVED_END = 9;
const int QBN_REG_CALLEE_SAVED_START = 9;
const int QBN_REG_CALLEE_SAVED_END = 14;
const int QBN_REG_ARG_INT_START = 0;
const int QBN_REG_ARG_INT_END = 6;

const QbnAmd64Register QBN_REG_FLOAT[] = {
        QBN_XMM0, QBN_XMM1, QBN_XMM2,  QBN_XMM3,  QBN_XMM4,  QBN_XMM5,  QBN_XMM6,  QBN_XMM7,
        QBN_XMM8, QBN_XMM9, QBN_XMM10, QBN_XMM11, QBN_XMM12, QBN_XMM13, QBN_XMM14, QBN_XMM15
};
const int QBN_REG_FLOAT_COUNT = 16;
const int QBN_REG_ARG_FLOAT_COUNT = 8;

typedef struct {
    char nmem;
    bool zflag;
    bool lflag;
} QbnAmd64Operation;

void qbn_add_stack(QbnFn* fn, unsigned char count) {
    fn->stack_alignment += count;
    fn->stack_alignment %= 16;
}

void qbn_add_push(QbnFn* fn, QbnRef reg, QbnType type) {
    qbn_context_add_instr(fn->context, QBN_OP_PUSH, reg, QBN_REF0, QBN_REF0, type);
    qbn_add_stack(fn, QBN_TYPE_INFO[type].bytes);
}

void qbn_add_pop(QbnFn* fn, QbnRef reg, QbnType type) {
    qbn_context_add_instr(fn->context, QBN_OP_POP, QBN_REF0, QBN_REF0, reg, type);
    qbn_add_stack(fn, -QBN_TYPE_INFO[type].bytes);
}

void qbn_amd64_sysv_save_caller_regs_move(QbnFn* fn) {
    int end = MIN(QBN_REG_CALLER_SAVED_END, fn->rega_n_int_regs_used);
    for (int i=QBN_REG_CALLER_SAVED_START; i<end; i++) {
        qbn_add_push(fn, QBN_REG_REF(QBN_REG_INT[i]), fn->context->size_type);
    }
}

void qbn_amd64_sysv_restore_caller_regs_move(QbnFn* fn) {
    int end = MIN(QBN_REG_CALLER_SAVED_END, fn->rega_n_int_regs_used);
    for (int i=end-1; i>=QBN_REG_CALLER_SAVED_START; i--) {
        qbn_add_pop(fn, QBN_REG_REF(QBN_REG_INT[i]), fn->context->size_type);
    }
}

void qbn_amd64_sysv_save_callee_regs_move(QbnFn* fn) {
    for (int i=QBN_REG_CALLEE_SAVED_START; i<fn->rega_n_int_regs_used; i++) {
        qbn_add_push(fn, QBN_REG_REF(QBN_REG_INT[i]), fn->context->size_type);
    }
}

void qbn_amd64_sysv_restore_callee_regs_move(QbnFn* fn) {
    for (int i=fn->rega_n_int_regs_used-1; i>=QBN_REG_CALLEE_SAVED_START; i--) {
        qbn_add_pop(fn, QBN_REG_REF(QBN_REG_INT[i]), fn->context->size_type);
    }
}

void qbn_amd64_sysv_copy(QbnContext* context, QbnBlock* block, QbnInstr* copy) {
    QbnConst* con;
    switch (QBN_REF_TYPE(copy->arg0)) {
        case QBN_REF_CONST:
            con = &context->consts[QBN_REF_INDEX(copy->arg0)];
            if (con->type == QBN_CONST_ADDR) {
                copy->op = QBN_OP_ADDR;
            }
            break;
        default:;
    }
}

void qbn_amd64_sysv_call_move(QbnFn* fn, QbnBlock* block, QbnInstr* instr) {
    qbn_amd64_sysv_save_caller_regs_move(fn);

    int n_int_args = 0;
    int n_float_args = 0;
    QbnInstr* new_instr;
    while (instr->op == QBN_OP_ARG) {
        switch (instr->type) {
            case QBN_TYPE_F32:
            case QBN_TYPE_F64:
                // TODO: support stack args
                assert(n_float_args < QBN_REG_ARG_FLOAT_COUNT);
                new_instr = fn->context->current_instr;
                qbn_context_add_instr(fn->context, QBN_OP_COPY, instr->arg0, QBN_REF0, QBN_REG_REF(QBN_REG_FLOAT[n_float_args]), instr->type);
                n_float_args++;
                break;
            default:// TODO: support stack args
                assert(n_int_args < QBN_REG_ARG_INT_END - QBN_REG_ARG_INT_START);
                new_instr = fn->context->current_instr;
                qbn_context_add_instr(fn->context, QBN_OP_COPY, instr->arg0, QBN_REF0, QBN_REG_REF(QBN_REG_INT[QBN_REG_ARG_INT_START + n_int_args]), instr->type);
                n_int_args++;
        }
        qbn_amd64_sysv_copy(fn->context, block, new_instr);
        instr++;
        if (instr->op != QBN_OP_ARG) {
            break;
        }
    }
    assert(instr->op == QBN_OP_CALL);
    if (fn->stack_alignment) {
        QbnRef stack_adjustment = qbn_context_new_const_number(fn->context, 16 - fn->stack_alignment);
        qbn_context_add_instr(fn->context, QBN_OP_SUB, stack_adjustment, QBN_REF0, QBN_REG_REF(QBN_RSP), fn->context->size_type);
        qbn_context_copy_instr(fn->context, *instr);
        qbn_context_add_instr(fn->context, QBN_OP_ADD, stack_adjustment, QBN_REF0, QBN_REG_REF(QBN_RSP), fn->context->size_type);
    } else {
        qbn_context_copy_instr(fn->context, *instr);
    }

    qbn_amd64_sysv_restore_caller_regs_move(fn);
}

void qbn_amd64_sysv_return_move(QbnFn* fn, QbnBlock* block) {
    QbnInstr* instr;
    qbn_amd64_sysv_restore_callee_regs_move(fn);
    switch (block->jmp_type) {
        case QBN_JUMP_RET_BASE:
            instr = fn->context->current_instr;
            qbn_context_add_instr(fn->context, QBN_OP_COPY, QBN_REF0, QBN_REF0, QBN_REG_REF(QBN_RAX), block->jmp.ret.type);
            qbn_amd64_sysv_copy(fn->context, block, instr);
            switch (QBN_REF_TYPE(block->jmp.ret.value)) {
                case QBN_REF_CONST:
                    instr->arg0 = QBN_CONST_REF(block->jmp.ret.value);
                    break;
                case QBN_REF_TEMP:
                    instr->arg0 = QBN_TEMP_REF(block->jmp.ret.value);
                    break;
                case QBN_REF_NONE:
                    instr->arg0 = QBN_REF0;
                    break;
                default:
                    QBN_NOT_IMPLEMENTED
            }
            break;
        case QBN_JUMP_RET_NONE:
            break;
        default:
            QBN_NOT_IMPLEMENTED
    }
}

void qbn_amd64_sysv_block(QbnFn* fn, QbnBlock* block) {
    QbnInstr* instr_old = block->instr;
    while (instr_old->op != QBN_OP_BLOCK_END) {
        QbnInstr* instr_new;
        switch (instr_old->op) {
            case QBN_OP_PAR:
                break;
            case QBN_OP_ARG:
            case QBN_OP_CALL:
                qbn_amd64_sysv_call_move(fn, block, instr_old);
                while (instr_old->op == QBN_OP_ARG) {
                    instr_old++;
                }
                break;
            case QBN_OP_COPY:
                instr_new = fn->context->current_instr;
                qbn_context_copy_instr(fn->context, *instr_old);
                qbn_amd64_sysv_copy(fn->context, block, instr_new);
                break;
            default:
                qbn_context_copy_instr(fn->context, *instr_old);
        }
        instr_old++;
    }
    if (QBN_IS_RETURN(block->jmp_type)) {
        if (block->jmp_type != QBN_JUMP_RET_NONE) {
            qbn_amd64_sysv_return_move(fn, block);
        }
    }
    qbn_context_block_end(fn->context);
}

void qbn_amd64_sysv_abi(QbnFn* fn) {
    assert(fn->vec_blocks->length);
    fn->stack_alignment = 0;

    QbnInstr* instr_new = fn->context->current_instr;
    qbn_amd64_sysv_save_callee_regs_move(fn);
    // TODO: select parameters

    int i = 0;
    while (true) {
        QbnBlock* block = fn->blocks[i];
        qbn_amd64_sysv_block(fn, block);
        block->instr = instr_new;
        i++;
        if (i >= fn->vec_blocks->length) {
            break;
        }
        instr_new = fn->context->current_instr;
    }
}

QbnRef qbn_amd64_temp_to_ref(QbnFn* fn, QbnRef temp_ref) {
    assert(QBN_REF_TYPE(temp_ref) == QBN_REF_TEMP);
    QbnTemp* temp = &fn->temps[QBN_REF_INDEX(temp_ref)];
    assert(temp->slot != QBN_REF0);
    return temp->slot;
}

void qbn_amd64_basic_reg_allocation(QbnFn* fn) {
    fn->rega_n_int_regs_used = 0;
    fn->rega_n_float_regs_used = 0;
    for (int i=0; i<fn->vec_blocks->length; i++) {
        // allocates registers in order of calling convention
        // function parameters rely on this fact
        for (QbnInstr* instr = fn->blocks[i]->instr; instr->op != QBN_OP_BLOCK_END; instr++) {
            if (instr->op != QBN_OP0) {
                if (instr->to != QBN_REF0) {
                    assert(QBN_REF_TYPE(instr->to) == QBN_REF_TEMP);
                    QbnTemp *temp = &fn->temps[QBN_REF_INDEX(instr->to)];
                    if (temp->slot == QBN_REF0) {
                        // allocate a register
                        // TODO: stack temporaries
                        if (temp->type == QBN_ETYPE_F32 || temp->type == QBN_ETYPE_F64) {
                            // float
                            assert(fn->rega_n_float_regs_used < QBN_REG_FLOAT_COUNT);
                            temp->slot = QBN_REG_REF(QBN_REG_FLOAT[fn->rega_n_float_regs_used]);
                            fn->rega_n_float_regs_used++;
                        } else {
                            // int
                            assert(fn->rega_n_int_regs_used < QBN_REG_INT_COUNT);
                            temp->slot = QBN_REG_REF(QBN_REG_INT[fn->rega_n_int_regs_used]);
                            fn->rega_n_int_regs_used++;
                        }
                    }
                }
            }
        }
    }
}

void qbn_process(QbnContext* context) {
    for (int i=0; i<context->vec_functions->length; i++) {
        QbnFn* fn = context->functions[i];
        qbn_amd64_basic_reg_allocation(fn);
        qbn_print_all(stderr, fn->context, "after reg allocation");
        qbn_amd64_sysv_abi(fn);
    }
}

const char* QBN_GAS_INDENT = "    ";

const char* QBN_TYPE2GAS[] = {
        [QBN_TYPE_I32] = "int",
        [QBN_TYPE_I64] = "quad",
        [QBN_TYPE_F32] = "int",
        [QBN_TYPE_F64] = "quad",
        [QBN_TYPE_I8] = "byte",
        [QBN_TYPE_I16] = "short",
};

const char QBN_TYPE2GASSUFFIX[] = {
        [QBN_TYPE_I32] = 'l',  // long
        [QBN_TYPE_I64] = 'q',  // quadword
        [QBN_TYPE_F32] = 'l',  // long
        [QBN_TYPE_F64] = 'q',  // quadword
        [QBN_TYPE_I8] = 'b',   // byte
        [QBN_TYPE_I16] = 'w',  // word
};

const char* QBN_AMD64_REG2GAS_TABLE[][4] = {
        [QBN_RAX  ] = {"al", "ax", "eax", "rax"},
        [QBN_RBX  ] = {"bl", "bx", "ebx", "rbx"},
        [QBN_RCX  ] = {"cl", "cx", "ecx", "rcx"},
        [QBN_RDX  ] = {"dl", "dx", "edx", "rdx"},
        [QBN_RSI  ] = {"sil", "si", "esi", "rsi"},
        [QBN_RDI  ] = {"dil", "di", "edi", "rdi"},
        [QBN_RBP  ] = {"bpl", "bp", "ebp", "rbp"},
        [QBN_RSP  ] = {"spl", "sp", "esp", "rsp"},
        [QBN_R8   ] = {"r8b", "r8w", "r8d", "r8"},
        [QBN_R9   ] = {"r9b", "r9w", "r9d", "r9"},
        [QBN_R10  ] = {"r10b", "r10w", "r10d", "r10"},
        [QBN_R11  ] = {"r11b", "r11w", "r11d", "r11"},
        [QBN_R12  ] = {"r12b", "r12w", "r12d", "r12"},
        [QBN_R13  ] = {"r13b", "r13w", "r13d", "r13"},
        [QBN_R14  ] = {"r14b", "r14w", "r14d", "r14"},
        [QBN_R15  ] = {"r15b", "r15w", "r15d", "r15"},
        [QBN_XMM0 ] = {0, 0, "xmm0", "xmm0"},
        [QBN_XMM1 ] = {0, 0, "xmm1", "xmm1"},
        [QBN_XMM2 ] = {0, 0, "xmm2", "xmm2"},
        [QBN_XMM3 ] = {0, 0, "xmm3", "xmm3"},
        [QBN_XMM4 ] = {0, 0, "xmm4", "xmm4"},
        [QBN_XMM5 ] = {0, 0, "xmm5", "xmm5"},
        [QBN_XMM6 ] = {0, 0, "xmm6", "xmm6"},
        [QBN_XMM7 ] = {0, 0, "xmm7", "xmm7"},
        [QBN_XMM8 ] = {0, 0, "xmm8", "xmm8"},
        [QBN_XMM9 ] = {0, 0, "xmm9", "xmm9"},
        [QBN_XMM10] = {0, 0, "xmm10", "xmm10"},
        [QBN_XMM11] = {0, 0, "xmm11", "xmm11"},
        [QBN_XMM12] = {0, 0, "xmm12", "xmm12"},
        [QBN_XMM13] = {0, 0, "xmm13", "xmm13"},
        [QBN_XMM14] = {0, 0, "xmm14", "xmm14"},
        [QBN_XMM15] = {0, 0, "xmm15", "xmm15"},
};

#define QBN_AMD64_REG2GAS(reg, size) QBN_AMD64_REG2GAS_TABLE[reg][(size) <= 2 ? (size) - 1 : ((size) == 4 ? 2 : 3)]

typedef enum {
    QBN_GASOP_2A_TYPED,  // with type suffix, 1 src arg, 1 dest arg
    QBN_GASOP_1A_TYPED,  // with type suffix, 1 src arg
    QBN_GASOP_1D_TYPED,  // with type suffix, 1 dest arg
} QbnGasOpType;

typedef const struct {
    const char* str;
    QbnGasOpType type;
} QbnAmd64OpStuff;

QbnAmd64OpStuff QBN_AMD64_OP2GASSTR[] = {
        [QBN_OP_ADD]     = {"add", QBN_GASOP_2A_TYPED},
        [QBN_OP_SUB]     = {"sub", QBN_GASOP_2A_TYPED},
        [QBN_OP_ADDR]    = {"lea", QBN_GASOP_2A_TYPED},
        [QBN_OP_PUSH]    = {"push", QBN_GASOP_1A_TYPED},
        [QBN_OP_POP]     = {"pop", QBN_GASOP_1D_TYPED},
};

void qbn_fprintf_indent(FILE* file, const char* formatter, ...) {
    // print an indentation before the actual string
    fprintf(file, QBN_GAS_INDENT);
    va_list argp;
    va_start(argp, formatter);
    vfprintf(file, formatter, argp);
    va_end(argp);
}

void qbn_emit_amd64_reg(QbnRef ref, unsigned char size, FILE* file) {
    fprintf(file, "%%%s", QBN_AMD64_REG2GAS(QBN_REF_INDEX(ref), size));
}

void qbn_emit_amd64_const(QbnContext* context, QbnRef ref, FILE* file) {
    QbnConst* con = &context->consts[QBN_REF_INDEX(ref)];
    switch (con->type) {
        case QBN_CONST_NUMBER:
        case QBN_CONST_F64:
            fprintf(file, "$%ld", con->value.number);
            break;
        case QBN_CONST_F32:
            fprintf(file, "$%d", (int) con->value.number);
            break;
        case QBN_CONST_ADDR:
            fprintf(file, "%s(%%rip)", con->value.label);
            break;
        case QBN_CONST_NAME:
            fprintf(file, "%s", con->value.label);
            break;
    }
}

void qbn_emit_data(QbnContext* context, FILE* file) {
    assert(context->data_next->type == QBN_DATA_START);
    while (true) {
        if (context->current_section == QBN_SEC_NONE) {
            context->current_section = QBN_SEC_DATA;
            context->data_is_aligned = false;
            fprintf(file, ".data\n");
        }
        switch (context->data_next->type) {
            case QBN_DATA_START:
                if (!context->data_is_aligned) {
                    fprintf(file, ".balign 8\n");
                    // TODO: check if next line correct
                    context->data_is_aligned = true;
                }
                if (context->data_next->value.start.export) {
                    fprintf(file, ".globl %s\n", context->data_next->value.start.name);
                }
                fprintf(file, "%s:\n", context->data_next->value.start.name);
                break;
            case QBN_DATA_ALIGN:
                fprintf(file, ".balign %ld\n", context->data_next->value.align_length);
                context->data_is_aligned = true;
                break;
            case QBN_DATA_ZERO:
                qbn_fprintf_indent(file, ".fill %ld,1,0\n", context->data_next->value.zero_length);
                break;
            case QBN_DATA_REF_DATA:
                qbn_fprintf_indent(file, ".%s %s%+ld\n", QBN_TYPE2GAS[context->data_next->value.global_ref.ext_type],
                                   context->data_next->value.global_ref.name,
                                   context->data_next->value.global_ref.offset);
                break;
            case QBN_DATA_REF_FUNC:
                // TODO: change according to target pointer size
                qbn_fprintf_indent(file, ".%s %s\n", QBN_TYPE2GAS[QBN_TYPE_I64],
                                   context->data_next->value.global_ref.name);
                break;
            case QBN_DATA_STRING:
                qbn_fprintf_indent(file, ".ascii \"%s\"\n", context->data_next->value.string);
                break;
            case QBN_DATA_CONSTANT:
                if (context->data_next->value.number.ext_type == QBN_TYPE_F64) {
                    qbn_fprintf_indent(file, ".quad %ld\n", context->data_next->value.number.value.i);
                } else if (context->data_next->value.number.ext_type == QBN_TYPE_F32) {
                    qbn_fprintf_indent(file, ".int %d\n", (int) context->data_next->value.number.value.i);
                } else {
                    qbn_fprintf_indent(file, ".%s %ld\n", QBN_TYPE2GAS[context->data_next->value.number.ext_type],
                                       context->data_next->value.number.value.i);
                }
                break;
            case QBN_DATA_END:
                fprintf(file, "\n");
                return;
            default:
                QBN_UNREACHABLE
        }
        if (context->data_next->type == QBN_DATA_NEXT_VEC_BLOCK) {
            context->data_next = context->data_next->value.next;
        } else {
            context->data_next++;
        }
    }
}

unsigned long qbn_frame_size(QbnFn* fn) {
    // TODO
    return 128;
}

void qbn_emit_instr(QbnContext* context, QbnInstr* instr, FILE* file) {
    QbnAmd64OpStuff* op = &QBN_AMD64_OP2GASSTR[instr->op];
    assert(op->str != NULL);
    qbn_fprintf_indent(file, op->str);
    switch (op->type) {
        case QBN_GASOP_1A_TYPED:
            assert(instr->arg0 != QBN_REF0);
            fprintf(file, "%c ", QBN_TYPE2GASSUFFIX[instr->type]);
            switch (QBN_REF_TYPE(instr->arg0)) {
                case QBN_REF_REG:
                    qbn_emit_amd64_reg(instr->arg0, QBN_TYPE_INFO[instr->type].bytes, file);
                    break;
                case QBN_REF_CONST:
                    QBN_UNREACHABLE  // TODO: do we need this?
                    qbn_emit_amd64_const(context, instr->arg0, file);
                    break;
                default:
                    QBN_UNREACHABLE
            }
            fprintf(file, "\n");
            break;
        case QBN_GASOP_1D_TYPED:
            assert(instr->to != QBN_REF0);
            fprintf(file, "%c ", QBN_TYPE2GASSUFFIX[instr->type]);
            qbn_emit_amd64_reg(instr->to, QBN_TYPE_INFO[instr->type].bytes, file);
            fprintf(file, "\n");
            break;
        case QBN_GASOP_2A_TYPED:
            assert(instr->arg0 != QBN_REF0);
            fprintf(file, "%c ", QBN_TYPE2GASSUFFIX[instr->type]);
            switch (QBN_REF_TYPE(instr->arg0)) {
                case QBN_REF_REG:
                    qbn_emit_amd64_reg(instr->arg0, QBN_TYPE_INFO[instr->type].bytes, file);
                    break;
                case QBN_REF_CONST:
                    qbn_emit_amd64_const(context, instr->arg0, file);
                    break;
                default:
                    QBN_UNREACHABLE
            }
            fprintf(file, ", ");
            qbn_emit_amd64_reg(instr->to, QBN_TYPE_INFO[instr->type].bytes, file);
            fprintf(file, "\n");
            break;
    }
}

void qbn_emit_return(FILE* file, QbnBlock* block) {
    if (block->jmp_type != QBN_JUMP_RET_NONE) {
        // TODO ?
    }
    qbn_fprintf_indent(file, "leave\n");
    qbn_fprintf_indent(file, "ret\n");
}

void qbn_emit_block(QbnFn* fn, QbnBlock* block, FILE* file) {
    QbnInstr* instr = block->instr;
    QbnTemp* temp;
    while (instr->op != QBN_OP_BLOCK_END) {
        switch (instr->op) {
            case QBN_OP0:
                break;
            case QBN_OP_COPY:
                qbn_fprintf_indent(file, "mov");
                fprintf(file, "%c ", QBN_TYPE2GASSUFFIX[instr->type]);
                switch (QBN_REF_TYPE(instr->arg0)) {
                    case QBN_REF_REG:
                        qbn_emit_amd64_reg(instr->arg0, QBN_TYPE_INFO[instr->type].bytes, file);
                        break;
                    case QBN_REF_CONST:
                        qbn_emit_amd64_const(fn->context, instr->arg0, file);
                        break;
                    case QBN_REF_TEMP:
                        temp = &fn->temps[QBN_REF_INDEX(instr->arg0)];
                        assert(temp->slot != QBN_REF0);
                        assert(QBN_REF_TYPE(temp->slot) == QBN_REF_REG);
                        qbn_emit_amd64_reg(temp->slot, QBN_TYPE_INFO[instr->type].bytes, file);
                        break;
                    default:
                        QBN_UNREACHABLE
                }
                fprintf(file, ", ");
                if (QBN_REF_TYPE(instr->to) == QBN_REF_TEMP) {
                    temp = &fn->temps[QBN_REF_INDEX(instr->to)];
                    assert(temp->slot != QBN_REF0);
                    instr->to = temp->slot;
                }
                assert(QBN_REF_TYPE(instr->to));
                qbn_emit_amd64_reg(instr->to, QBN_TYPE_INFO[instr->type].bytes, file);
                fprintf(file, "\n");
                break;
            case QBN_OP_CALL:
                qbn_fprintf_indent(file, "call ");
                qbn_emit_amd64_const(fn->context, instr->arg0, file);
                fprintf(file, "\n");
                break;
            // TODO: support all (external) instructions
            // TODO refactor rest into table
            default:
                qbn_emit_instr(fn->context, instr, file);
        }
        instr++;
    }
}

void qbn_emit_fn(QbnFn* fn, FILE* file) {
    if (fn->context->current_section != QBN_SEC_TEXT) {
        fn->context->current_section = QBN_SEC_TEXT;
        fprintf(file, ".text\n");
    }
    if (fn->export) {
        fprintf(file, ".globl %s\n", fn->name);
    }
    fprintf(file, "%s:\n", fn->name);
    qbn_fprintf_indent(file, "pushq %%rbp\n");
    qbn_fprintf_indent(file, "movq %%rsp, %%rbp\n", fn->name);
    fn->frame_size = qbn_frame_size(fn);
    if (fn->frame_size) {
        qbn_fprintf_indent(file, "subq $%lu, %%rsp\n", fn->frame_size);
    }
    // TODO: varargs
    // push registers? callee saved registers probably

    for (int i=0; i<fn->vec_blocks->length; i++) {
        qbn_emit_block(fn, fn->blocks[i], file);
        if (QBN_IS_RETURN(fn->blocks[i]->jmp_type)) {
            qbn_emit_return(file, fn->blocks[i]);
        } else {
            // TODO: jumps
            QBN_NOT_IMPLEMENTED
        }
    }
    fprintf(file, "\n");
}

void qbn_emit(QbnContext* context, FILE* file) {
    context->current_section = QBN_SEC_NONE;
    context->data_next = context->data;
    for (int i=0; i<context->data_count; i++) {
        qbn_emit_data(context, file);
    }
    for (int i=0; i<context->vec_functions->length; i++) {
        qbn_emit_fn(context->functions[i], file);
    }
    fprintf(file, ".section .note.GNU-stack,\"\",@progbits\n");
}

#endif //QBN_PROCESSING_H
