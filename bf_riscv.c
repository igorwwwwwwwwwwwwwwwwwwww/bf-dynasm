/*
** This file has been pre-processed with DynASM.
** https://luajit.org/dynasm.html
** DynASM version 1.5.0, DynASM riscv64 version 1.5.0
** DO NOT EDIT! The original file is in "bf_riscv.dasc".
*/

#line 1 "bf_riscv.dasc"
//|.arch riscv64
#if DASM_VERSION != 10500
#error "Version mismatch between DynASM and included encoding engine"
#endif
#line 2 "bf_riscv.dasc"
//|.actionlist actions
static const unsigned int actions[249] = {
0x0149f2b3,
0x005902b3,
0x0002c303,
0x00000000,
0x00000293,
0x00981940,
0x005982b3,
0x0142f2b3,
0x005902b3,
0x0002c303,
0x00000000,
0x0149f3b3,
0x007903b3,
0x0003ce03,
0x00000000,
0x00000393,
0x00981940,
0x007983b3,
0x0143f3b3,
0x007903b3,
0x0003ce03,
0x00000000,
0x006e0e33,
0x00000000,
0x406e0e33,
0x00000000,
0x00000e93,
0x00981940,
0x03d30eb3,
0x01de0e33,
0x00000000,
0x01c38023,
0x00000000,
0x00094303,
0x00000000,
0x00094303,
0x00981940,
0x00000000,
0x00000293,
0x00981940,
0x005902b3,
0x0002c303,
0x00000000,
0x00094e03,
0x00000000,
0x00094e03,
0x00981940,
0x00000000,
0x00000393,
0x00981940,
0x007903b3,
0x0003ce03,
0x00000000,
0x006e0e33,
0x00000000,
0x406e0e33,
0x00000000,
0x00000e93,
0x00981940,
0x03d30eb3,
0x01de0e33,
0x00000000,
0x01c90023,
0x00000000,
0x01c90023,
0x00a00000,
0x00000000,
0x00000393,
0x00981940,
0x007903b3,
0x01c38023,
0x00000000,
0xfd010113,
0x02113423,
0x03213023,
0x01313c23,
0x01413823,
0x00050913,
0x00000993,
0x00000000,
0x00000a13,
0x00981940,
0x00000000,
0x00000513,
0x01013a03,
0x01813983,
0x02013903,
0x02813083,
0x03010113,
0x00008067,
0x00000000,
0x0149f2b3,
0x005902b3,
0x0002c303,
0x00000000,
0x00094303,
0x00000000,
0x00030063,
0x00708000,
0x00000000,
0x0149f2b3,
0x005902b3,
0x0002c303,
0x00000000,
0x00094303,
0x00000000,
0x00031063,
0x00708000,
0x00000000,
0x00800000,
0x00000000,
0x00800000,
0x00000000,
0x00090913,
0x00981940,
0x00000000,
0x00000293,
0x00981940,
0x00590933,
0x00000000,
0x00090913,
0x00981940,
0x00000000,
0x00000293,
0x00981940,
0x00590933,
0x00000000,
0x00098993,
0x00981940,
0x00000000,
0x00000293,
0x00981940,
0x005989b3,
0x00000000,
0x00098993,
0x00981940,
0x00000000,
0x00000293,
0x00981940,
0x005989b3,
0x00000000,
0x00000293,
0x00981940,
0x005982b3,
0x0142f2b3,
0x005902b3,
0x0002c303,
0x00000000,
0x00000393,
0x00981940,
0x00730333,
0x00628023,
0x00000000,
0x00094303,
0x00000000,
0x00000393,
0x00981940,
0x00730333,
0x00690023,
0x00000000,
0x00094303,
0x00981940,
0x00000000,
0x00000393,
0x00981940,
0x00730333,
0x00690023,
0x00a00000,
0x00000000,
0x00000293,
0x00981940,
0x005902b3,
0x0002c303,
0x00000000,
0x00000393,
0x00981940,
0x00730333,
0x00628023,
0x00000000,
0x00000293,
0x00981940,
0x000280e7,
0x00000000,
0x00000293,
0x00981940,
0x005982b3,
0x0142f2b3,
0x005902b3,
0x00a28023,
0x00000000,
0x00a90023,
0x00000000,
0x00a90023,
0x00a00000,
0x00000000,
0x00000293,
0x00981940,
0x005902b3,
0x00a28023,
0x00000000,
0x00000293,
0x00981940,
0x005982b3,
0x0142f2b3,
0x005902b3,
0x0002c503,
0x00000000,
0x00094503,
0x00000000,
0x00094503,
0x00981940,
0x00000000,
0x00000293,
0x00981940,
0x005902b3,
0x0002c503,
0x00000000,
0x00000293,
0x00981940,
0x000280e7,
0x00000000,
0x00000313,
0x00981940,
0x00000000,
0x00000293,
0x00981940,
0x005982b3,
0x0142f2b3,
0x005902b3,
0x00628023,
0x00000000,
0x00690023,
0x00000000,
0x00690023,
0x00a00000,
0x00000000,
0x00000293,
0x00981940,
0x005902b3,
0x00628023,
0x00000000,
0x00000513,
0x00981940,
0x00000593,
0x00981940,
0x00000293,
0x00981940,
0x000280e7,
0x00000000
};

