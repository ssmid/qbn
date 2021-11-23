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
    qbn_data_new_cstring(context, "s", "Hello, world!\\n", false);

    QbnFn* fn = qbn_context_new_fn(context, QBN_BTYPE_I32, "main", true);
    QbnBlock* start = qbn_fn_next_block(fn);
    qbn_fn_add_instr(fn, QBN_OP_ARG, qbn_context_new_data_ref(context, "s"), QBN_REF0, QBN_REF0, context->size_type);
    qbn_fn_add_instr(fn, QBN_OP_CALL, qbn_context_new_name_ref(context, "printf"), QBN_REF0, QBN_REF0, 0);
    qbn_fn_block_return(fn, start, QBN_BTYPE_I32, qbn_context_new_const_number(context, 0));
    return context;
}

static QbnContext* set_up_test() {
    QbnContext* context = qbn_context_new();
    //QbnFn* fn_square = qbn_context_new_fn(context, QBN_BTYPE_I32, "square", false);
    //QbnBlock* block_square = qbn_fn_next_block(fn_square);
    //qbn_fn_add_instr(fn_square, QBN_OP_PAR, QBN_REF0, QBN_REF0, )

    QbnFn* fn_main = qbn_context_new_fn(context, QBN_BTYPE_I32, "main", true);
    QbnBlock* block_main1 = qbn_fn_next_block(fn_main);

    return context;
}

void run_example() {
    QbnContext* context = set_up_hello();

    // process and emit code
    fprintf(stderr, "instruction cache start:\n");
    qbn_print_instr_cache(context, stderr);

    for (int i=0; i<context->vec_functions->length; i++) {
        qbn_amd64_sysv_abi(context->functions[i]);
    }

    fprintf(stderr, "instruction cache after abi:\n");
    qbn_print_instr_cache(context, stderr);
    qbn_print_fns(context, stderr);

    qbn_emit(context, stdout);
    fprintf(stdout, "\n");

    char* asm_path = "../out.s";
    write_to_file(context, asm_path);

    // run gcc
    char* exe_path = "../out";
    char p_out[1024];
    char p_err[1024];
    char gcc_cmd[256];
    snprintf(gcc_cmd, 256, "gcc -pipe -x assembler -o %s %s", exe_path, asm_path);
    UtilProcess* gcc = util_process_new(gcc_cmd);
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
        UtilProcess* hello = util_process_new(exe_path);
        util_process_run(hello);
        util_process_read(hello, p_out, 4096);
        printf("%s", p_out);
        status = util_process_wait_exit(hello);
        printf("-> %d\n", status);
    } else {
        printf("gcc -> %d\n", status);
    }
}
