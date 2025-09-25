#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

bool is_empty_line(const char *line);
bool check_target_line(const char *line, int line_num, bool *has_target);
bool check_command_line(const char *line, int line_num, bool *has_target);
bool check_line_syntax(const char *line, int line_num, bool *has_target);
bool syntax_check(const char *filename);

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("错误: 需要提供参数\n");
        printf("用法: %s [选项]\n", argv[0]);
        printf("尝试 '%s --help' 获取更多信息\n", argv[0]);
        return 1;
    }

    bool check_syntax = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("用法: %s [选项]\n\n", argv[0]);
            printf("Makefile语法检查工具\n\n");
            printf("选项:\n");
            printf("  -h, --help     显示此帮助信息\n");
            printf("  -s, --syntax-check  进行Makefile语法检查\n");
            return 0;
        } else if (strcmp(argv[i], "--syntax-check") == 0 || strcmp(argv[i], "-s") == 0) {
            check_syntax = true;
        }
    }

    if (check_syntax) {
        printf("开始语法检查...\n");
        if (syntax_check("Makefile")) {
            printf("语法检查通过！\n");
        } else {
            printf("语法检查发现错误！\n");
            return 1;
        }
    }

    return 0;
}

bool syntax_check(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("错误: 无法打开文件 %s\n", filename);
        return false;
    }

    char line[1024];
    int line_num = 0;
    bool has_target = false;

    while (fgets(line, sizeof(line), file)) {
        line_num++;
        line[strcspn(line, "\n")] = '\0';
        if (!check_line_syntax(line, line_num, &has_target)) {
            fclose(file);
            return false; 
        }
    }

    fclose(file);
    return true;
}

bool check_line_syntax(const char *line, int line_num, bool *has_target) {
    if (is_empty_line(line)) {
        return true;
    }
    if (strchr(line, ':') != NULL) {
        return check_target_line(line, line_num, has_target);
    } else if (line[0] == '\t') {
        return check_command_line(line, line_num, has_target);
    } else {
        printf("Line%d: Missing colon in target definition\n", line_num);
        return false;
    }
}


bool check_command_line(const char *line, int line_num, bool *has_target) {
    (void)line; 
    if (!(*has_target)) {
        printf("Line%d: Command found before rule\n", line_num);
        return false;
    }
    return true; 
}

bool check_target_line(const char *line, int line_num, bool *has_target) {
    (void)line; (void)line_num;
    *has_target = true;
    return true;
}

bool is_empty_line(const char *line) {
    for (int i = 0; line[i] != '\0'; i++) {
        if (!isspace((unsigned char)line[i])) {
            return false;
        }
    }
    return true;
}