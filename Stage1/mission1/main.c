#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "help.h"
#include "argumentRecognize.h"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("错误: 需要提供参数\n");
        printf("用法: %s [选项]\n", argv[0]);
        printf("尝试 '%s --help' 获取更多信息\n", argv[0]);
        return 1;
    }
    
    // 情况2: 遍历所有参数
    bool has_valid_arg = recognize_argument(argc, argv);
    
    
    return 0;
}