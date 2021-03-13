//
// parse.c - implementation of functions such as tokenize input_string into argv, get _next SIMPLE_WORD into a string
// Created by kannav on 9/1/20.
//

#include "parse.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NULL_TERMINATE(x, y) x[(y)++] = '\0'

// argv is pointer ot a 2-D array
int split_into_commands(char ***argv, char *inp) {
  register int i = 0;
  int buffer_size = 10;
  *argv = (char **) malloc(buffer_size * sizeof(char *));

  for (char *token = strtok(inp, ";"); token != NULL; token = strtok(NULL, ";")) {
	(*argv)[i++] = token;
	if (i == buffer_size) {
	  buffer_size += 10;
	  *argv = (char **) realloc(*argv, buffer_size * sizeof(char *));
	}
  }
  return i;
}

enum ParserState {
  INIT,
  SIMPLE_WORD,
  SINGLE_QUOTE,
  DOUBLE_QUOTE,
  SPACE
};

static int i = 0;    // number of parsed tokens in current command line
static int l = 0;
static char *input;        // The input source
static struct token *curr_token = (struct token *) NULL;

static void load_token(struct token *tok, char *string, enum TokenType state) {
  size_t len = strlen(string);
  tok->_text = (char *) malloc(len + 1);
  strncpy(tok->_text, string, len);
  tok->_text[len] = '\0';
  tok->_type = state;
}

void free_token(void) {
  if (curr_token != NULL) {
	if (curr_token->_text != NULL) {
	  free(curr_token->_text);
	}
	free(curr_token);
  }
}

struct token *get_next_token(char *line) {
  enum ParserState curr_state = INIT;
  char c;

  if (line != NULL) { // beginning a new command
	i = 0;
	input = line;
	l = (int) strlen(input) + 1;
  } else {
	if (curr_token != NULL) {
	  if (curr_token->_text != NULL) {
		free(curr_token->_text);
	  }
	  free(curr_token);
	}
  }

  curr_token = (struct token *) malloc(sizeof(struct token));
  curr_token->_text = NULL;

  char *starting_pos = input + i;
  if (i == l) {
	return NULL;
  }

  while (i < l) {
	c = input[i];
	if (isspace(c)) {
	  if (curr_state != SINGLE_QUOTE && curr_state != DOUBLE_QUOTE) {
		NULL_TERMINATE(input, i);
		if (strlen(starting_pos) > 0) {
		  load_token(curr_token, starting_pos, STRING);
		  return curr_token;
		}
	  }
	} else if (c == '"') {
	  if (curr_state == DOUBLE_QUOTE) {
		NULL_TERMINATE(input, i);
		if (strlen(starting_pos) > 0) {
		  load_token(curr_token, starting_pos, STRING);
		  return curr_token;
		}
	  } else if (curr_state != SINGLE_QUOTE) {
		curr_state = DOUBLE_QUOTE;
		starting_pos = input + i + 1;
		input[i] = '\0';
	  }
	} else if (c == '\'') {
	  if (curr_state == SINGLE_QUOTE) {
		NULL_TERMINATE(input, i);
		if (strlen(starting_pos) > 0) {
		  load_token(curr_token, starting_pos, STRING);
		  return curr_token;
		}
	  } else if (curr_state != DOUBLE_QUOTE) {
		curr_state = SINGLE_QUOTE;
		starting_pos = input + i + 1;
		input[i] = '\0';
	  }
	} else if (curr_state != SINGLE_QUOTE && curr_state != DOUBLE_QUOTE) {
	  char str[2] = "\0";
	  switch (c) {
	  case '&':
	  case '<':
	  case '|': str[0] = c;
		load_token(curr_token, str, SYMBOL);
		NULL_TERMINATE(input, i);
		return curr_token;
	  case '>':
		if (input[i + 1] == '>') {
		  load_token(curr_token, ">>", SYMBOL);
		  NULL_TERMINATE(input, i);
		  return curr_token;
		} else {
		  load_token(curr_token, ">", SYMBOL);
		  NULL_TERMINATE(input, i);
		  return curr_token;
		}
	  case '\0': load_token(curr_token, starting_pos, STRING);
		NULL_TERMINATE(input, i);
		return curr_token;
	  default:
		if (curr_state == INIT) {
		  curr_state = SIMPLE_WORD;
		  starting_pos = input + i;
		}
		i++;
		break;
	  }
	} else {
	  i++;
	}
  }

  load_token(curr_token, starting_pos, STRING);
  if (strlen(curr_token->_text) > 0) {
	return curr_token;
  }
  return NULL;
}

