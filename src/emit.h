/*\
 *  \function{emit()}
 *
 *  We have implemented emit as macros, since we expect to use them
 *  heavily, and would like to save a procedure call.  We need a several
 *  versions of this macro, which are alike except for the type of
 *  operands they expect.
\*/

/* general form -- symbol table operands */

#define emit(p,o1,o2,o3,fp) { \
   if (!USE_INTERMEDIATE_FILES) { \
      emit_quad_ptr = get_quad(SETL_SYSTEM_VOID); \
      *emit_quad_tail = emit_quad_ptr; \
      emit_quad_tail = &(emit_quad_ptr->q_next); } \
   emit_quad_ptr->q_opcode = p; \
   emit_quad_ptr->q_operand[0].q_symtab_ptr = (o1); \
   emit_quad_ptr->q_operand[1].q_symtab_ptr = (o2); \
   emit_quad_ptr->q_operand[2].q_symtab_ptr = (o3); \
   emit_quad_ptr->q_file_pos.fp_line = (fp)->fp_line; \
   emit_quad_ptr->q_file_pos.fp_column = (fp)->fp_column; \
   if (USE_INTERMEDIATE_FILES)  \
      if (fwrite((void *)emit_quad_ptr,sizeof(struct quad_item), \
                 (size_t)1,I1_FILE) != (size_t)1) \
         giveup(SETL_SYSTEM msg_iter_write_error); \
}

/* first argument integer, others symbol table pointers */

#define emitiss(p,o1,o2,o3,fp) { \
   if (!USE_INTERMEDIATE_FILES) { \
      emit_quad_ptr = get_quad(SETL_SYSTEM_VOID); \
      *emit_quad_tail = emit_quad_ptr; \
      emit_quad_tail = &(emit_quad_ptr->q_next); } \
   emit_quad_ptr->q_opcode = p; \
   emit_quad_ptr->q_operand[0].q_integer = (o1); \
   emit_quad_ptr->q_operand[1].q_symtab_ptr = (o2); \
   emit_quad_ptr->q_operand[2].q_symtab_ptr = (o3); \
   emit_quad_ptr->q_file_pos.fp_line = (fp)->fp_line; \
   emit_quad_ptr->q_file_pos.fp_column = (fp)->fp_column; \
   if (USE_INTERMEDIATE_FILES)  \
      if (fwrite((void *)emit_quad_ptr,sizeof(struct quad_item), \
                 (size_t)1,I1_FILE) != (size_t)1) \
         giveup(SETL_SYSTEM msg_iter_write_error); \
}

/* last argument integer, others symbol table pointers */

#define emitssi(p,o1,o2,o3,fp) { \
   if (!USE_INTERMEDIATE_FILES) { \
      emit_quad_ptr = get_quad(SETL_SYSTEM_VOID);  \
      *emit_quad_tail = emit_quad_ptr; \
      emit_quad_tail = &(emit_quad_ptr->q_next); } \
   emit_quad_ptr->q_opcode = p; \
   emit_quad_ptr->q_operand[0].q_symtab_ptr = (o1); \
   emit_quad_ptr->q_operand[1].q_symtab_ptr = (o2); \
   emit_quad_ptr->q_operand[2].q_integer = (o3); \
   emit_quad_ptr->q_file_pos.fp_line = (fp)->fp_line; \
   emit_quad_ptr->q_file_pos.fp_column = (fp)->fp_column; \
   if (USE_INTERMEDIATE_FILES)  \
      if (fwrite((void *)emit_quad_ptr,sizeof(struct quad_item), \
                 (size_t)1,I1_FILE) != (size_t)1) \
         giveup(SETL_SYSTEM msg_iter_write_error); \
}

