/* source/lm_parser.h
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

#ifndef __LM_PARSER_H__
#define __LM_PARSER_H__

#include "lm_list.h"
#include "lm_macro.h"


#define    VAR_C_SOURCE             "C_SOURCE"
#define    VAR_C_OBJECT             "C_OBJECT"
#define    VAR_C_PATH               "C_PATH"
#define    VAR_C_DEFINE             "C_DEFINE"
#define    VAR_ASM_SOURCE           "ASM_SOURCE"
#define    VAR_LDS_SOURCE           "LDS_SOURCE"
#define    VAR_AS_FLAG              "AS_FLAG"
#define    VAR_C_FLAG               "C_FLAG"
#define    VAR_CPP_FLAG             "CPP_FLAG"
#define    VAR_LD_FLAG              "LD_FLAG"
#define    VAR_LIB_PATH             "LIB_PATH"


typedef enum lm_parser_err {
    LM_PARSER_OK = 0,
    LM_PARSER_SYNTAX = 1,
    LM_PARSER_NOT_MATCH = 2,
    LM_PARSER_INVALID_VALUE = 3,
    LM_PARSER_INVALID_DEPEND = 4,

}lm_parser_err_e;


#ifdef __cplusplus
extern "C" {
#endif


void lm_parser_init(void);
int lm_parser_config_file(const char *path);
int lm_parser_lm_file(const char *base_path, const char *path);
void lm_parser_print_macro_list(void);
void lm_parser_print_path_list(void);
void lm_parser_print_define_list(void);
void lm_parser_print_option_list(void);
void lm_parser_print_src_list(void);
int lm_parser_gen_header_file(const char* file_path);
int lm_parser_gen_lmmk_file(const char* file_path);
void lm_parser_print_all_macro_value(void);
void lm_parser_print_config_list(void);
bool lm_parser_lds_is_empty(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif //!__LM_PARSER_H__
