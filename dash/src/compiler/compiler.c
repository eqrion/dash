#include "ast.h"

#include "parser.h"

#include <stdio.h>

typedef void *yyscan_t;
int yylex_init(yyscan_t * ptr_yy_globals);
int yylex(union YYSTYPE *yyval_param, struct YYLTYPE *yylloc_param, yyscan_t yyscanner);
int yylex_destroy(yyscan_t yyscanner);
void yyset_in(FILE *in, yyscan_t yyscanner);

void test_tokenize(yyscan_t scanner)
{
	YYSTYPE yylval;
	YYLTYPE yylloc;

	int token = 0;

	printf("$begin ");

	while ((token = yylex(&yylval, &yylloc, scanner)) != 0)
	{
		switch (token)
		{
		case TOKEN_INTEGER:
			printf("int(%d)", yylval.integer);
			break;
		case TOKEN_REAL:
			printf("real(%f)", yylval.real);
			break;
		case TOKEN_IDENTIFIER:
			printf("id(%s)", yylval.string);
			break;
		case TOKEN_TYPE:
			printf("type(%d)", yylval.type);
			break;
		case TOKEN_DEF:
			printf("def");
			break;
		case TOKEN_ARROW:
			printf("->");
			break;
		case TOKEN_LET:
			printf("let");
			break;
		case TOKEN_WHILE:
			printf("while");
			break;
		case TOKEN_IF:
			printf("if");
			break;
		case TOKEN_ELSE:
			printf("else");
			break;
		default:
			printf("%c", token);
			break;
		}

		printf(" ");
	}

	printf("$end");
}

int dsh_compile_source(const char *source_file, const char *out_file)
{
	yyscan_t scanner;
	if (yylex_init(&scanner) != 0)
	{
		fprintf(stderr, "couldn't init lex.\n");
		return 1;
	}

	FILE *source = fopen(source_file, "rb");
	if (source == NULL)
	{
		yylex_destroy(scanner);

		fprintf(stderr, "couldn't open source file %s\n", source_file);
		return 1;
	}
	yyset_in(source, scanner);

	//yydebug = 1;
	dsh_func_list *module = NULL;
	if (yyparse(&module, scanner) == 0)
	{
		dsh_print_func_list(module);
	}
	dsh_dealloc_func_list(module);

	//test_tokenize(scanner);

	fclose(source);
	yylex_destroy(scanner);

	return 0;
}