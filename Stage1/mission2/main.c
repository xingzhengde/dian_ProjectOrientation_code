#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

bool file_exists(const char *filename);
char* process_line(char *line);
void process_makefile(bool verbose_mode);
void show_help(const char *program_name);

int main(int argc, char *argv[]) {
    bool verbose_mode = false;
    bool help_mode = false;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            help_mode = true;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose_mode = true;
        } else {
            printf("错误: 未知参数 '%s'\n", argv[i]);
            printf("使用 --help 查看用法\n");
            return 1;
        }
    }
    
    if (help_mode) {
        show_help(argv[0]);
        return 0;
    }
    
    process_makefile(verbose_mode);
    
    return 0;
}

void show_help(const char *program_name) {
    printf("用法: %s [选项]\n\n", program_name);
    printf("Makefile预处理工具\n\n");
    printf("选项:\n");
    printf("  -h, --help     显示此帮助信息\n");
    printf("  -v, --verbose  输出清理后的内容到Minimake_cleared.mk文件\n");
}

bool file_exists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

void process_makefile(bool verbose_mode) {

    if (!file_exists("./Makefile")) {
        printf("错误: Makefile文件不存在\n");
        return;
    }
    
    FILE *input_file = fopen("./Makefile", "r");
    FILE *output_file = NULL;
    
    if (verbose_mode) {
        output_file = fopen("Minimake_cleared.mk", "w");
        if (!output_file) {
            printf("错误: 无法创建输出文件\n");
            fclose(input_file);
            return;
        }
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), input_file)) {
        char *processed_line = process_line(line);
        if (processed_line != NULL && strlen(processed_line) > 0) {

            if (verbose_mode && output_file) {
                fprintf(output_file, "%s\n", processed_line);
            }

            //printf("处理后的行: %s\n", processed_line);
        }
    }
    
    fclose(input_file);
    if (output_file) fclose(output_file);
}

char* process_line(char *line) {
    static char result[1024];
    strcpy(result, line);
    
    result[strcspn(result, "\n")] = '\0';
    
    int len = strlen(result);
    while (len > 0 && isspace(result[len - 1])) {
        result[--len] = '\0';
    }
    
    char *comment_start = strchr(result, '#');
    if (comment_start != NULL) {
        *comment_start = '\0';
        // 再次去除可能因注释删除产生的行尾空格
        len = strlen(result);
        while (len > 0 && isspace(result[len - 1])) {
            result[--len] = '\0';
        }
    }
    
    //检查是否为空行
    bool is_empty = true;
    for (int i = 0; result[i] != '\0'; i++) {
        if (!isspace(result[i])) {
            is_empty = false;
            break;
        }
    }
    
    return is_empty ? NULL : result;
}