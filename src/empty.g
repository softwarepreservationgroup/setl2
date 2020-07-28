/*\
 *
 *  % MIT License
 *  %
 *  % Copyright (c) 1990 W. Kirk Snyder
 *  %
 *  % Permission is hereby granted, free of charge, to any person obtaining a copy
 *  % of this software and associated documentation files (the "Software"), to deal
 *  % in the Software without restriction, including without limitation the rights
 *  % to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  % copies of the Software, and to permit persons to whom the Software is
 *  % furnished to do so, subject to the following conditions:
 *  % 
 *  % The above copyright notice and this permission notice shall be included in all
 *  % copies or substantial portions of the Software.
 *  %
 *  % THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  % IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  % FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  % AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  % LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  % OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  % SOFTWARE.
 *  %
 *
 *  \package{\setl\ Grammar}
 *
 *  This file contains a grammar for \setl. Please note --- the purpose of
 *  this grammar is to generate LALR(1) parse tables automatically.  It
 *  is not intended as a description of \setl. Although it is possible to
 *  give an LALR(1) grammar for \setl, it is much simpler to give a grammar
 *  which accepts a superset of \setl, and use checks in the semantic
 *  actions to enforce a few syntax rules.  Major areas in which we accept
 *  a superset of \setl\ are:
 *
 *  \begin{enumerate}
 *  \item
 *  {\bf Enumerated and arithmetic tuples.}
 *  We are allowed to use enumerated tuples in left hand side contexts,
 *  and may have place-holders on the left.  Although we could enforce
 *  these rules in the grammar, it would require many rules and have
 *  implications in seemingly unrelated parts of the grammar.  Instead,
 *  we accept placeholders and general expressions on both the left and
 *  right hand sides, and expect the semantic actions to detect errors.
 *  Unfortunately, this also affects arithmetic tuple formers, in that
 *  they must also accept placeholders.
 *
 *  \item
 *  {\bf Iterators.}
 *  Unfortunately, iterators (i.e. \verb"x IN S", \verb"x = f(x)", and
 *  \verb"x = f{x}") are also valid expression terms.  This creates a
 *  conflict between enumerated set and tuple formers and general set and
 *  tuple formers.  To solve this problem, we accept general expressions
 *  as iterators, and expect semantic actions to detect errors.
 *  \end{enumerate}
 *
 *  There are several other situations in which we accept a superset of
 *  the language, but the superset is less drastic.  We have noted those
 *  areas in comments with the rules.
 *
 *  Another `trick' we use to simplify the grammar is to require the
 *  lexical analyzer to merge some operators for us.  \setl\ allows
 *  assignment operators, like C, and also has application operators.
 *  Assignment operators are binary operators followed by \verb":=" and
 *  application operators are binary operators followed by \verb"/".  We
 *  expect the lexical analyzer to look for these sequences and merge
 *  them into a single token.
\*/

/* output files */

%nonterm_desc_file = "ntdesc.h"
%rule_desc_file    = "ruledesc.h"
%action_file       = "semact.c"
%parse_table_file  = "parsetab.c"

%terminals

