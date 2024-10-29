/* source/lm_parser.c
 *
 * MIT License
 *
 * Copyright(c) 2023-present @ li shanwen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>
#include <stdlib.h>
#include "lm_string.h"
#include "lm_error.h"
#include "lm_mem.h"
#include "lm_macro.h"
#include "lm_parser.h"
#include "lm_log.h"
#include <dirent.h>


#define MAX_PER_LINE_LENGTH          4096
#define MAX_FILE_PATH                1024
#define MAX_MACRO_NAME               1024


static char lm_parser_list_name[][20] = {
    {VAR_C_SOURCE},
    {VAR_C_OBJECT},
    {VAR_C_PATH},
    {VAR_C_DEFINE},
    {VAR_ASM_SOURCE},
    {VAR_LDS_SOURCE},
    {VAR_AS_FLAG},
    {VAR_C_FLAG},
    {VAR_CPP_FLAG},
    {VAR_LD_FLAG},
    {VAR_LIB_PATH},
};


static struct lm_parser_list {
    lm_array_t src_list;
    lm_array_t obj_list;
    lm_array_t path_list;
    lm_array_t define_list;
    lm_array_t asm_list;
    lm_array_t lds_list;
    lm_array_t asflag_list;
    lm_array_t cflag_list;
    lm_array_t cppflag_list;
    lm_array_t ldflag_list;
    lm_array_t libpath_list;

}lm_parser_list;


static lm_macro_head_t config_head;
static lm_macro_head_t macro_head;
static const char *config_file = NULL;


static char *lm_parser_alloc(void)
{
    char *mem = (char*)lm_malloc(MAX_PER_LINE_LENGTH);
    if(mem == NULL) {
        LM_LOG_ERROR("out of memory");
        lm_mem_destroy();
        exit(1);
    }

    memset(mem, 0, MAX_PER_LINE_LENGTH);
    return mem;
}


static void lm_parser_skip_space(char **p)
{
    while(**p) {
        if(**p != ' ') {
            return;
        }

        (*p) ++;
    }
}


void lm_parser_init(void)
{
    int len = sizeof(lm_parser_list) / sizeof(lm_array_t);
    lm_array_t *list = (lm_array_t*)&lm_parser_list;

    for(int i = 0; i < len; i++) {
        lm_list_init(&list->head);
        list ++;
    }

    lm_list_init(&config_head.node);
    lm_macro_list_cache_init(&config_head);

    lm_list_init(&macro_head.node);
    lm_macro_list_cache_init(&macro_head);
}


static char* lm_parser_get_macro_value(lm_macro_head_t *head, lm_macro_t *macro)
{
    lm_macro_t *search = lm_macro_search_by_name(head, macro->name);
    if (search != NULL) { // found macro name
        if (search->value == NULL) {
            return NULL;
        }
        else {
            return search->value;
        }
    }
    else {
        return NULL;
    }
}


static bool lm_parser_is_skip_line(char *read_line)
{
    if (lm_str_find_char(read_line, '#') == 0) { // checked annotate
        return true;
    }
    else {
        if (lm_str_is_all_space(read_line)) {
            return true;
        }
    }
    return false;
}


static char *lm_parser_config_get_macro_name(char *str)
{
    char *ret = str;
    char *p = str;
    while (*p) {
        if (*p == '=') {
            *p = '\0';
            return ret;
        }
        p++;
    }

    return NULL;
}


static char *lm_parser_config_get_macro_value(char *str)
{
    while (*str) {
        if (*str == '=') {
            return str + 1;
        }
        str++;
    }

    return NULL;
}


static int lm_parser_config_file_key_string(char *readline)
{
    char *tmp = lm_parser_alloc();

    strcpy(tmp, readline);

    int len = sizeof(lm_parser_list_name) / sizeof(lm_parser_list_name[0]);
    char *name = NULL;

    for(int i = 0; i < len; i++) {
        name = lm_parser_list_name[0];
        strcpy(tmp, readline);

        int key_len = strlen(name);
        tmp[key_len] = '\0';
        if(strcmp(tmp, name) == 0) {
            lm_free(tmp);
            return LM_OK;
        }
    }

    lm_free(tmp);
    return LM_ERR;
}


int lm_parser_config_file(const char *path)
{
    if(path == NULL) {
        return 0; //no defconfig file
    }

    config_file = path;

    lm_macro_t *macro = NULL;
    char *read_line = lm_parser_alloc();

    char *read_scan = NULL;
    FILE *file_p = NULL;
    int line_count = 0;

    file_p = fopen(path, "r"); // read only
    if (file_p == NULL) {
        LM_LOG_ERROR("file: %s, lines: %d, No such file", path, line_count);
        return LM_ERR;
    }

    while (fgets(read_line, MAX_PER_LINE_LENGTH, file_p)) {

        line_count++;

        if (read_line[strlen(read_line) - 1] == '\n' 
           || read_line[strlen(read_line) - 1] == '\r') {
            read_line[strlen(read_line) - 1] = '\0';
        }

        if (lm_parser_is_skip_line(read_line)) {
            continue;
        }

        if(lm_parser_config_file_key_string(read_line) == LM_OK){
            continue;
        }

        char *macro_name = lm_str_delete_space(read_line);
        if (macro_name == NULL) {
            LM_LOG_ERROR("file: %s, lines: %d, syntax error", path, line_count);
            return LM_ERR;
        }

        macro_name = lm_parser_config_get_macro_name(macro_name);
        if (macro_name == NULL) {
            LM_LOG_ERROR("file: %s, lines: %d, syntax error", path, line_count);
            return LM_ERR;
        }

        macro = lm_macro_new_and_add(&config_head, macro_name);
        if (macro == NULL) {
            LM_LOG_ERROR("macro new error\n");
            return LM_ERR;
        }

        lm_free(macro_name);

        read_scan = lm_parser_config_get_macro_value(read_line);
        if (read_scan == NULL) {
            LM_LOG_ERROR("file: %s, lines: %d, syntax error", path, line_count);
            return LM_ERR;
        }

        if(lm_str_is_all_space(read_scan)) {
            lm_macro_value_set(macro, " ");
        }
        else {
            char *macro_value = lm_str_delete_head_tail_space(read_scan);
            lm_macro_value_set(macro, macro_value);
            lm_free(macro_value);
        }
    }

    fclose(file_p); // close file
    return LM_OK;
}

//TODO: enhance this function 
static int lm_parser_macro_express_value(const char *expr) 
{
    char operand[100] = {0};
    int operand_top = 0;
    char operator[100] = {0};
    int operator_top = 0;

    for (int i = 0; i < strlen(expr); i++) {
        if(expr[i] == ' ') {
            continue;
        }
        else if(expr[i] == '(' || expr[i] == '!' || expr[i] == '&' || expr[i] == '|') {
            operator[operator_top++] = expr[i];
            continue;
        }
        else if(expr[i] == '0' || expr[i] == '1') {
            if(operator_top == 0) {
                operand[operand_top++] = expr[i] - '0';
                continue;
            }

            if(operator[operator_top - 1] == '&' || operator[operator_top - 1] == '|') {
                int num = 0;
                int a = operand[--operand_top];
                int b = expr[i] - '0';
                num = (operator[operator_top - 1] == '&' ? a & b : a | b);
                operand[operand_top++] = num;

                operator_top --;
            }
            else if(operator[operator_top - 1] == '!') {
                operand[operand_top++] = !(expr[i] - '0');
                operator_top --;
            }
            // else {
            //     operand[operand_top++] = expr[i];
            // }
        }
        else if(expr[i] == ')') {
            
        }
        else {
            return -1;
        }
    }

    if(operator_top == 1) {
        int a = operand[--operand_top];
        int b = operand[--operand_top];
        operand[operand_top++] = (operator[--operator_top] == '&' ? a & b : a | b);
    }

    if(operand_top != 1 || operator_top != 0) {
        return -1;
    }

    return operand[0];
}


static char *lm_parser_macro_depend_preprocess(char *depend)
{
    if(depend == NULL) {
        return NULL;
    }
    
    int dep_len = strlen(depend) + 1;

    char *output = lm_parser_alloc();
    char *p = output;
    char macro_name[MAX_MACRO_NAME];
    char *name_p = macro_name;

    for (int i = 0; i < dep_len; i++) {
        if (*depend == ' ') {
            depend++;
            continue;
        }
        else if (*depend != '&' && *depend != '|' && *depend != '!' 
                 && *depend != '(' && *depend != ')' && *depend != '\0') {
            *name_p++ = *depend++;
        }
        else {
            *name_p++ = '\0';

            if(macro_name[0] != 0) {

                lm_macro_t *macro = lm_macro_search_by_name(&macro_head, macro_name);
                if (macro != NULL) { // found macro name
                    if (macro->value == NULL || strcmp(macro->value, "n") == 0 || strcmp(macro->value, " ") == 0) {
                        *p++ = '0';
                    }
                    else {
                        *p++ = '1';
                    }
                }
                else {
                    *p++ = '0';
                }
            }

            if(*depend)
                *p++ = *depend++;

            name_p = macro_name;
        }
    }
    *p = '\0';

    return output;
}


static int lm_parser_get_macro_depend_value(lm_macro_t *macro)
{
    if (macro == NULL) {
        return -1;
    }

    char *dep = lm_parser_macro_depend_preprocess(macro->depend);
    if(dep == NULL) { //no dependence
        return 1;
    }
    else if(dep == "error") {
        return 2; //undefine macro error
    }

    return lm_parser_macro_express_value(dep);
}


static lm_parser_err_e lm_parser_prompt_is_src(char *read_line)
{
    char *p = read_line;
    bool flag = false;
    if(p[0] == 's' && p[1] == 'r' && p[2] == 'c' && p[3] == '-' && p[4] == 'y') {
        p += 5;
        while(*p) {
            if(*p == '+' && *(p+1) == '=') {
                return LM_PARSER_OK;
            }
            else if(*p == ' ') {
                p ++;
            }
            else {
                return LM_PARSER_SYNTAX; 
            }
        }

        return LM_PARSER_SYNTAX; 
    }
    else if(p[0] == 's' && p[1] == 'r' && p[2] == 'c' && p[3] == '-' && p[4] == '$'
           && p[5] == '(' ) {
        p += 6;
        while(*p) {
            if(*(++p) == ')') {
                flag = true;
                break;
            }
        }
        p++;

        if(!flag) {
            return LM_PARSER_SYNTAX;
        }

        while(*p) {
            if(*p == '+' && *(p+1) == '=') {
                return LM_PARSER_OK;
            }
            else if(*p == ' ') {
                p ++;
            }
            else {
                return LM_PARSER_SYNTAX; 
            }
        }

        return LM_PARSER_SYNTAX; 
    }

    return LM_PARSER_NOT_MATCH;
}


static lm_parser_err_e lm_parser_prompt_is_obj(char *read_line)
{
    char *p = read_line;
    bool flag = false;
    if(p[0] == 'o' && p[1] == 'b' && p[2] == 'j' && p[3] == '-' && p[4] == 'y') {
        p += 5;
        while(*p) {
            if(*p == '+' && *(p+1) == '=') {
                return LM_PARSER_OK;
            }
            else if(*p == ' ') {
                p ++;
            }
            else {
                return LM_PARSER_SYNTAX; 
            }
        }

        return LM_PARSER_SYNTAX; 
    }
    else if(p[0] == 'o' && p[1] == 'b' && p[2] == 'j' && p[3] == '-' && p[4] == '$'
           && p[5] == '(' ) {
        p += 6;
        while(*p) {
            if(*(++p) == ')') {
                flag = true;
                break;
            }
        }
        p++;

        if(!flag) {
            return LM_PARSER_SYNTAX;
        }

        while(*p) {
            if(*p == '+' && *(p+1) == '=') {
                return LM_PARSER_OK;
            }
            else if(*p == ' ') {
                p ++;
            }
            else {
                return LM_PARSER_SYNTAX; 
            }
        }

        return LM_PARSER_SYNTAX; 
    }

    return LM_PARSER_NOT_MATCH;
}


static lm_parser_err_e lm_parser_prompt_is_src_or_obj(char *read_line)
{
    if(lm_parser_prompt_is_src(read_line) != LM_PARSER_OK) {
        return lm_parser_prompt_is_obj(read_line);
    }
    else {
        return LM_PARSER_OK;
    }
}


static lm_parser_err_e lm_parser_prompt_is_key_string(char *key_str, char *read_line)
{
    char *p = read_line;
    int key_len = strlen(key_str);

    for(int i = 0; i < key_len; i ++) {
        if(p[i] == key_str[i]) {
            continue;
        }
        else {
            return LM_PARSER_NOT_MATCH;
        }
    }

    p += (key_len);
    
    while(*p) {
        if(*p == '+' && *(p+1) == '=') {
            return LM_PARSER_OK;
        }
        else if(*p == ' ') {
            p ++;
        }
        else {
            return LM_PARSER_SYNTAX; 
        }
    }

    return LM_PARSER_SYNTAX; 
}


static void lm_parser_key_list_add_with_path(lm_array_t *list, const char *path, char *readline)
{
    char file_name[MAX_FILE_PATH];
    char *p = readline;

    while(*p++) {
        if(*p == '+' && *(p+1) == '=') {
            p += 2;
            break;
        }
    }

    int num = lm_str_num_str_space(p);
    char *pick = NULL;

    for(int i = 0; i < num; i++) {
        pick = lm_str_pick_str(p, i);
        if(pick && list) {
            if(strcmp(pick, ".") == 0 || strcmp(pick, "./") == 0) {
                if(strcmp(path, ".") != 0 && strcmp(path, "./") == 0) {
                    sprintf(file_name, "%s/%s", path, pick);
                    lm_array_add(list, file_name);
                    lm_free(pick);
                }
            }
            else {
                if(strcmp(path, ".") == 0)
                    sprintf(file_name, "%s", pick);
                else
                    sprintf(file_name, "%s/%s", path, pick);

                lm_array_add(list, file_name);
                lm_free(pick);
            }
        }
    }
}


void lm_parser_key_list_add_no_path(lm_array_t *list, char *readline)
{
    char *str = lm_parser_alloc();
    char *p = readline;

    while(*p++) {
        if(*p == '+' && *(p+1) == '=') {
            p += 2;
            break;
        }
    }

    int num = lm_str_num_str_space(p);
    char *pick = NULL;

    for(int i = 0; i < num; i++) {
        pick = lm_str_pick_str(p, i);
        if(pick && list) {
            sprintf(str, "%s", pick);
            lm_array_add(list, str);
            lm_free(pick);
        }
    }

    lm_free(str);
}


static void lm_parser_key_list_add_raw_value(lm_array_t *list, char *readline)
{
    char *str = lm_parser_alloc();
    char *p = readline;

    while(*p++) {
        if(*p == '+' && *(p+1) == '=') {
            p += 2;
            break;
        }
    }

    lm_parser_skip_space(&p);

    if(list) {
        sprintf(str, "%s", p);
        lm_array_add(list, str);
    }

    lm_free(str);
}


static void lm_parser_key_list_add_with_path_and_prefix(lm_array_t *list, const char *path, char *prefix, char *readline)
{
    char *str = lm_parser_alloc();
    char *p = readline;

    while(*p++) {
        if(*p == '+' && *(p+1) == '=') {
            p += 2;
            break;
        }
    }

    int num = lm_str_num_str_space(p);
    char *pick = NULL;

    for(int i = 0; i < num; i++) {
        pick = lm_str_pick_str(p, i);
        if(pick && list) {
            if(strcmp(pick, ".") == 0 || strcmp(pick, "./") == 0) {
                if(strcmp(path, ".") != 0 && strcmp(path, "./") != 0)
                    sprintf(str, "%s%s", prefix, path);
                else
                    sprintf(str, "%s.", prefix);
            }
            else {
                if(strcmp(path, ".") == 0)
                    sprintf(str, "%s%s", prefix, pick);
                else
                    sprintf(str, "%s%s/%s", prefix, path, pick);
            }
            
            lm_array_add(list, str);
            lm_free(pick);
        }
    }

    lm_free(str);
}


static void lm_parser_key_list_add_no_path_with_prefix(lm_array_t *list, char *prefix, char *readline)
{
    char *str = lm_parser_alloc();
    char *p = readline;

    while(*p++) {
        if(*p == '+' && *(p+1) == '=') {
            p += 2;
            break;
        }
    }

    int num = lm_str_num_str_space(p);
    char *pick = NULL;

    for(int i = 0; i < num; i++) {
        pick = lm_str_pick_str(p, i);
        if(pick && list) {
            sprintf(str, "%s%s", prefix, pick);
            lm_array_add(list, str);
            lm_free(pick);
        }
    }

    lm_free(str);
}


static void lm_parser_src_obj_add_wildcard(const char *wildcard, const char *path, lm_array_t *array)
{
    DIR *dir;
    struct dirent *entry;
    char file_name[MAX_FILE_PATH];
    char file_extension[3];

    dir = opendir(path);
    if (dir == NULL) {
        LM_LOG_ERROR("opening %s directory", path);
    }

    strncpy(file_extension, wildcard + 1, 2);

    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);

        if (len >= 2 && strcmp(entry->d_name + len - 2, ".c") == 0) {
            if(strcmp(path, ".") == 0) {
                sprintf(file_name, "%.*s%s", (int)(len - 2), entry->d_name, file_extension);
            }
            else {
                sprintf(file_name, "%s/%.*s%s", path, (int)(len - 2), entry->d_name, file_extension);
            }
            
            lm_array_add(array, file_name);
        }
    }

    closedir(dir);
}


static void lm_parser_src_obj_add(lm_macro_head_t *head, const char *path, char *readline)
{
    lm_array_t *array = NULL;
    char macro_name[MAX_MACRO_NAME];
    char file_name[MAX_FILE_PATH];
    char *p = readline;
    int i = 0;
    bool flag = false;

    if(lm_parser_prompt_is_src(readline) == LM_PARSER_OK) {
        array = &lm_parser_list.src_list;
    }
    else if(lm_parser_prompt_is_obj(readline) == LM_PARSER_OK) {
        array = &lm_parser_list.obj_list;
    }

    while(*p++) {
        if(*p == 'y') {
            flag = true;
        }
        else if(*p == '$' && *(p+1) == '(') {
            p += 1;
            while(*p++) {
                if(*p == ')') {
                    macro_name[i++] = '\0';
                    break;
                }
                macro_name[i++] = *p;
            }
        }

        if(*p == '+' && *(p+1) == '=') {
            p += 2;
            break;
        }
    }

    if(!flag) {
        lm_macro_t * macro = lm_macro_search_by_name(head, macro_name);
        if(macro != NULL) {
            if(strcmp(macro->value, "n") == 0) {
                return;
            }
        }
        else {
            return;
        }
    }

    int num = lm_str_num_str_space(p);
    char *pick = NULL;

    for(int i = 0; i < num; i++) {
        pick = lm_str_pick_str(p, i);
        if(pick && array) {
            if((strcmp(pick, "*.c") == 0) || (strcmp(pick, "*.o") == 0)) {
                lm_parser_src_obj_add_wildcard(pick, path, array);
                lm_free(pick);
            }
            else {
                if(strcmp(path, ".") == 0)
                    sprintf(file_name, "%s", pick);
                else
                    sprintf(file_name, "%s/%s", path, pick);

                lm_array_add(array, file_name);
                lm_free(pick);
            }
        }
    }
}


static lm_macro_t *lm_parser_prompt_is_macro(lm_macro_head_t *head, char *read_line)
{
    lm_macro_t *ret;
    if (read_line[0] != ' ') {
        if (lm_str_num_str_space(read_line) == 1) {
            char *name = lm_str_delete_space(read_line);
            ret = lm_macro_new_and_add(head, name);

            lm_free(name);
            return ret;
        }
    }
    return NULL;
}


static lm_parser_err_e lm_parser_prompt_is_depend(lm_macro_t *macro, char *read_line)
{
    char *p = read_line;

    if (!lm_str_head_is_four_space(read_line))
        return LM_PARSER_SYNTAX;

    if (lm_str_find_str_space(read_line, "depend") == 0 
        && lm_str_find_str_space(read_line, "=") == 1) {

        while (*p) {
            if (*p != '=')
                p++;
            else
                break;
        }

        lm_macro_depend_set(macro, p+1);

        return LM_PARSER_OK;
    }

    return LM_PARSER_NOT_MATCH;
}


static char *lm_parser_prompt_process_var(char *read_line)
{
    char *rel_val = lm_parser_alloc();
    char *rel_p = rel_val;

    char *p = read_line;
    char var_name[MAX_MACRO_NAME];
    int i = 0;

    while(*p) {
        if(*p == '$' && *(p+1) == '(') {
            p+=2;
            while(*p) {
                if(*p != ')' && *p != ' ') {
                   var_name[i++] = *p;
                }
                else if(*p != ' ') {
                    var_name[i] = '\0';

                    lm_macro_t * macro = lm_macro_search_by_name(&macro_head, var_name);
                    if(macro != NULL) {
                        if(macro->value != NULL) {
                            strcat(rel_val, macro->value);
                            rel_p += strlen(macro->value);
                        }
                    }

                    i = 0;
                    break;
                }

                p++;
            }
        }
        else {
            *rel_p++ = *p;
        }

        p++;
    }

    if(i != 0) {
        return NULL;
    }
    
    *rel_p = '\0';
    return rel_val;
}


static char *lm_parser_prompt_is_include(char *read_line)
{
    char *tmp = NULL;

    if (read_line[0] != ' ') {
        if (lm_str_num_str_space(read_line) == 2) {
            if (lm_str_find_str_space(read_line, "include") == 0) {
                tmp = lm_str_get_quote(read_line);
                return lm_parser_prompt_process_var(tmp);
            }
        }
    }
    return NULL;
}


static lm_parser_err_e lm_parser_prompt_is_choice_number(lm_macro_t *macro, char *str)
{
    char *p_sc = str;
    int str_len = strlen(str);
    int colon = 0;
    double range_min, range_max;
    char num_str[100];

    lm_parser_skip_space(&p_sc);
    
    if(p_sc[0] != '[') {
        return LM_PARSER_NOT_MATCH;
    }

    if (strlen(p_sc) < 3 || p_sc[strlen(p_sc) - 1] != ']') {
        return LM_PARSER_SYNTAX;
    }

    char *token = strtok((char *)p_sc + 1, ",]");
    if (token == NULL) {
        return LM_PARSER_SYNTAX;
    }

    lm_macro_choice_append(macro, token);

    char *endptr;
    float num = strtof(token, &endptr);

    if (*endptr != 0) {
        return LM_PARSER_SYNTAX; // 第一个数字无效
    }
    range_min = num; // 第一个数字

    token = strtok(NULL, "]");
    if (token == NULL) {
        return LM_PARSER_SYNTAX; // 没有找到第二个数字
    }

    num = strtof(token, &endptr);
    if (*endptr != 0) {
        return LM_PARSER_SYNTAX; // 第二个数字无效
    }
    range_max = num; // 第二个数字

    lm_macro_type_set(macro, LM_MACRO_NUMBER);
    lm_macro_range_set(macro, range_min, range_max);

    return LM_PARSER_OK;
}


static int lm_parser_prompt_choice_add_value(lm_macro_t *macro, char *value_str)
{
    char *p = value_str;
    int len = strlen(p) + 1;

    char stack[MAX_MACRO_NAME];
    char *stack_p = stack;
    memset(stack, 0, MAX_MACRO_NAME);
    bool flag = false;

    for (int i = 0; i < len; i++) {

        if(*p == '{' && *p != '\0') {
            flag = true;
            while(*p) {
                *stack_p++ = *p++;
                i ++;

                if(*p == '}' && *p != '\0') {
                    flag = false;
                    break;
                }
            }
        }

        if(*p == '\"' && *p != '\0') {
            flag = true;
            while(*p) {
                *stack_p++ = *p++;
                i ++;

                if(*p == '\"' && *p != '\0') {
                    flag = false;
                    break;
                }
            }
        }

        if(*p == '\'' && *p != '\0') {
            p ++;
            flag = true;
            while(*p) {
                *stack_p++ = *p++;
                i ++;

                if(*p == '\'' && *p != '\0') {
                    flag = false;
                    break;
                }
            }
        }

        if(*p != ' ' && *p != '\'') {
            if (*p != ',' && *p != '\0') {
                *stack_p++ = *p;
            }
            else {
                *stack_p = '\0';

                lm_macro_type_set(macro, LM_MACRO_STRING);
                lm_macro_choice_append(macro, stack);
                stack_p = stack;
            }
        }

        p++;
    }

    if(flag) {
        return -1;
    }
    return 0;
}


static lm_parser_err_e lm_parser_prompt_is_choice(lm_macro_t *macro, char *read_line)
{
    char *p = read_line;

    if (!lm_str_head_is_four_space(read_line))
        return LM_PARSER_SYNTAX;

    if (lm_str_find_str_space(read_line, "choice") == 0 
         && lm_str_find_str_space(read_line, "=") == 1) {

        while (*p) {
            if (*p != '=')
                p++;
            else
                break;
        }

        char *value_str = NULL;
        lm_str_dupli_string(&value_str, ++p);

        lm_parser_err_e err_ret = lm_parser_prompt_is_choice_number(macro, value_str);
        if(err_ret == LM_PARSER_OK) {
            return LM_PARSER_OK;
        }
        else if(err_ret == LM_PARSER_SYNTAX) {
            return LM_PARSER_SYNTAX;
        }

        if (value_str[0] == ',' || value_str[strlen(value_str) - 1] == ',') {
            return LM_PARSER_SYNTAX;
        }

        if(lm_parser_prompt_choice_add_value(macro, value_str)) {
            return LM_PARSER_SYNTAX;
        }

        lm_free(value_str);

        return LM_PARSER_OK;
    }

    return LM_PARSER_NOT_MATCH;
}


static lm_parser_err_e lm_parser_macro_set_value(lm_macro_t *macro)
{
    if(macro == NULL) {
        return LM_PARSER_OK;
    }

    int value = lm_parser_get_macro_depend_value(macro);
    if(value == 1) {
        lm_macro_t *search = lm_macro_search_by_name(&config_head, macro->name);
        if(search == NULL) { //not found in defconfig list
            char *first_value = lm_macro_choice_get_first(macro);
            if(first_value)
                lm_macro_value_set(macro, first_value);
            else
                lm_macro_value_set(macro, "Unknow ERROR");
        }
        else {
            if(strcmp(search->value, "n") == 0) {
                lm_macro_value_set(macro, "n");
            }
            else if(strcmp(search->value, "'n'") == 0) {
                if(lm_macro_value_is_valid(macro, "n")) {
                    lm_macro_value_set(macro, "'n'");
                }
                else {
                    return LM_PARSER_INVALID_VALUE;
                }
            }
            else if(lm_macro_value_is_valid(macro, search->value)) {
                lm_macro_value_set(macro, search->value);
            }
            else {
                return LM_PARSER_INVALID_VALUE;
            }
        }
    }
    else if(value < 0) {
        return LM_PARSER_INVALID_DEPEND;
    }
    else {
        lm_macro_value_set(macro, "n");
    }

    return LM_PARSER_OK;
}


static lm_parser_err_e lm_parser_lm_file_key_string(lm_macro_head_t *head, const char *base_path, char *read_line)
{
    lm_parser_err_e prompt_err = lm_parser_prompt_is_src_or_obj(read_line);
    if(prompt_err == LM_PARSER_OK) {
        lm_parser_src_obj_add(head, base_path, read_line);
        return LM_PARSER_OK;
    }
    else if(prompt_err == LM_PARSER_SYNTAX) {
        return LM_PARSER_SYNTAX;
    }

    prompt_err = lm_parser_prompt_is_key_string("path", read_line);
    if(prompt_err == LM_PARSER_OK) {
        lm_parser_key_list_add_with_path_and_prefix(&lm_parser_list.path_list, base_path, "-I", read_line);
        return LM_PARSER_OK;
    }
    else if(prompt_err == LM_PARSER_SYNTAX) {
        return LM_PARSER_SYNTAX;
    }

    prompt_err = lm_parser_prompt_is_key_string("define", read_line);
    if(prompt_err == LM_PARSER_OK) {
        lm_parser_key_list_add_no_path_with_prefix(&lm_parser_list.define_list, "-D", read_line);
        return LM_PARSER_OK;
    }
    else if(prompt_err == LM_PARSER_SYNTAX) {
        return LM_PARSER_SYNTAX;
    }

    prompt_err = lm_parser_prompt_is_key_string("asm", read_line);
    if(prompt_err == LM_PARSER_OK) {
        lm_parser_key_list_add_with_path(&lm_parser_list.asm_list, base_path, read_line);
        return LM_PARSER_OK;
    }
    else if(prompt_err == LM_PARSER_SYNTAX) {
        return LM_PARSER_SYNTAX;
    }

    prompt_err = lm_parser_prompt_is_key_string("lds", read_line);
    if(prompt_err == LM_PARSER_OK) {
        lm_parser_key_list_add_with_path(&lm_parser_list.lds_list, base_path, read_line);
        return LM_PARSER_OK;
    }
    else if(prompt_err == LM_PARSER_SYNTAX) {
        return LM_PARSER_SYNTAX;
    }

    prompt_err = lm_parser_prompt_is_key_string("asflag", read_line);
    if(prompt_err == LM_PARSER_OK) {
        lm_parser_key_list_add_raw_value(&lm_parser_list.asflag_list, read_line);
        return LM_PARSER_OK;
    }
    else if(prompt_err == LM_PARSER_SYNTAX) {
        return LM_PARSER_SYNTAX;
    }

    prompt_err = lm_parser_prompt_is_key_string("cflag", read_line);
    if(prompt_err == LM_PARSER_OK) {
        lm_parser_key_list_add_raw_value(&lm_parser_list.cflag_list, read_line);
        return LM_PARSER_OK;
    }
    else if(prompt_err == LM_PARSER_SYNTAX) {
        return LM_PARSER_SYNTAX;
    }

    prompt_err = lm_parser_prompt_is_key_string("cppflag", read_line);
    if(prompt_err == LM_PARSER_OK) {
        lm_parser_key_list_add_raw_value(&lm_parser_list.cppflag_list, read_line);
        return LM_PARSER_OK;
    }
    else if(prompt_err == LM_PARSER_SYNTAX) {
        return LM_PARSER_SYNTAX;
    }

    prompt_err = lm_parser_prompt_is_key_string("ldflag", read_line);
    if(prompt_err == LM_PARSER_OK) {
        lm_parser_key_list_add_raw_value(&lm_parser_list.ldflag_list, read_line);
        return LM_PARSER_OK;
    }
    else if(prompt_err == LM_PARSER_SYNTAX) {
        return LM_PARSER_SYNTAX;
    }

    prompt_err = lm_parser_prompt_is_key_string("libpath", read_line);
    if(prompt_err == LM_PARSER_OK) {
        lm_parser_key_list_add_with_path_and_prefix(&lm_parser_list.libpath_list, base_path, "-L", read_line);
        return LM_PARSER_OK;
    }
    else if(prompt_err == LM_PARSER_SYNTAX) {
        return LM_PARSER_SYNTAX;
    }

    return LM_PARSER_NOT_MATCH;
}


static int lm_parser_macro_set_prompt(lm_macro_t *macro, char *read_line)
{
    lm_parser_err_e prompt_ret = LM_PARSER_NOT_MATCH;

    prompt_ret = lm_parser_prompt_is_choice(macro, read_line);
    if(prompt_ret == LM_PARSER_SYNTAX) {
        return LM_ERR;
    }
    else if(prompt_ret == LM_PARSER_OK) {
        return LM_OK;
    }

    prompt_ret = lm_parser_prompt_is_depend(macro, read_line);
    if(prompt_ret == LM_PARSER_SYNTAX) {
        return LM_ERR;
    }
    else if(prompt_ret == LM_PARSER_OK) {
        return LM_OK;
    }

    return LM_ERR;
}


static void lm_parser_macro_help(lm_macro_t *macro, lm_parser_err_e flag)
{
    switch(flag) {
        case LM_PARSER_INVALID_VALUE:
            char *value_p = lm_parser_get_macro_value(&config_head, macro);
            LM_LOG_ERROR("%s: %s = %s value is invalid", config_file, macro->name, value_p);
            printf("\x1b[32m[INFO]💡You can choose: ");

            if(lm_macro_type_get(macro) == LM_MACRO_NUMBER) {
                printf("[%f ~ %f]", macro->range.min, macro->range.max);
            }
            else {
                lm_array_print(stdout, &macro->choice);
            }
            printf("\x1b[0m\n");
            break;

        case LM_PARSER_INVALID_DEPEND:
            LM_LOG_ERROR("%s: depend of %s syntax is invalid", config_file, macro->name);
            break;
        
        default:
            LM_LOG_ERROR("%s: %s Unknow ERROR", config_file, macro->name);
            break;
    }
}


int lm_parser_lm_file(const char *base_path, const char *path)
{
    char *read_line = lm_parser_alloc();
    int line_count = 0;
    lm_macro_t *macro = NULL;
    FILE *file_p = NULL;

    char *full_path = lm_parser_alloc();
    if(base_path == NULL || (strcmp(base_path, ".") == 0)) {
        strcpy(full_path, path);
    }
    else {
        sprintf(full_path, "%s/%s", base_path, path);
    }

    file_p = fopen(full_path, "r"); // read only
    if (file_p == NULL) {
        LM_LOG_ERROR("file: %s, No such file", full_path);
        return LM_ERR;
    }

    char *new_base_path = lm_parser_alloc();
    strcpy(new_base_path, full_path);

    char *last_dir = strrchr(new_base_path, '/');
    if(last_dir) {
        *last_dir = '\0';
    }
    else {
        strcpy(new_base_path, ".");
    }

    while (fgets(read_line, MAX_PER_LINE_LENGTH, file_p)) {

        line_count++;

        if (read_line[strlen(read_line) - 1] == '\n' || read_line[strlen(read_line) - 1] == '\r') {
            read_line[strlen(read_line) - 1] = '\0';
        }

        if(read_line[strlen(read_line) - 1] == '\\') {
            read_line[strlen(read_line) - 1] = '\0';

            char *read_line_tmp = lm_parser_alloc();

            while(fgets(read_line_tmp, MAX_PER_LINE_LENGTH, file_p)) {
                if (read_line_tmp[strlen(read_line_tmp) - 1] == '\n' || read_line_tmp[strlen(read_line_tmp) - 1] == '\r') {
                    read_line_tmp[strlen(read_line_tmp) - 1] = '\0';
                }

                line_count++;
                char *pure = lm_str_delete_head_tail_space(read_line_tmp);
                
                read_line[strlen(read_line) - 1] = '\0';
                strcat(read_line, pure);
                
                lm_free(pure);
                if(read_line_tmp[strlen(read_line_tmp) - 1] != '\\') {
                    break;
                }
            }

            lm_free(read_line_tmp);
        }

        if (lm_parser_is_skip_line(read_line)) {
            if(macro && macro->choice.count == 0) {
                goto macro_err;
            }
            macro = NULL;
            continue;
        }

        char *sub_file = lm_parser_prompt_is_include(read_line);
        if(sub_file != NULL) {

            uint64_t file_pos = ftell(file_p);
            fclose(file_p);

            int sub_ret = lm_parser_lm_file(new_base_path, sub_file);
            if(sub_ret != LM_OK) {
                goto exit;
            }
            macro = NULL;

            file_p = fopen(full_path, "r");
            fseek(file_p, file_pos, SEEK_SET);

            continue;
        }

        if(macro != NULL) {
            if(lm_parser_macro_set_prompt(macro, read_line) == LM_ERR) {
                LM_LOG_ERROR("file: %s, lines: %d, invalid syntax", path, line_count);
                goto exit;
            }

            if(macro->choice.count == 0) {
                goto macro_err;
            }

            lm_parser_err_e val_ret = lm_parser_macro_set_value(macro);
            if(val_ret == LM_PARSER_INVALID_VALUE) {
                lm_parser_macro_help(macro, LM_PARSER_INVALID_VALUE);
                goto exit;
            }
            else if(val_ret == LM_PARSER_INVALID_DEPEND) {
                lm_parser_macro_help(macro, LM_PARSER_INVALID_DEPEND);
                goto exit;
            }
            continue;
        }

        lm_parser_err_e key_ret = lm_parser_lm_file_key_string(&macro_head, new_base_path, read_line);
        if(key_ret == LM_PARSER_OK) {
            macro = NULL;
            continue;
        }
        else if(key_ret == LM_PARSER_SYNTAX) {
            goto syntax_err;
        }

        lm_macro_t *tmp_macro = lm_parser_prompt_is_macro(&macro_head, read_line);
        if(tmp_macro != NULL) {
            macro = tmp_macro;
        }
        else {
            goto syntax_err;
        }
    }

    if(lm_parser_macro_set_value(macro) == LM_PARSER_INVALID_VALUE) {
        char *value_p = lm_parser_get_macro_value(&config_head, macro);
        LM_LOG_ERROR("%s: %s value is invalid", macro->name, value_p);
        goto exit;
    }

    fclose(file_p); // close file
    lm_free(read_line);
    lm_free(full_path);
    lm_free(new_base_path);
    return LM_OK;

syntax_err:
    LM_LOG_ERROR("file: %s, lines: %d, invalid syntax", path, line_count);
    fclose(file_p); // close file
    lm_free(read_line);
    return LM_ERR;

macro_err:
    LM_LOG_ERROR("file: %s, lines: %d, missing 'choice' attribute", path, line_count);
    fclose(file_p); // close file
    lm_free(read_line);
    return LM_ERR;

exit:
    fclose(file_p); // close file
    lm_free(read_line);
    return LM_ERR;
}


int lm_parser_gen_header_file(const char* file_path)
{
    lm_list_node_t *node = lm_list_next_node(&macro_head.node);
    lm_macro_t *macro = NULL;

    FILE* file = fopen(file_path, "w");
    if (file == NULL) {
        printf("Failed to open or create the %s\n", file_path);
        return LM_ERR;
    }

    fprintf(file, "//****************************************************************\n");
    fprintf(file, "//* lite-manager                                                 *\n");
    fprintf(file, "//* NOTE: do not edit this file as it is automatically generated *\n");
    fprintf(file, "//****************************************************************\n\n");
    fprintf(file, "#ifndef  __CONFIG_H__\n#define  __CONFIG_H__\n\n\n");

    for(int i = 0; i < macro_head.count; i++) {

        macro = container_of(node, lm_macro_t, node);
        if(macro == NULL) {
            return LM_ERR;
        }

        if(macro->value != NULL) {

            if(strcmp(macro->value, " ") == 0){
                fprintf(file, "// %s is not set\n", macro->name);
            }

            if(strcmp(macro->value, "y") == 0) {
                if (fprintf(file, "#define    %-30s     1\n", macro->name) < 0) {
                    printf("Failed to write to the %s\n", file_path);
                    return LM_ERR;
                }
            }
            else if(strcmp(macro->value, "'n'") == 0) {
                if (fprintf(file, "#define    %-30s     n\n", macro->name) < 0) {
                    printf("Failed to write to the %s\n", file_path);
                    return LM_ERR;
                }
            }
            else if(strcmp(macro->value, " ") != 0 && strcmp(macro->value, "n") != 0){
                if (fprintf(file, "#define    %-30s     %-s\n", macro->name, macro->value) < 0) {
                    printf("Failed to write to the %s\n", file_path);
                    return LM_ERR;
                }
            }
        }

        node = lm_list_next_node(node);
    }

    fprintf(file, "\n\n#endif  //!__CONFIG_H__\n");
    
    fclose(file);
    return LM_OK;
}


int lm_parser_gen_mkconf_file(const char* file_path)
{
    lm_list_node_t *node = lm_list_next_node(&macro_head.node);
    lm_macro_t *macro = NULL;

    FILE* file = fopen(file_path, "w");
    if (file == NULL) {
        printf("Failed to open or create the %s\n", file_path);
        return LM_ERR;
    }

    fprintf(file, "#****************************************************************\n");
    fprintf(file, "#* lite-manager                                                 *\n");
    fprintf(file, "#* NOTE: do not edit this file as it is automatically generated *\n");
    fprintf(file, "#****************************************************************\n\n");

    for(int i = 0; i < macro_head.count; i++)  {

        macro = container_of(node, lm_macro_t, node);
        if(macro == NULL) {
            return LM_ERR;
        }

        if(macro->value != NULL) {
            if (fprintf(file, "%s = %s\n", macro->name, macro->value) < 0) {
                printf("Failed to write to the %s\n", file_path);
                return LM_ERR;
            }
        }

        node = lm_list_next_node(node);
    }

    fprintf(file, "\n# Variables provided for Makefile\n");

    int len = sizeof(lm_parser_list) / sizeof(lm_array_t);
    lm_array_t *list = (lm_array_t*)&lm_parser_list;
    char *name = NULL;

    for(int i = 0; i < len; i++) {

        if(list->count != 0) {
            name = lm_parser_list_name[i];
            if(name != NULL) {
                fprintf(file, "%s := ", name);
            }

            lm_array_print(file, list);
        }

        list ++;
    }

    fclose(file);
    return LM_OK;
}


void lm_parser_print_macro_list(void)
{
    lm_macro_print_all(stdout, &macro_head);
}


void lm_parser_print_config_list(void)
{
    lm_macro_print_all(stdout, &config_head);
}


void lm_parser_print_path_list(void)
{
    lm_array_print(stdout, &lm_parser_list.path_list);
}


void lm_parser_print_define_list(void)
{
    lm_array_print(stdout, &lm_parser_list.define_list);
}


void lm_parser_print_option_list(void)
{
    lm_array_print(stdout, &lm_parser_list.ldflag_list);
}


void lm_parser_print_src_list(void)
{
    lm_array_print(stdout, &lm_parser_list.src_list);
}


bool lm_parser_lds_is_empty(void)
{
    return lm_parser_list.lds_list.count == 0;
}


#define ESC "\x1b"
#define CSI "\x1b["

void lm_parser_print_all_macro_value(void)
{
    printf("\x1b[32m"); //green color
#if (_WIN32)
    printf(ESC"(0""┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓"ESC"(B" "\n");
    printf(ESC"(0""┃"ESC"(B");
    printf(        "                                   lite-manager config information      🔨                           ");
    printf(ESC"(0""┃"ESC"(B" "\n");
    printf(ESC"(0""┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫"ESC"(B" "\n");
#elif ( __linux__)
    printf("┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
    printf("┃                                   lite-manager config information      🔨                           ┃\n");
    printf("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
#else
    #error Unknow host OS type!
#endif
    printf("\x1b[0m");  //clear color

    // print all macro value
    lm_list_node_t *node_head = &macro_head.node;
    lm_list_node_t *node;
    lm_macro_t *macro = NULL;

    lm_list_for_each(node, node_head) {

        macro = container_of(node, lm_macro_t, node);
        if(macro == NULL) {
            continue;
        }

        if(strcmp(macro->value, "n") == 0) {
            printf("\x1b[32m┃\x1b[0m");
            printf("⛔ %-43s%-55s", macro->name, " ");
            printf("\x1b[32m┃\x1b[0m\n");
        }
        else if(strcmp(macro->value, "'n'") == 0) {
            printf("\x1b[32m┃\x1b[0m");
            printf("✅ %-43s%-55s", macro->name, "n");
            printf("\x1b[32m┃\x1b[0m\n");
        }
        else {
            printf("\x1b[32m┃\x1b[0m");
            printf("✅ %-43s%-55s", macro->name, macro->value);
            printf("\x1b[32m┃\x1b[0m\n");
        }
    }

    printf("\x1b[32m"); //green color
#if (_WIN32)
    printf(ESC"(0""┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛"ESC"(B" "\n");
#elif ( __linux__)
    printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");
#else
    #error Unknow host OS type!
#endif
    printf("\x1b[0m"); //clear color
}
