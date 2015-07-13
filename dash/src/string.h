#ifndef dash_string_h
#define dash_string_h

char *str_end(char *str)
{
	if (!str)
		return NULL;
	char *runner = str;
	while (*runner)
		++runner;
	return runner;
}
char *str_search_first_of(char *str, const char delim)
{
	if (!str)
		return NULL;
	while (*str)
	{
		if (*str == delim)
			return str;
		++str;
	}
	return NULL;
}
int str_starts_with(const char *string, const char *prefix)
{
	size_t	lenpre = strlen(prefix),
		lenstr = strlen(string);

	return lenstr < lenpre ? 0 : strncmp(prefix, string, lenpre) == 0;
}
int str_is_whitespace(const char *string)
{
	/*this should use isspace but cctype wasn't working on msvc for who knows why*/
	while (*string)
	{
		if (!(*string == ' ' || *string == '\r' || *string == '\n' || *string == '\t'))
			return 0;

		++string;
	}
	return 1;
}

typedef struct
{
	char* end;
	char* marker;
	char delim;
} tokenizing_data;

void tokenize_init(char* str, char delim, tokenizing_data *out)
{
	out->end = str_end(str);
	out->marker = str;
	out->delim = delim;
}
char *tokenize_step(tokenizing_data *data)
{
	char *next_delim = str_search_first_of(data->marker, data->delim);
	char *search_start = data->marker;

	if (data->marker == data->end)
	{
		return NULL;
	}
	if (next_delim == NULL)
	{
		data->marker = data->end;
		return search_start;
	}

	data->marker = next_delim + 1;
	*next_delim = 0;

	return search_start;
}

#endif