/* ## begin terminals */
tok_id             -> "identifier"     /* identifier                        */
tok_literal        -> "literal"        /* literal                           */
tok_and            -> "AND"            /* keyword => AND                    */
tok_assert         -> "ASSERT"         /* keyword => ASSERT                 */
tok_body           -> "BODY"           /* keyword => BODY                   */
tok_case           -> "CASE"           /* keyword => CASE                   */
tok_class          -> "CLASS"          /* keyword => CLASS                  */
tok_const          -> "CONST"          /* keyword => CONST                  */
tok_continue       -> "CONTINUE"       /* keyword => CONTINUE               */
tok_else           -> "ELSE"           /* keyword => ELSE                   */
tok_elseif         -> "ELSEIF"         /* keyword => ELSEIF                 */
tok_end            -> "END"            /* keyword => END                    */
tok_exit           -> "EXIT"           /* keyword => EXIT                   */
tok_for            -> "FOR"            /* keyword => FOR                    */
tok_if             -> "IF"             /* keyword => IF                     */
tok_inherit        -> "INHERIT"        /* keyword => INHERIT                */
tok_lambda         -> "LAMBDA"         /* keyword => LAMBDA                 */
tok_loop           -> "LOOP"           /* keyword => LOOP                   */
tok_not            -> "NOT"            /* keyword => NOT                    */
tok_null           -> "NULL"           /* keyword => NULL                   */
tok_or             -> "OR"             /* keyword => OR                     */
tok_otherwise      -> "OTHERWISE"      /* keyword => OTHERWISE              */
tok_package        -> "PACKAGE"        /* keyword => PACKAGE                */
tok_procedure      -> "PROCEDURE"      /* keyword => PROCEDURE              */
tok_process        -> "PROCESS"        /* keyword => PROCESS                */
tok_program        -> "PROGRAM"        /* keyword => PROGRAM                */
tok_rd             -> "RD"             /* keyword => RD                     */
tok_return         -> "RETURN"         /* keyword => RETURN                 */
tok_rw             -> "RW"             /* keyword => RW                     */
tok_sel            -> "SEL"            /* keyword => SEL                    */
tok_self           -> "SELF"           /* keyword => SELF                   */
tok_stop           -> "STOP"           /* keyword => STOP                   */
tok_then           -> "THEN"           /* keyword => THEN                   */
tok_until          -> "UNTIL"          /* keyword => UNTIL                  */
tok_use            -> "USE"            /* keyword => USE                    */
tok_var            -> "VAR"            /* keyword => VAR                    */
tok_when           -> "WHEN"           /* keyword => WHEN                   */
tok_while          -> "WHILE"          /* keyword => WHILE                  */
tok_wr             -> "WR"             /* keyword => WR                     */
tok_semi           -> ";"              /* ;                                 */
tok_comma          -> ","              /* ,                                 */
tok_colon          -> ":"              /* :                                 */
tok_lparen         -> "("              /* (                                 */
tok_rparen         -> ")"              /* )                                 */
tok_lbracket       -> "["              /* [                                 */
tok_rbracket       -> "]"              /* ]                                 */
tok_lbrace         -> "{"              /* {                                 */
tok_rbrace         -> "}"              /* }                                 */
tok_dot            -> "."              /* .                                 */
tok_dotdot         -> ".."             /* ..                                */
tok_assign         -> ":="             /* :=                                */
tok_suchthat       -> "suchthat"       /* |                                 */
tok_rarrow         -> "=>"             /* =>                                */
tok_assignop       -> "assignop"       /* assignment operator               */
tok_applyop        -> "applyop"        /* assignment operator               */
tok_unop           -> "unop"           /* unary operator                    */
tok_caret          -> "^"              /* pointer reference                 */
tok_addop          -> "addop"          /* addop                             */
tok_dash           -> "-"              /* -                                 */
tok_mulop          -> "mulop"          /* mulop                             */
tok_expon          -> "**"             /* **                                */
tok_relop          -> "relop"          /* relop                             */
tok_fromop         -> "fromop"         /* fromop                            */
tok_quantifier     -> "quantifier"     /* quantifier                        */
/* ## end terminals */

%rules


/*\
 *  \function{compilation units}
 *
 *  A source file consists of one or more compilation units.  We keep
 *  many tables for an entire source file in memory (although we keep
 *  intermediate files for the largest), so it would be nice if the user
 *  did not allow source files to get larger than necessary.  It is
 *  possible to have many compilation units in a file however, it just
 *  means that there is a higher risk of \verb"malloc()" failing.
\*/

   <compilation_unit_list>    ::= <compilation_unit_list> <compilation_unit>
                              |   <compilation_unit>

   <compilation_unit>         ::= <package_spec_unit>
                              |   <package_body_unit>
                              |   <class_spec_unit>
                              |   <class_body_unit>
                              |   <process_spec_unit>
                              |   <process_body_unit>
                              |   <program_unit>
                              |   %error

/*\
 *  \function{package specifications}
 *
 *  We make package specifications separate compilation units from their
 *  associated bodies, to allow cycles in the package dependency graph.
 *  A package specification has no associated code, only declarations.
\*/

   <package_spec_unit>        ::= <package_spec_header>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_declaration_part>
                                  <unit_tail>

   <package_spec_header>      ::= PACKAGE identifier ;

   <package_spec_header>      ::= PACKAGE %error ;

