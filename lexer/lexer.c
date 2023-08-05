#include <ctype.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "lexer.h"
#include "../global/defines.h"
#include "../data_structures/trie/trie.h"


static int is_trie_inited = FALSE;

Trie op_lookup = NULL;
Trie dir_lookup = NULL;

static struct op_map{
    const char *op_name;
    int key;
    const char *src_options;
    const char *dst_options;
} op_map[16] = {
    {"mov",op_mov, "ILR", "LR"},
    {"cpm",op_cmp, "ILR", "ILR"},
    {"add",op_add, "ILR", "LR"},
    {"sub",op_sub, "ILR", "LR"},
    {"lea",op_lea, "L", "LR"},

    {"not",op_not, NULL, "LR"},
    {"clr",op_clr, NULL, "LR"},
    {"inc",op_inc, NULL, "LR"},
    {"dec",op_dec, NULL, "LR"},
    {"jmp",op_jmp, NULL, "LR"},
    {"bne",op_bne, NULL, "LR"},
    {"red",op_red, NULL, "LR"},
    {"prn",op_prn, NULL, "ILR"},
    {"jsr",op_jsr, NULL, "LR"},

    {"rts",op_rts, NULL, NULL},
    {"stop",op_stop, NULL, NULL},
};


static struct dir_map{
    const char *dir_name;
    int key;
} dir_map[4] = {
    {"data", dir_data},
    {"string", dir_string},
    {"entry", dir_entry},
    {"extern", dir_extern},
};


typedef enum{
    label_ok,
    start_without_alpha,
    contains_non_alpha_numeric,
    label_is_too_long
}valid_label_err;


static void lexer_trie_init();
static valid_label_err is_valid_label(const char *label);
static void parse_operation_operands(assembler_ast * ast, char * operands_string, struct op_map * op_map_ptr);
static char parse_operand(char * operand_string, char ** label, int * const_number, int * register_number);
static int parse_num(char * num_string, char ** endptr, long * num, long min, long max);

assembler_ast line_to_ast_lexer(char *line){
    assembler_ast ast ={0};
    valid_label_err label_status;
    struct op_map *op_map_ptr;
    struct dir_map *dir_map_ptr;
    char *aux1, *aux2;
    if(!is_trie_inited){
        lexer_trie_init();
    }
    SKIP_SPACES(line);
    aux1 = strchr(line, ':');
    if(aux1){
        aux2 = strchr(aux1+1, ':');
        if(aux2){
            ast.line_type = syntax_error;
            strcpy(ast.error_msg, "too many ':'");
        }
        (*aux1) = '\0';
        switch(label_status = is_valid_label(line)){
            case label_ok: 
                strcpy(ast.label_name, line);
                break;
            case start_without_alpha:
                sprintf(ast.error_msg, "label '%s' must start with an alpha character", line);
                break;
            case contains_non_alpha_numeric:
                sprintf(ast.error_msg, "label '%s' contains non alpha numeric characters", line);
                break;
            case label_is_too_long:
                sprintf(ast.error_msg, "label '%s' is too long, max length is %d", line, MAX_SYMBOL_LENGTH);
                break;
        }
        if(label_status != label_ok){
            ast.line_type = syntax_error;
            return ast;
        }
        line = aux1+1;
        SKIP_SPACES(line);
    }
    if(*line == '\0' && ast.label_name[0] != '\0'){
        sprintf(ast.error_msg, "label '%s' must be followed by a command", ast.label_name);
        ast.line_type = syntax_error;
        return ast;
    }
    aux1 = strpbrk(line, SPACE_CHARS);
    if(aux1){
        *aux1 = '\0';
    }
    if(*line == '.'){
        dir_map_ptr = trie_exists(dir_lookup, line+1);
    }
    if(!dir_map_ptr){
        op_map_ptr = trie_exists(op_lookup, line);
    }
    if(!op_map_ptr){
        sprintf(ast.error_msg, "unknown command '%s'", line);
        ast.line_type = syntax_error;
        return ast;
    }

    line = aux1+1;
    SKIP_SPACES(line);

    if(op_map_ptr){
        ast.line_type = op;
        ast.op_or_dir.op_line.op_type = op_map_ptr->key;
        parse_operation_operands(&ast, line, op_map_ptr);
    }
    if(dir_map_ptr){
        ast.line_type = dir;
        ast.op_or_dir.dir_line.dir_type = dir_map_ptr->key;
    }
}

static void lexer_trie_init(){
    int i;
    op_lookup = trie();
    for(i=0;i<16;i++){
        trie_insert(op_lookup, op_map[i].op_name, &op_map[i]);
    }
    for(i=0;i<4;i++){
        trie_insert(dir_lookup, dir_map[i].dir_name, &dir_map[i]);
    }
    is_trie_inited = TRUE;
}

