#include "common.h"
#include "frontend/parser.h"
#include "backend/codegen.h"

#include <stdio.h>

typedef void *yyscan_t;
int yylex_init(yyscan_t * ptr_yy_globals);
int yylex(union YYSTYPE *yyval_param, struct YYLTYPE *yylloc_param, yyscan_t yyscanner);
int yylex_destroy(yyscan_t yyscanner);
void yyset_in(FILE *in, yyscan_t yyscanner);
void yyset_extra(dsc_memory *memory, yyscan_t yyscanner);

dcg_proc_decl_list *import_stdlib_decls(dsc_memory *mem)
{
	dcg_proc_decl_list *stdlib = NULL;

	stdlib = dcg_append_proc_decl_list(
		stdlib,
		dcg_create_proc_decl("print_c",
			dst_append_func_param_list(NULL, dst_create_proc_param("val", dst_type_integer, mem), mem),
			NULL,
			0, mem),
		mem
		);

	stdlib = dcg_append_proc_decl_list(
		stdlib,
		dcg_create_proc_decl("print_i",
			dst_append_func_param_list(NULL, dst_create_proc_param("val", dst_type_integer, mem), mem),
			NULL,
			1, mem),
		mem
		);

	stdlib = dcg_append_proc_decl_list(
		stdlib,
		dcg_create_proc_decl("print_r",
			dst_append_func_param_list(NULL, dst_create_proc_param("val", dst_type_real, mem), mem),
			NULL,
			2, mem),
		mem
		);

	stdlib = dcg_append_proc_decl_list(
		stdlib,
		dcg_create_proc_decl("sin",
			dst_append_func_param_list(NULL, dst_create_proc_param("x", dst_type_real, mem), mem),
			dst_append_type_list(NULL, dst_type_real, mem),
			3, mem),
		mem
		);

	stdlib = dcg_append_proc_decl_list(
		stdlib,
		dcg_create_proc_decl("cos",
			dst_append_func_param_list(NULL, dst_create_proc_param("x", dst_type_real, mem), mem),
			dst_append_type_list(NULL, dst_type_real, mem),
			4, mem),
		mem
		);

	stdlib = dcg_append_proc_decl_list(
		stdlib,
		dcg_create_proc_decl("tan",
			dst_append_func_param_list(NULL, dst_create_proc_param("x", dst_type_real, mem), mem),
			dst_append_type_list(NULL, dst_type_real, mem),
			5, mem),
		mem
		);

	stdlib = dcg_append_proc_decl_list(
		stdlib,
		dcg_create_proc_decl("pow",
			dst_append_func_param_list(
				dst_append_func_param_list(NULL, dst_create_proc_param("x", dst_type_real, mem), mem),
				dst_create_proc_param("y", dst_type_real, mem), mem),
			dst_append_type_list(NULL, dst_type_real, mem),
			6, mem),
		mem
		);

	return stdlib;
}

int dvm_import_source(const char *source_file, struct dvm_context *context)
{
	srand((unsigned int)time(NULL));

	yyscan_t scanner;
	if (yylex_init(&scanner) != 0)
	{
		dsc_error_internal();
		return 0;
	}

	FILE *source = fopen(source_file, "rb");
	if (source == NULL)
	{
		yylex_destroy(scanner);

		dsc_error("couldn't open source file %s", source_file);
		return 0;
	}
	yyset_in(source, scanner);

	dsc_memory mem;
	if (!dsc_create(0, &mem))
	{
		fclose(source);
		yylex_destroy(scanner);

		dsc_error_oom();
		return 0;
	}

	dcg_proc_decl_list *stdlib = import_stdlib_decls(&mem);
		
	dsc_parse_context parse;
	parse.memory = &mem;
	parse.parsed_module = NULL;

	yyset_extra(&mem, scanner);

	if (yyparse(&parse, scanner) == 0)
	{
		if (!dcg_import_procedure_list(parse.parsed_module, stdlib, context, &mem))
		{
			dsc_destroy(&mem);

			fclose(source);
			yylex_destroy(scanner);

			return 0;
		}
		else
		{
			dsc_destroy(&mem);

			fclose(source);
			yylex_destroy(scanner);

			return 1;
		}
	}

	dsc_destroy(&mem);

	fclose(source);
	yylex_destroy(scanner);
	
	return 0;
}