/*\
 *  \function{package bodies}
 *
 *  A package body may contain local data, and must contain definitions
 *  of any procedures declared in the associated specification.
\*/

   <package_body_unit>        ::= <package_body_header>
                                  <use_part>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_definition_part>
                                  <unit_tail>

   <package_body_header>      ::= PACKAGE BODY identifier ;

   <package_body_header>      ::= PACKAGE BODY %error ;

/*\
 *  \function{class specifications}
 *
 *  We use the same syntax for class specifications and class bodies that
 *  we use for package specifications and package bodies, primarily for
 *  consistency.
\*/

   <class_spec_unit>          ::= <class_spec_header>
                                  <inherit_part>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_declaration_part>
                                  <unit_tail>

   <class_spec_header>        ::= CLASS identifier ;

   <class_spec_header>        ::= CLASS %error ;

/*\
 *  \function{class bodies}
 *
 *  A class body may contain local data, and must contain definitions
 *  of any procedures declared in the associated specification.
\*/

   <class_body_unit>          ::= <class_body_header>
                                  <use_part>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_definition_part>
                                  <unit_tail>

   <class_body_header>        ::= CLASS BODY identifier ;

   <class_body_header>        ::= CLASS BODY %error ;

/*\
 *  \function{process specifications}
 *
 *  We use the same syntax for process specifications and process bodies that
 *  we use for package specifications and package bodies, primarily for
 *  consistency.
\*/

   <process_spec_unit>        ::= <process_spec_header>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_declaration_part>
                                  <unit_tail>

   <process_spec_header>      ::= PROCESS CLASS identifier ;

   <process_spec_header>      ::= PROCESS CLASS %error ;

/*\
 *  \function{process bodies}
 *
 *  A process body may contain local data, and must contain definitions
 *  of any procedures declared in the associated specification.
\*/

   <process_body_unit>        ::= <process_body_header>
                                  <use_part>
                                  <data_declaration_part>
                                  <no_body>
                                  <proc_definition_part>
                                  <unit_tail>

   <process_body_header>      ::= PROCESS CLASS BODY identifier ;

   <process_body_header>      ::= PROCESS CLASS BODY %error ;

/*\
 *  \function{programs}
 *
 *  We expect most compilation units to be programs.  These are the
 *  only units which can be directly executed by the interpreter.
\*/

   <program_unit>             ::= <program_header>
                                  <use_part>
                                  <data_declaration_part>
                                  <body>
                                  <proc_definition_part>
                                  <unit_tail>

   <program_header>           ::= PROGRAM identifier ;

   <program_header>           ::= PROGRAM %error ;

/*\
 *  \function{imported package list}
 *
 *  We use a \verb"USE" clause to import other packages.   We don't need
 *  an analog of Ada's \verb"WITH" clause, since we don't have generics.
\*/

   <use_part>                 ::= <use_clause> <use_part>
                              |   %empty

   <use_clause>               ::= USE <use_list> ;

   <use_list>                 ::= <use_list> , <use_item>
                              |   <use_item>

   <use_item>                 ::= identifier

   <use_item>                 ::= %error

/*\
 *  \function{inherited class list}
 *
 *  We use an \verb"INHERIT" clause as in Eiffel, to inherit other
 *  classes.
\*/

   <inherit_part>             ::= <inherit_clause> <inherit_part>
                              |   %empty

   <inherit_clause>           ::= INHERIT <inherit_list> ;

   <inherit_list>             ::= <inherit_list> , <inherit_item>
                              |   <inherit_item>

   <inherit_item>             ::= identifier

   <inherit_item>             ::= %error