#line 3 "bf_riscv.dasc"
//|.section code
#define DASM_SECTION_CODE	0
#define DASM_MAXSECTION		1
#line 4 "bf_riscv.dasc"

// Access to global unsafe mode flag
extern bool g_unsafe_mode;

static int putchar_wrapper(int c) {
    return putchar(c);
}

static int getchar_wrapper(void) {
    int c = getchar();
    if (c == EOF) return 0;
    return c;
}

static void debug_log_location(int line, int column) {
    fprintf(stderr, "DEBUG: Line %d, Column %d\n", line, column);
    fflush(stderr);
}

static void compile_bf_mul(dasm_State **Dst, int multiplier, int src_offset, int dst_offset) {
    if (multiplier == 0) return;

    if (!g_unsafe_mode) {
        if (src_offset == 0) {
            //|  and x5, x19, x20
            //|  add x5, x18, x5
            //|  lbu x6, 0(x5)
            dasm_put(Dst, 0);
#line 31 "bf_riscv.dasc"
        } else {
            //|  li x5, src_offset
            //|  add x5, x19, x5
            //|  and x5, x5, x20
            //|  add x5, x18, x5
            //|  lbu x6, 0(x5)
            dasm_put(Dst, 4, src_offset);
#line 37 "bf_riscv.dasc"
        }

        if (dst_offset == 0) {
            //|  and x7, x19, x20
            //|  add x7, x18, x7
            //|  lbu x28, 0(x7)
            dasm_put(Dst, 11);
#line 43 "bf_riscv.dasc"
        } else {
            //|  li x7, dst_offset
            //|  add x7, x19, x7
            //|  and x7, x7, x20
            //|  add x7, x18, x7
            //|  lbu x28, 0(x7)
            dasm_put(Dst, 15, dst_offset);
#line 49 "bf_riscv.dasc"
        }

        if (multiplier == 1) {
            //|  add x28, x28, x6
            dasm_put(Dst, 22);
#line 53 "bf_riscv.dasc"
        } else if (multiplier == -1) {
            //|  sub x28, x28, x6
            dasm_put(Dst, 24);
#line 55 "bf_riscv.dasc"
        } else {
            //|  li x29, multiplier
            //|  mul x29, x6, x29
            //|  add x28, x28, x29
            dasm_put(Dst, 26, multiplier);
#line 59 "bf_riscv.dasc"
        }
        //|  sb x28, 0(x7)
        dasm_put(Dst, 31);
#line 61 "bf_riscv.dasc"
    } else {
        if (src_offset == 0) {
            //|  lbu x6, 0(x18)
            dasm_put(Dst, 33);
#line 64 "bf_riscv.dasc"
        } else if (src_offset >= -2048 && src_offset <= 2047) {
            //|  lbu x6, src_offset(x18)
            dasm_put(Dst, 35, src_offset);
#line 66 "bf_riscv.dasc"
        } else {
            //|  li x5, src_offset
            //|  add x5, x18, x5
            //|  lbu x6, 0(x5)
            dasm_put(Dst, 38, src_offset);
#line 70 "bf_riscv.dasc"
        }

        if (dst_offset == 0) {
            //|  lbu x28, 0(x18)
            dasm_put(Dst, 43);
#line 74 "bf_riscv.dasc"
        } else if (dst_offset >= -2048 && dst_offset <= 2047) {
            //|  lbu x28, dst_offset(x18)
            dasm_put(Dst, 45, dst_offset);
#line 76 "bf_riscv.dasc"
        } else {
            //|  li x7, dst_offset
            //|  add x7, x18, x7
            //|  lbu x28, 0(x7)
            dasm_put(Dst, 48, dst_offset);
#line 80 "bf_riscv.dasc"
        }

        if (multiplier == 1) {
            //|  add x28, x28, x6
            dasm_put(Dst, 53);
#line 84 "bf_riscv.dasc"
        } else if (multiplier == -1) {
            //|  sub x28, x28, x6
            dasm_put(Dst, 55);
#line 86 "bf_riscv.dasc"
        } else {
            //|  li x29, multiplier
            //|  mul x29, x6, x29
            //|  add x28, x28, x29
            dasm_put(Dst, 57, multiplier);
#line 90 "bf_riscv.dasc"
        }

        if (dst_offset == 0) {
            //|  sb x28, 0(x18)
            dasm_put(Dst, 62);
#line 94 "bf_riscv.dasc"
        } else if (dst_offset >= -2048 && dst_offset <= 2047) {
            //|  sb x28, dst_offset(x18)
            dasm_put(Dst, 64, dst_offset);
#line 96 "bf_riscv.dasc"
        } else {
            //|  li x7, dst_offset
            //|  add x7, x18, x7
            //|  sb x28, 0(x7)
            dasm_put(Dst, 67, dst_offset);
#line 100 "bf_riscv.dasc"
        }
    }
}

