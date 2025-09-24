#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "help.h"
#include "argumentRecognize.h"

bool recognize_argument(int argc, char *argv[]){
    for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--help") == 0) {
                show_help(argv[0]);
            }else if (strcmp(argv[i], "--version") == 0) {
                printf("程序版本 1.0\n");
            } else {
                // 未知参数处理
                printf("错误: 未知选项 '%s'\n", argv[i]);
                printf("尝试 '%s --help' 获取可用选项\n", argv[0]);
                return 0;
            }
        }
        return 1;
}    