/*\
 *  \function{procedures}
 *
 *  Procedure syntax is a different from packages, since we use them in
 *  two contexts.  There are procedure headers in package and class
 *  specifications as well as preceeding the actual procedure
 *  definitions.  Within specifications we have no declarations or
 *  bodies.
\*/

   <proc_declaration_part>    ::= <proc_declaration_list>
                              |   %empty

   <proc_declaration_list>    ::= <proc_declaration_list> <procedure_header> ;
                              |   <procedure_header> ;

   <proc_definition_part>     ::= <proc_definition_list>
                              |   %empty

   <proc_definition_list>     ::= <proc_definition_list> <procedure_unit>
                              |   <procedure_unit>

   <procedure_unit>           ::= <procedure_header> ;
                                  <data_declaration_part>
                                  <body>
                                  <proc_definition_part>
                                  <unit_tail>

   <procedure_header>         ::= <procedure_name> <parameter_part>

   <procedure_name>           ::= PROCEDURE identifier

   <procedure_name>           ::= PROCEDURE %error

   <parameter_part>           ::= ( <parameter_list> )
                              |   ( )
                              |   %empty
                              |   %error

   <parameter_list>           ::= <parameter_list> , <parameter_spec>
                              |   <parameter_spec>

   <parameter_spec>           ::= identifier

   <parameter_spec>           ::= RD identifier

   <parameter_spec>           ::= WR identifier

   <parameter_spec>           ::= RW identifier

   <parameter_spec>           ::= %error

/*\
 *  \function{methods}
 *
 *  Method headers are a bit strange.  Essentially, these methods are
 *  hooks into the \setl\ interpreter, corresponding to various syntactic
 *  constructs.  We use the syntactic construct itself as the method
 *  name, with identifiers in the operand positions as formal parameters.
\*/

   <procedure_header>         ::= <method_header>


   <method_header>            ::= PROCEDURE SELF addop identifier

   <method_header>            ::= PROCEDURE identifier addop SELF

   <method_header>            ::= PROCEDURE SELF - identifier

   <method_header>            ::= PROCEDURE identifier - SELF

   <method_header>            ::= PROCEDURE SELF mulop identifier

   <method_header>            ::= PROCEDURE identifier mulop SELF

   <method_header>            ::= PROCEDURE SELF ** identifier

   <method_header>            ::= PROCEDURE identifier ** SELF

   <method_header>            ::= PROCEDURE SELF relop identifier

   <method_header>            ::= PROCEDURE identifier relop SELF

   <method_header>            ::= PROCEDURE fromop SELF

   <method_header>            ::= PROCEDURE unop SELF

   <method_header>            ::= PROCEDURE - SELF

   <method_header>            ::= PROCEDURE SELF ( identifier )

   <method_header>            ::= PROCEDURE SELF { identifier }

   <method_header>            ::= PROCEDURE SELF ( identifier .. identifier )

   <method_header>            ::= PROCEDURE SELF ( identifier .. )

   <method_header>            ::= PROCEDURE SELF ( identifier ) := identifier

   <method_header>            ::= PROCEDURE SELF { identifier } := identifier

   <method_header>            ::= PROCEDURE SELF ( identifier .. identifier )
                                     := identifier

   <method_header>            ::= PROCEDURE SELF ( identifier .. )
                                     := identifier

/*\
 *  \function{tails}
 *
 *  Unit tails may optionally include the unit name.
\*/

   <unit_tail>                ::= END <optional_unit_name> ;

   <optional_unit_name>       ::= identifier

   <optional_unit_name>       ::= %error

   <optional_unit_name>       ::= %empty

/*\
 *  \function{data declarations}
 *
 *  We allow four forms of data declarations: variables, class variables,
 *  constants, and selectors.
\*/

   <data_declaration_part>    ::= <data_declaration_list>
                              |   %empty

   <data_declaration_list>    ::= <data_declaration_list> <data_declaration> ;
                              |   <data_declaration> ;

   <data_declaration>         ::= <const_clause>
                              |   <sel_clause>
                              |   <var_clause>
                              |   <class_var_clause>

/*\
 *  \function{constant declarations}
 *
 *  Constants are a little misleading.  They are constant in the sense
 *  that the programmer may not change their values in the code body, but
 *  are unusual in that constant values are formed at run time, during
 *  program load rather than by the compiler.  This is because constants
 *  may assume set or tuple values, and these objects are formed by the
 *  interpreter in dynamic memory, so it's much easier to defer
 *  evaluating constants until run time.
\*/

   <const_clause>             ::= CONST <const_declaration_list>

   <const_declaration_list>   ::= <const_declaration_list> ,
                                  <const_declaration>
                              |   <const_declaration>

   <const_declaration>        ::= identifier := <expression>

   <const_declaration>        ::= %error