static void compile_bf_prologue(dasm_State **Dst, size_t memory_size) {
    //|  addi sp, sp, -48
    //|  sd ra, 40(sp)
    //|  sd x18, 32(sp)
    //|  sd x19, 24(sp)
    //|  sd x20, 16(sp)
    //|  mv x18, x10
    //|  li x19, 0
    dasm_put(Dst, 72);
#line 112 "bf_riscv.dasc"

    size_t mask = memory_size - 1;
    //|  li x20, mask
    dasm_put(Dst, 80, mask);
#line 115 "bf_riscv.dasc"
}

static void compile_bf_epilogue(dasm_State **Dst) {
    //|  li x10, 0
    //|  ld x20, 16(sp)
    //|  ld x19, 24(sp)
    //|  ld x18, 32(sp)
    //|  ld ra, 40(sp)
    //|  addi sp, sp, 48
    //|  ret
    dasm_put(Dst, 83);
#line 125 "bf_riscv.dasc"
}

static void compile_bf_loop_start(dasm_State **Dst, int loop_end) {
    if (!g_unsafe_mode) {
        //|  and x5, x19, x20
        //|  add x5, x18, x5
        //|  lbu x6, 0(x5)
        dasm_put(Dst, 91);
#line 132 "bf_riscv.dasc"
    } else {
        //|  lbu x6, 0(x18)
        dasm_put(Dst, 95);
#line 134 "bf_riscv.dasc"
    }
    //|  beq x6, x0, =>(loop_end)
    dasm_put(Dst, 97, (loop_end));
#line 136 "bf_riscv.dasc"
}

