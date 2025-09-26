#include "make_types.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int file_mtime(const char *path, time_t *out){ struct stat st; if(stat(path,&st)==0){ *out=st.st_mtime; return 0;} return -1; }
static int file_exists(const char *p){ return access(p,F_OK)==0; }

int mk_should_rebuild(const MkRule *rule){
    if(!file_exists(rule->target)){
        for(int i=0;i<rule->dep_count;i++) if(!file_exists(rule->deps[i])) { fprintf(stderr, "缺失依赖: %s -> %s\n", rule->target, rule->deps[i]); return -1; } // 缺依赖
        return 1; // 目标不存在，依赖齐全
    }
    time_t tgt_mt=0; if(file_mtime(rule->target,&tgt_mt)!=0) return 1; // 无法获取，尝试重建
    for(int i=0;i<rule->dep_count;i++){
        time_t dep_mt=0; if(file_mtime(rule->deps[i],&dep_mt)!=0){ fprintf(stderr,"警告: 依赖 %s 无法获取时间\n", rule->deps[i]); continue; }
        if(dep_mt > tgt_mt) return 1; // 依赖更新
    }
    return 0; // 无需重建
}

int mk_exec_rule(const MkRule *rule){ for(int i=0;i<rule->cmd_count;i++){ const char *cmd=rule->cmds[i]; if(!cmd[0]) continue; printf("%s\n", cmd); int rc=system(cmd); if(rc!=0){ fprintf(stderr,"命令失败(%d): %s\n", rc, cmd); return rc; } } return 0; }