/*\
 *  \function{selector declarations}
 *
 *  Selectors are unusual, and completely absent in SETL.  SETL provides
 *  macros, which are used to name components of tuples, among other
 *  things.  We do not provide macros in \setl, in the belief that they
 *  degrade readability rather than enhance it if used without
 *  restriction.  However, it is still nice to be able to name the
 *  components of a tuple.  Selectors are the mechanism we use to
 *  facilitate that.
\*/

   <sel_clause>               ::= SEL <sel_declaration_list>

   <sel_declaration_list>     ::= <sel_declaration_list> , <sel_declaration>
                              |   <sel_declaration>

   <sel_declaration>          ::= identifier ( literal )

   <sel_declaration>          ::= identifier ( %error )

   <sel_declaration>          ::= %error

/*\
 *  \function{variable declarations}
 *
 *  Variable declarations are identical to those in SETL, except that we
 *  allow initialization expressions, but not \verb"INIT" clauses.  We do
 *  not have type declarations, but the user can (and possibly must,
 *  depending on compiler option) declare variables.
\*/

   <var_clause>               ::= VAR <var_declaration_list>

   <var_declaration_list>     ::= <var_declaration_list> , <var_declaration>
                              |   <var_declaration>

   <var_declaration>          ::= identifier

   <var_declaration>          ::= identifier := <expression>

   <var_declaration>          ::= %error

/*\
 *  \function{class variable declarations}
 *
 *  Class variables are similar to variables, but within classes they are
 *  common to all instances of a class.
\*/

   <class_var_clause>         ::= CLASS VAR <class_var_decl_list>

   <class_var_decl_list>      ::= <class_var_decl_list> , <class_var_decl>
                              |   <class_var_decl>

   <class_var_decl>           ::= identifier

   <class_var_decl>           ::= identifier := <expression>

   <class_var_decl>           ::= %error

/*\
 *  \function{statement and expression lists}
 *
 *  At various places in the grammar we collect lists of statements or
 *  expressions.  These rules just provide a mechanism for doing that.
\*/

   <no_body>                  ::= %empty

   <body>                     ::= %empty

   <body>                     ::= <statement_list>

   <statement_list>           ::= <statement_list> <stmt_or_expression> ;

   <statement_list>           ::= <stmt_or_expression> ;

   <stmt_or_expression>       ::= <statement>
                              |   <expression>

   <expression_list>          ::= <expression_list> , <expression>

   <expression_list>          ::= <expression>

/*\
 *  \function{assignment expressions}
 *
 *  \setl\ is a little more of an expression language than SETL. We treat
 *  statements as expressions, and allow them to be used interchangeably.
 *  This is the first of several expressions which would more commonly be
 *  used as statements.
\*/

   <expression>               ::= <left_term> := <expression>

   <expression>               ::= <left_term> assignop <expression>

/*\
 *  \function{loop expressions}
 *
 *  We have abandoned SETL syntax for loops, since we find it somewhat
 *  archaic.  Loop syntax in \setl\ combines the iterators of SETL with more
 *  modern control structures, such as those in Ada.
\*/

   <primary>                  ::= FOR <for_iterator> LOOP
                                     <statement_list>
                                  END LOOP

   <for_iterator>             ::= <expression_list>

   <for_iterator>             ::= <expression_list> suchthat <expression>

   <primary>                  ::= WHILE <expression> LOOP
                                     <statement_list>
                                  END LOOP

   <primary>                  ::= UNTIL <expression> LOOP
                                     <statement_list>
                                  END LOOP

   <primary>                  ::= LOOP
                                     <statement_list>
                                  END LOOP

