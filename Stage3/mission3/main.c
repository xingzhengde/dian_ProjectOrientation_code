// mission3: 使用 common API 实现时间戳判断与按需构建 + 自举示例
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../common/make_types.h"

typedef struct {
    const char *mkfile;
    const char *cli_target;
    bool show_latest;
} Opts;

static void parse_args(int argc, char *argv[], Opts *o){
    // 默认使用当前目录下的自举文件 MiniMakefile
    o->mkfile = "./MiniMakefile";
    o->cli_target = NULL;
    o->show_latest = false;
    for(int i=1;i<argc;i++){
        if(strncmp(argv[i], "--mk=", 5) == 0) {
            o->mkfile = argv[i] + 5;
        } else if(strcmp(argv[i], "--show-latest") == 0) {
            o->show_latest = true;
        } else if(argv[i][0] != '-') {
            o->cli_target = argv[i];
        }
    }
}

static int pick_target_index(const MkRuleSet *rs, const char *cli){
    if(cli){
        for(int i=0;i<rs->rule_count;i++){
            if(strcmp(rs->rules[i].target, cli) == 0) return i;
        }
        fprintf(stderr, "警告: 指定目标 %s 未找到, 使用第一个\n", cli);
    }
    if(rs->rule_count == 0) return -1;
    return 0;
}

static int execute_build(const MkRuleSet *rs, const char *final_target, bool show_latest){
    MkGraph g;
    if(!mk_build_graph(&g, rs)) {
        fprintf(stderr, "构图失败\n");
        return 1;
    }
    int final_idx = mk_find_target_index(&g, final_target);
    if(final_idx < 0){
        fprintf(stderr, "错误: 目标不在图中 %s\n", final_target);
        mk_free_graph(&g);
        return 1;
    }
    bool *needed = (bool*)calloc(g.n, sizeof(bool));
    if(!needed){ fprintf(stderr, "内存不足(needed)\n"); mk_free_graph(&g); return 1; }
    mk_mark_needed(&g, final_idx, needed);
    int *order = (int*)malloc(sizeof(int)*g.n); int on = 0;
    if(!order){ fprintf(stderr, "内存不足(order)\n"); free(needed); mk_free_graph(&g); return 1; }
    mk_topo_needed(&g, needed, order, &on);

    // 逆向遍历保证依赖先于目标
    for(int i = on - 1; i >= 0; --i){
        int idx = order[i];
        if(!needed[idx]) continue;
        const char *name = g.v[idx].name;
        const MkRule *rule = mk_find_rule(rs, name);
        if(!rule) continue; // 纯文件依赖
        int act = mk_should_rebuild(rule);
        if(act == -1){
            fprintf(stderr, "错误: 目标 %s 缺失依赖\n", name);
            free(order); free(needed); mk_free_graph(&g); return 1;
        }
        if(act == 1){
            if(mk_exec_rule(rule) != 0){
                free(order); free(needed); mk_free_graph(&g); return 1;
            }
        } else if(show_latest){
            printf("%s 已最新\n", name);
        }
    }

    free(order); free(needed); mk_free_graph(&g); return 0;
}

int main(int argc, char *argv[]){
    Opts opt; parse_args(argc, argv, &opt);
    MkRuleSet rs;
    if(!mk_parse_makefile(opt.mkfile, &rs)) return 1;
    if(rs.rule_count == 0){ fprintf(stderr, "错误: 没有规则\n"); return 1; }
    int tindex = pick_target_index(&rs, opt.cli_target);
    if(tindex < 0){ fprintf(stderr, "错误: 不能选择目标\n"); return 1; }
    const char *final_target = rs.rules[tindex].target;
    return execute_build(&rs, final_target, opt.show_latest);
}