static void compile_bf_loop_end(dasm_State **Dst, int back_to_start) {
    if (!g_unsafe_mode) {
        //|  and x5, x19, x20
        //|  add x5, x18, x5
        //|  lbu x6, 0(x5)
        dasm_put(Dst, 100);
#line 143 "bf_riscv.dasc"
    } else {
        //|  lbu x6, 0(x18)
        dasm_put(Dst, 104);
#line 145 "bf_riscv.dasc"
    }
    //|  bne x6, x0, =>(back_to_start)
    dasm_put(Dst, 106, (back_to_start));
#line 147 "bf_riscv.dasc"
}

static void compile_bf_label(dasm_State **Dst, int label) {
    //|=>(label):
    dasm_put(Dst, 109, (label));
#line 151 "bf_riscv.dasc"
}

static void compile_bf_debug_label(dasm_State **Dst, int debug_label) {
    //|=>(debug_label):
    dasm_put(Dst, 111, (debug_label));
#line 155 "bf_riscv.dasc"
}

static void compile_bf_move_ptr(dasm_State **Dst, int count) {
    if (g_unsafe_mode) {
        if (count > 0) {
            if (count <= 2047) {
                //|  addi x18, x18, count
                dasm_put(Dst, 113, count);
#line 162 "bf_riscv.dasc"
            } else {
                //|  li x5, count
                //|  add x18, x18, x5
                dasm_put(Dst, 116, count);
#line 165 "bf_riscv.dasc"
            }
        } else if (count < 0) {
            if (count >= -2048) {
                //|  addi x18, x18, count
                dasm_put(Dst, 120, count);
#line 169 "bf_riscv.dasc"
            } else {
                //|  li x5, count
                //|  add x18, x18, x5
                dasm_put(Dst, 123, count);
#line 172 "bf_riscv.dasc"
            }
        }
    } else {
        if (count > 0) {
            if (count <= 2047) {
                //|  addi x19, x19, count
                dasm_put(Dst, 127, count);
#line 178 "bf_riscv.dasc"
            } else {
                //|  li x5, count
                //|  add x19, x19, x5
                dasm_put(Dst, 130, count);
#line 181 "bf_riscv.dasc"
            }
        } else if (count < 0) {
            if (count >= -2048) {
                //|  addi x19, x19, count
                dasm_put(Dst, 134, count);
#line 185 "bf_riscv.dasc"
            } else {
                //|  li x5, count
                //|  add x19, x19, x5
                dasm_put(Dst, 137, count);
#line 188 "bf_riscv.dasc"
            }
        }
    }
}

static void compile_bf_add_val(dasm_State **Dst, int count, int offset) {
    if (!g_unsafe_mode) {
        //|  li x5, offset
        //|  add x5, x19, x5
        //|  and x5, x5, x20
        //|  add x5, x18, x5
        //|  lbu x6, 0(x5)
        dasm_put(Dst, 141, offset);
#line 200 "bf_riscv.dasc"
        if (count != 0) {
            //|  li x7, count
            //|  add x6, x6, x7
            //|  sb x6, 0(x5)
            dasm_put(Dst, 148, count);
#line 204 "bf_riscv.dasc"
        }
    } else {
        if (offset == 0) {
            //|  lbu x6, 0(x18)
            dasm_put(Dst, 153);
#line 208 "bf_riscv.dasc"
            if (count != 0) {
                //|  li x7, count
                //|  add x6, x6, x7
                //|  sb x6, 0(x18)
                dasm_put(Dst, 155, count);
#line 212 "bf_riscv.dasc"
            }
        } else if (offset >= -2048 && offset <= 2047) {
            //|  lbu x6, offset(x18)
            dasm_put(Dst, 160, offset);
#line 215 "bf_riscv.dasc"
            if (count != 0) {
                //|  li x7, count
                //|  add x6, x6, x7
                //|  sb x6, offset(x18)
                dasm_put(Dst, 163, count, offset);
#line 219 "bf_riscv.dasc"
            }
        } else {
            //|  li x5, offset
            //|  add x5, x18, x5
            //|  lbu x6, 0(x5)
            dasm_put(Dst, 169, offset);
#line 224 "bf_riscv.dasc"
            if (count != 0) {
                //|  li x7, count
                //|  add x6, x6, x7
                //|  sb x6, 0(x5)
                dasm_put(Dst, 174, count);
#line 228 "bf_riscv.dasc"
            }
        }
    }
}