/*\
 *  \function{if expressions}
 *
 *  Since most statements are also valid expressions, and vice versa, we
 *  have only one set of rules for if expressions.  They are fairly
 *  conventional.
\*/

   <primary>                  ::= IF <expression> THEN
                                     <statement_list>
                                     <opt_else_stmt_clause>
                                  END IF

   <opt_else_stmt_clause>     ::= ELSEIF <expression> THEN
                                     <statement_list>
                                     <opt_else_stmt_clause>

   <opt_else_stmt_clause>     ::= ELSE <statement_list>

   <opt_else_stmt_clause>     ::= %empty

   <primary>                  ::= IF <expression> THEN
                                     <expression>
                                     <opt_else_expr_clause>
                                  END IF

   <opt_else_expr_clause>     ::= ELSEIF <expression> THEN
                                     <expression>
                                     <opt_else_expr_clause>

   <opt_else_expr_clause>     ::= ELSE <expression>

   <opt_else_expr_clause>     ::= %empty

/*\
 *  \function{case and guarded expressions}
 *
 *  \setl\ has two forms of multi-way branches. The case expression is a
 *  standard case statement, with one exception: we do not insist that
 *  the expressions associated with each clause be constant.  The
 *  expressions are evaluated each time the case is executed.
 *
 *  The guarded statement chooses non-deterministically among the clauses
 *  until it finds one which is true.  Then it executes the associated
 *  body and exits.
\*/

   <primary>                  ::= CASE <expression>
                                     <case_when_stmt_list>
                                     <guard_case_stmt_default>
                                  END CASE

   <case_when_stmt_list>      ::= <case_when_stmt_list>
                                  <case_when_stmt_item>

   <case_when_stmt_list>      ::= <case_when_stmt_item>

   <case_when_stmt_item>      ::= WHEN <expression_list> =>
                                     <statement_list>

   <primary>                  ::= CASE
                                     <guard_when_stmt_list>
                                     <guard_case_stmt_default>
                                  END CASE

   <guard_when_stmt_list>     ::= <guard_when_stmt_list>
                                  <guard_when_stmt_item>

   <guard_when_stmt_list>     ::= <guard_when_stmt_item>

   <guard_when_stmt_item>     ::= WHEN <expression> =>
                                     <statement_list>

   <guard_case_stmt_default>  ::= OTHERWISE =>
                                     <statement_list>

   <guard_case_stmt_default>  ::= %empty

   <primary>                  ::= CASE <expression>
                                     <case_when_expr_list>
                                     <guard_case_expr_default>
                                  END CASE

   <case_when_expr_list>      ::= <case_when_expr_list>
                                  <case_when_expr_item>

   <case_when_expr_list>      ::= <case_when_expr_item>

   <case_when_expr_item>      ::= WHEN <expression_list> =>
                                     <expression>

   <primary>                  ::= CASE
                                     <guard_when_expr_list>
                                     <guard_case_expr_default>
                                  END CASE

   <guard_when_expr_list>     ::= <guard_when_expr_list>
                                  <guard_when_expr_item>

   <guard_when_expr_list>     ::= <guard_when_expr_item>

   <guard_when_expr_item>     ::= WHEN <expression> =>
                                     <expression>

   <guard_case_expr_default>  ::= OTHERWISE =>
                                     <expression>

   <guard_case_expr_default>  ::= %empty

/*\
 *  \function{miscellaneous statement-like expressions}
 *
 *  We have various expressions which are generally used as statements
 *  rather than as expressions.  The semantic actions associated with
 *  these rules are trivial.
\*/

   <statement>                ::= STOP

   <statement>                ::= RETURN

   <statement>                ::= RETURN <expression>

   <statement>                ::= EXIT

   <statement>                ::= EXIT <expression>

   <statement>                ::= EXIT WHEN <expression>

   <statement>                ::= EXIT <expression> WHEN <expression>

   <statement>                ::= CONTINUE

   <statement>                ::= CONTINUE WHEN <expression>

   <statement>                ::= ASSERT <expression>

   <statement>                ::= NULL

