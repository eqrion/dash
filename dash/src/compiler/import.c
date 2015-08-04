#include "ast.h"
#include "frontend/parser.h"
#include "backend/codegen.h"

#include <stdio.h>

typedef void *yyscan_t;
int yylex_init(yyscan_t * ptr_yy_globals);
int yylex(union YYSTYPE *yyval_param, struct YYLTYPE *yylloc_param, yyscan_t yyscanner);
int yylex_destroy(yyscan_t yyscanner);
void yyset_in(FILE *in, yyscan_t yyscanner);

int dvm_import_source(const char *source_file, struct dvm_context *context)
{
	// seed the error code generator
	srand((unsigned int)time(NULL));

	yyscan_t scanner;
	if (yylex_init(&scanner) != 0)
	{
		fprintf(stderr, "couldn't init lex.\n");
		return 0;
	}

	FILE *source = fopen(source_file, "rb");
	if (source == NULL)
	{
		yylex_destroy(scanner);

		fprintf(stderr, "couldn't open source file %s\n", source_file);
		return 0;
	}
	yyset_in(source, scanner);

	//yydebug = 1;
	dst_proc_list *module = NULL;

	if (yyparse(&module, scanner) == 0)
	{
		dst_print_func_list(module);

		if (!dcg_import_procedure_list(module, context))
		{
			dst_destroy_func_list(module);

			fclose(source);
			yylex_destroy(scanner);

			return 0;
		}
		else
		{
			dst_destroy_func_list(module);

			fclose(source);
			yylex_destroy(scanner);

			return 1;
		}
	}

	fclose(source);
	yylex_destroy(scanner);
	
	return 0;
}