static void compile_bf_input(dasm_State **Dst, int offset) {
    //|  li x5, (uintptr_t)getchar_wrapper
    //|  jalr x5
    dasm_put(Dst, 179, (uintptr_t)getchar_wrapper);
#line 236 "bf_riscv.dasc"

    if (!g_unsafe_mode) {
        //|  li x5, offset
        //|  add x5, x19, x5
        //|  and x5, x5, x20
        //|  add x5, x18, x5
        //|  sb x10, 0(x5)
        dasm_put(Dst, 183, offset);
#line 243 "bf_riscv.dasc"
    } else if (offset == 0) {
        //|  sb x10, 0(x18)
        dasm_put(Dst, 190);
#line 245 "bf_riscv.dasc"
    } else if (offset >= -2048 && offset <= 2047) {
        //|  sb x10, offset(x18)
        dasm_put(Dst, 192, offset);
#line 247 "bf_riscv.dasc"
    } else {
        //|  li x5, offset
        //|  add x5, x18, x5
        //|  sb x10, 0(x5)
        dasm_put(Dst, 195, offset);
#line 251 "bf_riscv.dasc"
    }
}

static void compile_bf_output(dasm_State **Dst, int offset) {
    if (!g_unsafe_mode) {
        //|  li x5, offset
        //|  add x5, x19, x5
        //|  and x5, x5, x20
        //|  add x5, x18, x5
        //|  lbu x10, 0(x5)
        dasm_put(Dst, 200, offset);
#line 261 "bf_riscv.dasc"
    } else if (offset == 0) {
        //|  lbu x10, 0(x18)
        dasm_put(Dst, 207);
#line 263 "bf_riscv.dasc"
    } else if (offset >= -2048 && offset <= 2047) {
        //|  lbu x10, offset(x18)
        dasm_put(Dst, 209, offset);
#line 265 "bf_riscv.dasc"
    } else {
        //|  li x5, offset
        //|  add x5, x18, x5
        //|  lbu x10, 0(x5)
        dasm_put(Dst, 212, offset);
#line 269 "bf_riscv.dasc"
    }

    //|  li x5, (uintptr_t)putchar_wrapper
    //|  jalr x5
    dasm_put(Dst, 217, (uintptr_t)putchar_wrapper);
#line 273 "bf_riscv.dasc"
}

static void compile_bf_set_const(dasm_State **Dst, int value, int offset) {
    //|  li x6, value
    dasm_put(Dst, 221, value);
#line 277 "bf_riscv.dasc"
    if (!g_unsafe_mode) {
        //|  li x5, offset
        //|  add x5, x19, x5
        //|  and x5, x5, x20
        //|  add x5, x18, x5
        //|  sb x6, 0(x5)
        dasm_put(Dst, 224, offset);
#line 283 "bf_riscv.dasc"
    } else if (offset == 0) {
        //|  sb x6, 0(x18)
        dasm_put(Dst, 231);
#line 285 "bf_riscv.dasc"
    } else if (offset >= -2048 && offset <= 2047) {
        //|  sb x6, offset(x18)
        dasm_put(Dst, 233, offset);
#line 287 "bf_riscv.dasc"
    } else {
        //|  li x5, offset
        //|  add x5, x18, x5
        //|  sb x6, 0(x5)
        dasm_put(Dst, 236, offset);
#line 291 "bf_riscv.dasc"
    }
}

static void compile_bf_debug_log(dasm_State **Dst, bool debug_mode, int line, int column) {
    if (debug_mode) {
        //|  li x10, line
        //|  li x11, column
        //|  li x5, (uintptr_t)debug_log_location
        //|  jalr x5
        dasm_put(Dst, 241, line, column, (uintptr_t)debug_log_location);
#line 300 "bf_riscv.dasc"
    }
}
