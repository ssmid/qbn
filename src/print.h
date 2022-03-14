
#ifndef QBN_PRINT_H
#define QBN_PRINT_H

#include <stdio.h>
#include "qbn.h"
#include "processing.h"


const char* QBN_IDENT = "    ";

const char* QBN_DATA_ITEM_TYPE_TO_STR[] = {
        [QBN_DATA_START] = "QBN_DATA_START",
        [QBN_DATA_ALIGN] = "QBN_DATA_ALIGN",
        [QBN_DATA_ZERO] = "QBN_DATA_ZERO",
        [QBN_DATA_REF_DATA] = "QBN_DATA_REF_DATA",
        [QBN_DATA_REF_FUNC] = "QBN_DATA_REF_FUNC",
        [QBN_DATA_STRING] = "QBN_DATA_STRING",
        [QBN_DATA_CONSTANT] = "QBN_DATA_CONSTANT",
        [QBN_DATA_END] = "QBN_DATA_END",
        [QBN_DATA_NEXT_VEC_BLOCK] = "QBN_DATA_NEXT_VEC_BLOCK",
};

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
                case QBN_CONST_GLOBAL_ADDR:
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

void qbn_print_ref_fn(QbnFn* fn, QbnRef ref, unsigned char size, FILE* file) {
    QbnConst* con;
    QbnTemp* temp;
    switch (QBN_REF_TYPE(ref)) {
        case QBN_REF_REG:
            fprintf(file, "%%%s", QBN_AMD64_REG2GAS(QBN_REF_INDEX(ref), size));
            break;
        case QBN_REF_TEMP:
            temp = &fn->temps[QBN_REF_INDEX(ref)];
            if (temp->slot == QBN_REF0) {
                fprintf(file, "%s %%%d", qbn_type2s[temp->type], QBN_REF_INDEX(ref));
            } else {
                qbn_print_ref_fn(fn, temp->slot, size, file);
            }
            break;
        case QBN_REF_CONST:
            con = &fn->context->consts[QBN_REF_INDEX(ref)];
            switch (con->type) {
                case QBN_CONST_NAME:
                case QBN_CONST_GLOBAL_ADDR:
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
    fprintf(file, QBN_IDENT);
    if (instr->op == QBN_OP0 && last_op0) {
        return;
    }
    if (QBN_REF_TYPE(instr->to) != QBN_REF_NONE) {
        qbn_print_ref(context, instr->to, QBN_TYPE_INFO[instr->type].bytes, file);
        fprintf(file, " = %s ", qbn_type2s[instr->type]);
    }
    fprintf(file, "%s", qbn_op2str[instr->op]);
    if (QBN_REF_TYPE(instr->arg0) != QBN_REF_NONE) {
        fprintf(file, " ");
        qbn_print_ref(context, instr->arg0, QBN_TYPE_INFO[instr->type].bytes, file);
    }
    if (QBN_REF_TYPE(instr->arg1) != QBN_REF_NONE) {
        fprintf(file, ", ");
        qbn_print_ref(context, instr->arg1, QBN_TYPE_INFO[instr->type].bytes, file);
    }
    fprintf(file, "\n");
    last_op0 = instr->op == QBN_OP0;
}

void qbn_print_instr_fn(QbnFn* fn, QbnInstr* instr, FILE* file) {
    static bool last_op0 = false;
    fprintf(file, QBN_IDENT);
    if (instr->op == QBN_OP0 && last_op0) {
        return;
    }
    if (QBN_REF_TYPE(instr->to) != QBN_REF_NONE) {
        qbn_print_ref_fn(fn, instr->to, QBN_TYPE_INFO[instr->type].bytes, file);
        fprintf(file, " = %s ", qbn_type2s[instr->type]);
    }
    fprintf(file, "%s", qbn_op2str[instr->op]);
    if (QBN_REF_TYPE(instr->arg0) != QBN_REF_NONE) {
        fprintf(file, " ");
        qbn_print_ref_fn(fn, instr->arg0, QBN_TYPE_INFO[instr->type].bytes, file);
    }
    if (QBN_REF_TYPE(instr->arg1) != QBN_REF_NONE) {
        fprintf(file, ", ");
        qbn_print_ref_fn(fn, instr->arg1, QBN_TYPE_INFO[instr->type].bytes, file);
    }
    fprintf(file, "\n");
    last_op0 = instr->op == QBN_OP0;
}

void qbn_print_fn(QbnFn* fn, FILE* file) {
    fprintf(file, "%s():\n", fn->name);
    for (int i=0; i<fn->vec_blocks->length; i++) {
        fprintf(file, "b%d:\n", i);
        for (QbnInstr* instr = fn->blocks[i]->instr; instr->op != QBN_OP_BLOCK_END; instr++) {
            qbn_print_instr_fn(fn, instr, file);
        }
        // TODO: print jumps/returns
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

void qbn_print_data(QbnContext* context, FILE* file) {
    assert(context->data->type == QBN_DATA_START);
    QbnDataItem* data = context->data;
    while (1) {
        fprintf(file, "%s\n", QBN_DATA_ITEM_TYPE_TO_STR[data->type]);
        switch (data->type) {
            case QBN_DATA_NEXT_VEC_BLOCK:
                data = data->value.next;
                if (data == NULL) {
                    return;
                } else {
                    break;
                }
            default:
                data++;
        }
    }
}

void qbn_print_all(FILE* file, QbnContext* context, char* hint) {
    fprintf(file, "%s:\n", hint);
    qbn_print_instr_cache(context, file);
    qbn_print_fns(context, file);
}

#endif //QBN_PRINT_H
