/* source/main.c
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

#include "stdio.h"
#include "lm_log.h"
#include "lm_error.h"
#include "lm_parser.h"
#include "lm_string.h"
#include "lm_mem.h"
#include <getopt.h>
#include <config.h>


#define    VERSION           "0.2024120"


static const char *mkfile = NULL;
static const char *pro_name = "demo";
static const char *build_dir = "build";
static const char *config_file = "proj.cfg";
static const char *lm_file = "lm.cfg";
static const char *header_file = "config.h";
static const char *confmk_file = ".lm.mk";
static const char *info_file = "info.log";
static int mem_size = CONFIG_MEM_POOL_SIZE;
static bool blind = false;


static void show_key_usage(void)
{
    printf("lite-manager key: [option] += [value] [value] ...\n");
    printf("\n[option]:\n");
    printf("    src:     add c or c++ source files\n");
    printf("    obj:     add c or c++ object files\n");
    printf("    path:    add c or c++ include path\n");
    printf("    define:  add c or c++ global macro define\n");
    printf("    asm:     add asm source files\n");
    printf("    lds:     add build link script file\n");
    printf("    asflag:  add asm build flag\n");
    printf("    cflag:   add c build flag\n");
    printf("    cppflag: add c++ build flag\n");
    printf("    ldflag:  add link flag\n");
    printf("    libpath: add library path\n");
}


static void show_help(char *app_name)
{
    printf("Usage: %s [options]\n", app_name);
    printf("[options:]\n");
    printf("  -h, --help                          Display this help message\n");
    printf("  -d, --help detail                   Display this help detail message\n");
    printf("  -v, --version                       Display the version of the lite-manager\n");
    printf("  -c, --input project config          Input using project config file name, default: %s\n", config_file);
    printf("  -f, --input lmfile                  Input lite manager file(toplayer lm.cfg), default: %s\n", lm_file);
    printf("  -o, --output header                 Output header file output path, default: %s\n", header_file);
    printf("  -k, --output config.mk              Output config makefile, default: %s\n", confmk_file);
    printf("  -p, --print information             Print config information\n");
    printf("  -m, --memory size(MB)               Memory size used by lm, default: %dMB\n", mem_size);
    printf("  -b, --blind                         Hide information about configuration macros\n");
    printf("\n");
    printf("  -g, --generate makefile             Generate Makefile: by toplayer lm.cfg, defaule: Makefile\n");
    printf("  -j, --project name                  Generate Makefile: project name, default: demo\n");
    printf("  -u, --build directory               Generate Makefile: build directory, default: build\n");
}


static int lm_gen_projcfg_file(const char* file_path)
{
    if(access(file_path, F_OK) == 0) {
        return LM_OK;
    }

    FILE* file = fopen(file_path, "w");
    if (file == NULL) {
        LM_LOG_ERROR("Failed to open or create the %s\n", file_path);
        return LM_ERR;
    }

    fprintf(file, "#****************************************************************\n");
    fprintf(file, "#* lite-manager                                                 *\n");
    fprintf(file, "#* NOTE: You can edit this file to configure the project        *\n");
    fprintf(file, "#****************************************************************\n\n");

    fclose(file);
    return LM_OK;
}


static int lm_gen_mkfile_file(const char *mkfile, const char *pro_name, const char *build_dir)
{
    FILE* file = fopen(mkfile, "w");
    if (file == NULL) {
        LM_LOG_ERROR("Failed to generate the %s file\n", mkfile);
        return LM_ERR;
    }

    const char *target = NULL;

    if(lm_parser_lds_is_empty()) {
        target = ".exe";
    }
    else {
        target = ".elf";
    }

    fprintf(file, "TARGET    := %s\n", pro_name);
    fprintf(file, "BUILD_DIR := %s\n", build_dir);
    fprintf(file, "\n\n");

    fprintf(file, "# include configuration file for makefile\n");
    fprintf(file, "ifneq ($(wildcard %s),)\n", confmk_file);
    fprintf(file, "-include %s\n", confmk_file);
    fprintf(file, "endif\n");
    fprintf(file, "\n\n");

    fprintf(file, "# toolchain\n");
    fprintf(file, "CC = $(CC_PREFIX)gcc\n");
    fprintf(file, "AS = $(CC_PREFIX)gcc -x assembler-with-cpp\n");
    fprintf(file, "CP = $(CC_PREFIX)objcopy\n");
    fprintf(file, "SZ = $(CC_PREFIX)size\n");
    fprintf(file, "OD = $(CC_PREFIX)objdump\n");
    fprintf(file, "HEX = $(CP) -O ihex\n");
    fprintf(file, "BIN = $(CP) -O binary -S\n");
    fprintf(file, "\n\n");


    fprintf(file, "CFLAGS    := $(%s) $(%s) $(%s) $(%s)\n", VAR_C_PATH, VAR_C_DEFINE, VAR_C_FLAG, VAR_CPP_FLAG);
    
    if(lm_parser_lds_is_empty()) {
        fprintf(file, "LDFLAGS   := $(%s) $(%s) -Wl,-M=$(BUILD_DIR)/$(TARGET).map\n", VAR_LD_FLAG, VAR_LIB_PATH);
    }
    else {
        fprintf(file, "LDFLAGS   := $(%s) $(%s) -T$(%s) -Wl,-M=$(BUILD_DIR)/$(TARGET).map\n", VAR_LD_FLAG, VAR_LIB_PATH, VAR_LDS_SOURCE);
    }
    fprintf(file, "\n\n");

    fprintf(file, ".PHONY: all\n");
    if(lm_parser_lds_is_empty()) {
        fprintf(file, "all: $(BUILD_DIR)/$(TARGET)%s elf_info\n", target);
    }
    else {
        fprintf(file, "all: $(BUILD_DIR)/$(TARGET)%s $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin elf_info\n", target);
    }
    fprintf(file, "\n\n");

    fprintf(file, "# list of c program objects\n");
    fprintf(file, "OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(%s:.c=.o)))\n", VAR_C_SOURCE);
    fprintf(file, "vpath %%.c $(sort $(dir $(%s)))\n", VAR_C_SOURCE);
    fprintf(file, "# list of ASM program objects\n");
    fprintf(file, "OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(%s:.s=.o)))\n", VAR_ASM_SOURCE);
    fprintf(file, "vpath %%.s $(sort $(dir $(%s)))\n", VAR_ASM_SOURCE);
    fprintf(file, "\n\n");


    fprintf(file, "$(BUILD_DIR)/%%.o: %%.c Makefile | $(BUILD_DIR)\n");
    fprintf(file, "\t@echo \"CC   $<\"\n");
    fprintf(file, "\t@$(CC) -c $(CFLAGS) -MMD -MP \\\n");
    fprintf(file, "\t\t-MF  $(BUILD_DIR)/$(notdir $(<:.c=.d)) \\\n");
    fprintf(file, "\t\t-Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@\n");
    fprintf(file, "\n");

    fprintf(file, "$(BUILD_DIR)/%%.o: %%.s Makefile | $(BUILD_DIR)\n");
    fprintf(file, "\t@echo \"AS   $<\"\n");
    fprintf(file, "\t@$(AS) -c $(CFLAGS) -MMD -MP  \\\n");
    fprintf(file, "\t\t-MF $(BUILD_DIR)/$(notdir $(<:.s=.d)) $< -o $@\n");
    fprintf(file, "\n\n");

    fprintf(file, "$(BUILD_DIR)/$(TARGET)%s: $(OBJECTS) Makefile\n", target);
    fprintf(file, "\t@echo \"LD   $@\"\n");
    fprintf(file, "\t@$(CC) $(OBJECTS) $(LDFLAGS) -o $@\n");
    fprintf(file, "\t@$(OD) $(BUILD_DIR)/$(TARGET)%s -xS > $(BUILD_DIR)/$(TARGET).s $@\n", target);
    fprintf(file, "\t@printf \"\\n\033[32mBuild Successful! \033[0m \\n\"\n");
    fprintf(file, "\t@echo \"ELF   $@\"\n");
    fprintf(file, "\n\n");


    if(!lm_parser_lds_is_empty()) {
        fprintf(file, "$(BUILD_DIR)/%%.hex: $(BUILD_DIR)/%%.elf | $(BUILD_DIR)\n");
        fprintf(file, "\t@echo \"HEX   $@\"\n");
        fprintf(file, "\t@$(HEX) $< $@\n");
        fprintf(file, "\n");

        fprintf(file, "$(BUILD_DIR)/%%.bin: $(BUILD_DIR)/%%.elf | $(BUILD_DIR)\n");
        fprintf(file, "\t@echo \"BIN   $@\"\n");
        fprintf(file, "\t@$(BIN) $< $@\n");
        fprintf(file, "\n");
    }

    fprintf(file, "elf_info: $(BUILD_DIR)/$(TARGET)%s\n", target);
    fprintf(file, "\t@echo \"==================================================================\"\n");
    fprintf(file, "\t@$(SZ) $<\n");
    fprintf(file, "\t@echo \"==================================================================\"\n");
    fprintf(file, "\n\n");

    fprintf(file, "$(BUILD_DIR):\n");
    fprintf(file, "\t@mkdir $@\n");
    fprintf(file, "\n\n");

    fprintf(file, "# Pseudo command\n");
    fprintf(file, ".PHONY: config clean\n");
    fprintf(file, "\n");

    fprintf(file, "# Check if the %s file exists\n", config_file);
    fprintf(file, "config: %s\n", config_file);
    fprintf(file, "\t@./lm.exe -c %s -f %s -o %s -m 50\n", config_file, lm_file, header_file);
    fprintf(file, "\t@rm -rf $(BUILD_DIR)\n");
    fprintf(file, "\n\n");

    fprintf(file, "# clean command, delete build directory\n");
    fprintf(file, "clean:\n");
    fprintf(file, "\t@rm -rf $(BUILD_DIR)\n");
    fprintf(file, "\n");

    fclose(file);
    return LM_OK;
}


int main(int argc, char *argv[])
{
    int ret;
    int opt;

    while ((opt = getopt(argc, argv, "hdvc:f:o:k:p:m:bg:j:u:")) != -1) {
        switch (opt) {
            case 'h':
                show_help(argv[0]);
                exit(0);
                break;
            case 'd':
                show_key_usage();
                exit(0);
                break;
            case 'v':
                printf("lite-manager version "VERSION"\n");
                exit(0);
                break;
            case 'c':
                config_file = optarg;
                break;
            case 'f':
                lm_file = optarg;
                break;
            case 'o':
                header_file = optarg;
                break;
            case 'k':
                confmk_file = optarg;
                break;
            case 'p':
                info_file = optarg;
                break;
            case 'm':
                mem_size = strtol(optarg, NULL, 10);
                break;
            case 'b':
                blind = true;
                break;
            
            case 'g':
                mkfile = optarg;
                break;
            case 'j':
                pro_name = optarg;
                break;
            case 'u':
                build_dir = optarg;
                break;
            
            case '?':
                printf("Unknown option: %c\n", optopt);
                exit(1);
                break;
        }
    }

    int pcode = -1;
#if (_WIN32)
    pcode = system("chcp 65001 >null");  //UTF-8
    if(pcode) {
        LM_LOG_INFO("failed to set page code");
    }

    pcode = system("del null"); // delete null file
    if(pcode) {
        LM_LOG_INFO("failed to del null file");
    }
#elif ( __linux__)
    pcode = system("export LANG=zh_CN.UTF-8");
    if(pcode) {
        LM_LOG_INFO("failed to set page code");
    }
#endif

    lm_mem_init(mem_size);

    lm_parser_init();

    if(!mkfile) {
        ret = lm_parser_config_file(config_file);
        if(ret == LM_ERR) {
            goto error;
        }
    }

    ret = lm_parser_lm_file(NULL, lm_file);
    if(ret == LM_ERR) {
        goto error;
    }

    if(mkfile) {
        lm_gen_mkfile_file(mkfile, pro_name, build_dir);
        lm_gen_projcfg_file(config_file);
        goto exit;
    }

    if(!mkfile) {
        ret = lm_parser_gen_mkconf_file(confmk_file);
        if(ret == LM_ERR) {
            goto error;
        }

        ret = lm_parser_gen_header_file(header_file);
        if(ret == LM_ERR) {
            goto error;
        }

        if(!blind) {
            lm_parser_print_all_macro_value();
        }
    }

exit:
    lm_mem_destroy();
    return 0;

error:
    lm_mem_destroy();
    LM_LOG_ERROR("parser failed, exiting");
    return -1;
}
