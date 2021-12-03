#include "qbn.h"
#include <stdio.h>
#include "util/process.h"
#include "processing.h"
#include "print.h"


static void write_to_file(QbnContext* context, const char* file_name) {
    FILE* file = fopen(file_name, "w");
    if (!file) {
        int err = errno;
        fprintf(stderr, "Could not open %s with qbn_error %d\n", file_name, err);
        exit(1);
    }
    qbn_emit(context, file);
    fclose(file);
}

static QbnContext* set_up_hello() {
    QbnContext* context = qbn_context_new();
    QbnRef s = qbn_data_new_cstring(context, "s", "Hello, world!\\n", false);

    QbnFn* fn = qbn_context_new_fn(context, QBN_BTYPE_I32, "main", true);
    QbnBlock* block = qbn_fn_new_block(fn);
    qbn_fn_add_instr(fn, QBN_OP_ARG, s, QBN_REF0, QBN_REF0, context->size_type);
    qbn_fn_add_instr(fn, QBN_OP_CALL, qbn_context_new_name_ref(context, "printf"), QBN_REF0, QBN_REF0, 0);
    qbn_fn_close_block(fn);
    qbn_fn_block_return(fn, block, QBN_BTYPE_I32, qbn_context_new_const_number(context, 0));
    return context;
}

static QbnContext* set_up_test() {
    QbnContext* context = qbn_context_new();
    QbnRef s = qbn_data_new_cstring(context, "s", "Hello, world!\\n", false);

    QbnFn* fn = qbn_context_new_fn(context, QBN_BTYPE_I32, "main", true);
    QbnRef argc = qbn_fn_add_parameter(fn, QBN_BTYPE_I32);
    QbnRef argv = qbn_fn_add_parameter(fn, context->size_type);
    QbnBlock* block = qbn_fn_new_block(fn);
    QbnRef ret_value = qbn_fn_new_temp(fn, QBN_BTYPE_I32);
    qbn_fn_add_instr(fn, QBN_OP_COPY, argc, QBN_REF0, ret_value, QBN_BTYPE_I32);
    qbn_fn_add_instr(fn, QBN_OP_ARG, s, QBN_REF0, QBN_REF0, context->size_type);
    qbn_fn_add_instr(fn, QBN_OP_CALL, qbn_context_new_name_ref(context, "printf"), QBN_REF0, QBN_REF0, 0);
    qbn_fn_close_block(fn);
    qbn_fn_block_return(fn, block, QBN_BTYPE_I32, ret_value);
    return context;
}

static QbnContext* set_up_test2() {
    QbnContext* context = qbn_context_new();
    QbnFn* fn_square = qbn_context_new_fn(context, QBN_BTYPE_I32, "square", false);
    QbnRef i = qbn_fn_add_parameter(fn_square, QBN_BTYPE_I32);
    QbnBlock* block_square = qbn_fn_new_block(fn_square);
    qbn_fn_add_instr(fn_square, QBN_OP_MUL, i, i, i, QBN_BTYPE_I32);
    qbn_fn_close_block(fn_square);
    qbn_fn_block_return(fn_square, block_square, QBN_BTYPE_I32, i);

    QbnFn* fn_main = qbn_context_new_fn(context, QBN_BTYPE_I32, "main", true);
    QbnBlock* block_main1 = qbn_fn_new_block(fn_main);

    qbn_fn_close_block(fn_main);
    qbn_fn_block_return(fn_main, block_main1, QBN_BTYPE_I32, qbn_context_new_const_number(context, 0));

    return context;
}

void run_example() {
    QbnContext* context = set_up_test();

    qbn_print_all(stderr, context, "after ir gen");
    qbn_process(context);
    qbn_print_all(stderr, context, "after processing");
    qbn_emit(context, stdout);
    fprintf(stdout, "\n");

    char* asm_path = "../out.s";
    write_to_file(context, asm_path);

    // run gcc
    char* exe_path = "../out";
    char p_out[1024];
    char p_err[1024];
    char cmd[256];
    snprintf(cmd, 256, "gcc -pipe -x assembler -o %s %s", exe_path, asm_path);
    UtilProcess* gcc = util_process_new(cmd);
    util_process_run(gcc);

    // print gcc output
    size_t n_read_out = 1;
    size_t n_read_err = 1;
    while (n_read_out || n_read_err) {
        n_read_out = util_process_read(gcc, p_out, 1024);
        fprintf(stdout, "%s", p_out);
        n_read_err = util_process_read_err(gcc, p_err, 1024);
        fprintf(stderr, "%s", p_err);
    }
    int status = util_process_wait_exit(gcc);

    // run executable
    if (!status) {
        snprintf(cmd, 256, "%s test_arg0 test_arg1 test_arg2", exe_path);
        UtilProcess* hello = util_process_new(cmd);
        util_process_run(hello);
        util_process_read(hello, p_out, 4096);
        printf("%s", p_out);
        status = util_process_wait_exit(hello);
        printf("-> %d\n", status);
    } else {
        printf("gcc -> %d\n", status);
    }
}