static valid_label_err is_valid_label(const char *label){
    int char_count = 0;
    if(!isalpha(*label)){
        return start_without_alpha;
    }
    label++;
    while(*label && isalnum(*label)){
        label++;
        char_count++;
    }
    if(*label != '\0'){
        return contains_non_alpha_numeric;
    }
    if(char_count > MAX_SYMBOL_LENGTH){
        return label_is_too_long;
    }
    return label_ok;
}

static void parse_operation_operands(assembler_ast * ast, char * operands_string, struct op_map * op_map_ptr){
    int operands_count = 0;
    char operand_type;
    char *aux1;
    char * sep = strchr(operands_string, ',');
    
    if(sep){
        aux1 - strchr(sep+1, ',');
        if(aux1){
            sprintf(ast->error_msg, "too many ','");
            return;
        }
        if(op_map_ptr->src_options == NULL){
            sprintf(ast->error_msg, "command '%s' does not support source operand", op_map_ptr->op_name);
            return;
        }
        operand_type = parse_operand(operands_string, &ast->op_or_dir.op_line.op_content[0].label_name,&ast->op_or_dir.op_line.op_content[0].const_num,&ast->op_or_dir.op_line.op_content[0].reg_num);
        if(operand_type == 'N'){
            sprintf(ast->error_msg, "invalid operand '%s' for command '%s'", operands_string, op_map_ptr->op_name);
            return;
        }
        if(strchr(op_map_ptr->src_options, operand_type) == NULL){
            sprintf(ast->error_msg, "operand '%s' is not supported for command '%s'", operands_string, op_map_ptr->op_name);
            return;
        }
        operands_string = sep+1;
        operand_type = parse_operand(operands_string, &ast->op_or_dir.op_line.op_content[1].label_name,&ast->op_or_dir.op_line.op_content[1].const_num,&ast->op_or_dir.op_line.op_content[1].reg_num);
        if(operand_type == 'N'){
            sprintf(ast->error_msg, "invalid operand '%s' for command '%s'", operands_string, op_map_ptr->op_name);
            return;
        }
        if(strchr(op_map_ptr->dst_options, operand_type) == NULL){
            sprintf(ast->error_msg, "operand '%s' is not supported for command '%s'", operands_string, op_map_ptr->op_name);
            return;
        }

    } else {
        if(op_map_ptr->src_options != NULL){
            sprintf(ast->error_msg, "command '%s' requires source operand", op_map_ptr->op_name);
            return;
        }
        operand_type = parse_operand(operands_string, &ast->op_or_dir.op_line.op_content[1].label_name,&ast->op_or_dir.op_line.op_content[1].const_num,&ast->op_or_dir.op_line.op_content[1].reg_num);
        if(operand_type != 'E' && op_map_ptr->dst_options == NULL){
            sprintf(ast->error_msg, "command '%s' does not support operands", op_map_ptr->op_name);
            return;
        }
        if(operand_type == 'E'){
            sprintf(ast->error_msg, "command '%s' expects one operand",op_map_ptr->op_name);
            return;
        }
        if(strchr(op_map_ptr->dst_options, operand_type) == NULL){
            sprintf(ast->error_msg, "operand '%s' is not supported for command '%s'", operands_string, op_map_ptr->op_name);
            return;
        }
    }

}

static char parse_operand(char * operand_string, char ** label, int * const_number, int * register_number){
    char *temp;
    SKIP_SPACES(operand_string);
    long num;
    if(*operand_string == '\0'){
        return 'E';
    }
    if(*operand_string == '@'){
        if(*(operand_string+1) == 'r'){
            if(*(operand_string+2) == '+' || *(operand_string+2) == '-'){
                return 'N';
            }
            if(parse_num(operand_string+2, NULL, &num, MIN_REG, MAX_REG) != 0){
                return 'N';
            }
            *register_number = (int)num;
            return 'R';
        }
        return 'N';
    }
    if(parse_num(operand_string, NULL, &num, MIN_CONST_NUM, MAX_CONST_NUM)){
        
    }
}

static int parse_num(char * num_string, char ** endptr, long * num, long min, long max){
    errno = 0;
    char * my_end;
    *num = strtol(num_string, &my_end, 10);
    while(isspace(*my_end)){
        my_end++;
    }
    if(*my_end != '\0'){
        return -1;
    }
    if(errno == ERANGE){
        return -2;
    }
    if(*num < min || *num > max){
        return -3;
    }
    if(endptr){
        *endptr = my_end;
    }
    return 0;
    
}






