
void mstart(FILE *fp_in, FILE *fp_out, int go_flag, int setup_tty_flag,
            int init_lines, void (*quit_fn)());
void mend(int follow_lines);
int mputc(char ch), mputs(char *s), mprintf(char *format, ...);
