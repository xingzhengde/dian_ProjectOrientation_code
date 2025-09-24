#include <stdio.h>


// 显示帮助信息函数
void show_help(const char *program_name) {
    printf("用法: %s [选项]\n\n", program_name);
    printf("这是一个基础命令行程序框架\n\n");
    printf("选项:\n");
    printf("  --help    显示此帮助信息并退出\n");
    printf("  --version 显示程序版本\n");
}