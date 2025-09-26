#include <stdio.h>
#include <stdlib.h>
// mission2 现在使用 common/ 目录中的通用实现，只负责输出依赖顺序
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../common/make_types.h"

static int pick_target(const MkGraph *g, const MkRuleSet *rs, const char *cli){
    if(cli){ int idx = mk_find_target_index(g, cli); if(idx>=0) return idx; fprintf(stderr,"警告: 指定目标 %s 未找到, 使用第一个\n", cli); }
    if(rs->rule_count==0) return -1; return mk_find_target_index(g, rs->rules[0].target);
}

int main(int argc, char *argv[]){
    const char *mk = "../mission1/Makefile"; const char *cli=NULL;
    for(int i=1;i<argc;i++){ if(strncmp(argv[i],"--mk=",5)==0) mk=argv[i]+5; else if(argv[i][0] != '-') cli=argv[i]; }
    MkRuleSet rs; if(!mk_parse_makefile(mk,&rs)){ fprintf(stderr,"解析失败\n"); return 1; }
    if(rs.rule_count==0){ fprintf(stderr,"错误: 空规则集\n"); return 1; }
    MkGraph g; if(!mk_build_graph(&g,&rs)){ fprintf(stderr,"构图失败\n"); return 1; }
    int tgt = pick_target(&g,&rs,cli); if(tgt<0){ fprintf(stderr,"错误: 无法选择目标\n"); mk_free_graph(&g); return 1; }
    bool *needed = (bool*)calloc(g.n,sizeof(bool)); if(!needed){ fprintf(stderr,"内存不足\n"); mk_free_graph(&g); return 1; }
    mk_mark_needed(&g, tgt, needed);
    int *order=(int*)malloc(sizeof(int)*g.n); int on=0; if(!order){ fprintf(stderr,"内存不足(order)\n"); free(needed); mk_free_graph(&g); return 1; }
    mk_topo_needed(&g, needed, order, &on);
    for(int i=on-1;i>=0;i--){ int idx=order[i]; if(!needed[idx]) continue; printf("%s\n", g.v[idx].name); }
    free(order); free(needed); mk_free_graph(&g); return 0; }