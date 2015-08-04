/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_PARSER_H_INCLUDED
# define YY_YY_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOKEN_INTEGER = 258,
    TOKEN_REAL = 259,
    TOKEN_IDENTIFIER = 260,
    TOKEN_TYPE = 261,
    TOKEN_DEF = 262,
    TOKEN_ARROW = 263,
    TOKEN_LET = 264,
    TOKEN_WHILE = 265,
    TOKEN_IF = 266,
    TOKEN_ELSE = 267,
    TOKEN_RETURN = 268,
    TOKEN_OP_ADD = 269,
    TOKEN_OP_SUB = 270,
    TOKEN_OP_MUL = 271,
    TOKEN_OP_DIV = 272,
    TOKEN_OP_LESS = 273,
    TOKEN_OP_LESS_EQ = 274,
    TOKEN_OP_GREATER = 275,
    TOKEN_OP_GREATER_EQ = 276
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 17 "parser.y" /* yacc.c:1909  */

	dst_type			 type;
	dst_type_list		*type_list;
	char				*identifier;
	dst_id_list			*identifier_list;
	dst_statement		*statement;
	dst_statement_list	*statement_list;
	dst_exp				*expression;
	dst_exp_list		*expression_list;
	dst_proc_param		*proc_param;
	dst_proc_param_list	*proc_param_list;
	dst_proc			*proc; 
	dst_proc_list		*proc_list;

	int			 integer;
	float		 real;
	char		*string;

#line 95 "parser.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int yyparse (dst_proc_list **parsed_module, void *scanner);

#endif /* !YY_YY_PARSER_H_INCLUDED  */
