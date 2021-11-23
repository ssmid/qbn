
#ifndef QBN_PRINT_H
#define QBN_PRINT_H

#include <stdio.h>
#include "qbn.h"
#include "processing.h"


void qbn_print_ref(QbnContext* context, QbnRef ref, unsigned char size, FILE* file) {
    QbnConst* con;
    switch (QBN_REF_TYPE(ref)) {
        case QBN_REF_REG:
            fprintf(file, "%%%s", QBN_AMD64_REG2GAS(QBN_REF_INDEX(ref), size));
            break;
        case QBN_REF_TEMP:
            fprintf(file, "%%%d", QBN_REF_INDEX(ref));
            break;
        case QBN_REF_CONST:
            con = &context->consts[QBN_REF_INDEX(ref)];
            switch (con->type) {
                case QBN_CONST_NAME:
                case QBN_CONST_ADDR:
                    fprintf(file, "$%s", con->value.label);
                    break;
                case QBN_CONST_NUMBER:
                    fprintf(file, "%ld", con->value.number);
                    break;
                case QBN_CONST_F64:
                    fprintf(file, "d_%lf", con->value.f64);
                    break;
                case QBN_CONST_F32:
                    fprintf(file ,"s_%f", con->value.f32);
                    break;
            }
            break;
    }
}

void qbn_print_instr(QbnContext* context, QbnInstr* instr, FILE* file) {
    static bool last_op0 = false;
    qbn_fprintf_indent(file, "");
    if (instr->op == QBN_OP0 && last_op0) {
        return;
    }
    if (QBN_REF_TYPE(instr->to) != QBN_REF_NONE) {
        qbn_print_ref(context, instr->to, QBN_TYPE_INFO[instr->type].bytes, file);
        fprintf(file, " = %s ", qbn_type2s[instr->type]);
    }
    fprintf(file, "%s ", qbn_op2str[instr->op]);
    if (QBN_REF_TYPE(instr->arg0) != QBN_REF_NONE) {
        qbn_print_ref(context, instr->arg0, QBN_TYPE_INFO[instr->type].bytes, file);
    }
    if (QBN_REF_TYPE(instr->arg1) != QBN_REF_NONE) {
        fprintf(file, ", ");
        qbn_print_ref(context, instr->arg1, QBN_TYPE_INFO[instr->type].bytes, file);
    }
    fprintf(file, "\n");
    last_op0 = instr->op == QBN_OP0;
}

void qbn_print_fn(QbnFn* fn, FILE* file) {
    fprintf(file, "%s():\n", fn->name);
    for (int i=0; i<fn->vec_blocks->length; i++) {
        for (QbnInstr* instr = fn->blocks[i]->instr; instr->op != QBN_OP_BLOCK_END; instr++) {
            qbn_print_instr(fn->context, instr, file);
        }
    }
    fprintf(file, "\n");
}

void qbn_print_fns(QbnContext* context, FILE* file) {
    for (int i=0; i<context->vec_functions->length; i++) {
        qbn_print_fn(context->functions[i], file);
    }
}

void qbn_print_instr_cache(QbnContext* context, FILE* file) {
    QbnInstr* ip = context->instr_cache;
    fprintf(file, "instr:\n");
    for (int i=0; i < 64; i++, ip++) {
        if (ip->op != QBN_OP0) {
            qbn_print_instr(context, ip, file);
        }
    }
    fprintf(file, "\n");
}

#endif //QBN_PRINT_H