/*\
 *  \function{lambda expressions}
 *
 *  $\lambda$-expressions are constant expressions which yield a
 *  procedure value.  They look a lot like procedure definitions.
\*/

   <primary>                  ::= <lambda_header> ;
                                  <data_declaration_part>
                                  <body>
                                  <proc_definition_part>
                                  END LAMBDA

   <lambda_header>            ::= <lambda_key> <lambda_parameters>

   <lambda_key>               ::= LAMBDA

   <lambda_parameters>        ::= ( <lambda_param_list> )
                              |   ( )
                              |   %empty
                              |   %error

   <lambda_param_list>        ::= <lambda_param_list> , <lambda_param_spec>
                              |   <lambda_param_spec>

   <lambda_param_spec>        ::= identifier

   <lambda_param_spec>        ::= %error

/*\
 *  \function{operators}
 *
 *  The following list of rules enforce normal operator precedence.
\*/

   <expression>               ::= quantifier <expression_list>
                                     suchthat <expression>

   <expression>               ::= <left_term> fromop <left_term>

   <expression>               ::= <and_expression>
                              |   <or_expression>
                              |   <not_term>

   <and_expression>           ::= <and_term> AND <not_term>

   <and_term>                 ::= <and_term> AND <not_term>

   <and_term>                 ::= <not_term>

   <or_expression>            ::= <or_term> OR <not_term>

   <or_term>                  ::= <or_term> OR <not_term>

   <or_term>                  ::= <not_term>

   <not_term>                 ::= NOT <relop_term>

   <not_term>                 ::= <relop_term>

   <relop_term>               ::= <relop_term> relop <addop_term>

   <relop_term>               ::= <addop_term>

   <addop_term>               ::= <addop_term> addop <mulop_term>

   <addop_term>               ::= <addop_term> - <mulop_term>

   <addop_term>               ::= <mulop_term>

   <mulop_term>               ::= <mulop_term> mulop <expon_term>

   <mulop_term>               ::= <mulop_term> applyop <expon_term>

   <mulop_term>               ::= <expon_term>

   <expon_term>               ::= <left_term> ** <expon_term>

   <expon_term>               ::= <left_term>

   <left_term>                ::= unop <left_term>

   <left_term>                ::= - <left_term>

   <left_term>                ::= ^ <left_term>

   <left_term>                ::= applyop <left_term>

   <left_term>                ::= <primary>

/*\
 *  \function{primaries}
 *
 *  We think of a {\em primary} as some primitive operand.  It can
 *  include map, tuple, and string references, identifiers, literals, and
 *  anything else that any operator can operate upon.
\*/

   <primary>                  ::= literal

   <primary>                  ::= { <expression_list> }

   <primary>                  ::= { <expression_list> .. <expression> }

   <primary>                  ::= { <iterator_expression> suchthat
                                    <expression> }

   <primary>                  ::= { <expression> :
                                    <expression_list> suchthat
                                    <expression> }

   <primary>                  ::= { <expression> : <expression_list> }

   <primary>                  ::= { }

   <primary>                  ::= [ <tuple_list> .. <expression> ]

   <primary>                  ::= [ <iterator_expression> suchthat
                                    <expression> ]

   <primary>                  ::= [ <expression> :
                                    <expression_list> suchthat
                                    <expression> ]

   <primary>                  ::= [ <expression> : <expression_list> ]

   <primary>                  ::= [ ]

   <iterator_expression>      ::= <expression>

   <primary>                  ::= ( <expression> )

/*\
 *  \function{left hand sides}
 *
 *  This is an attempt to isolate as many legitimate left hand sides as
 *  possible.  We do not try to completely control left hand sides in
 *  the grammar, but rely on semantic checks as well.
\*/

   <primary>                  ::= identifier

   <primary>                  ::= SELF

   <primary>                  ::= <primary> ( <expression_list> )

   <primary>                  ::= <primary> ( <expression> .. <expression> )

   <primary>                  ::= <primary> ( <expression> .. )

   <primary>                  ::= <primary> ( )

   <primary>                  ::= <primary> { <expression_list> }

   <primary>                  ::= <primary> . identifier

   <primary>                  ::= [ <tuple_list> ]

   <primary>                  ::= %error

   <tuple_list>               ::= <tuple_list> , <tuple_element>

   <tuple_list>               ::= <tuple_element>

   <tuple_element>            ::= <expression>

   <tuple_element>            ::= -

%end

