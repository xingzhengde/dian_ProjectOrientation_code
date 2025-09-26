#include "make_types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int find_idx(const MkGraph *g, const char *name){ for(int i=0;i<g->n;i++) if(strcmp(g->v[i].name,name)==0) return i; return -1; }
static int get_idx(MkGraph *g, const char *name, bool as_target){ int i=find_idx(g,name); if(i>=0){ if(as_target) g->v[i].is_target=true; return i; } if(g->n>=MK_MAX_VERTS) return -1; i=g->n; memset(&g->v[i],0,sizeof(g->v[i])); strncpy(g->v[i].name,name,MK_MAX_NAME); g->v[i].name[MK_MAX_NAME]='\0'; g->v[i].is_target=as_target; g->v[i].adj=NULL; g->n++; return i; }

bool mk_build_graph(MkGraph *g, const MkRuleSet *rs){
    if(!g||!rs) return false; memset(g,0,sizeof(*g));
    for(int i=0;i<rs->rule_count;i++){
        const MkRule *r=&rs->rules[i]; int t=get_idx(g,r->target,true); if(t<0) continue;
        for(int d=0; d<r->dep_count; d++){ int di=get_idx(g,r->deps[d],false); if(di<0) continue; MkEdgeNode *e=(MkEdgeNode*)malloc(sizeof(MkEdgeNode)); if(!e){fprintf(stderr,"内存不足(边)\n"); return false;} e->to=di; e->next=g->v[t].adj; g->v[t].adj=e; }
    }
    return true;
}

void mk_free_graph(MkGraph *g){ if(!g) return; for(int i=0;i<g->n;i++){ MkEdgeNode *e=g->v[i].adj; while(e){ MkEdgeNode*n=e->next; free(e); e=n;} g->v[i].adj=NULL;} g->n=0; }

int mk_find_target_index(const MkGraph *g, const char *name){ return find_idx(g,name); }

void mk_mark_needed(const MkGraph *g, int tgt, bool *vis){ int *q=(int*)malloc(sizeof(int)*g->n); int f=0,r=0; vis[tgt]=true; q[r++]=tgt; while(f<r){ int u=q[f++]; for(MkEdgeNode*e=g->v[u].adj;e;e=e->next){ int v=e->to; if(!vis[v]){ vis[v]=true; q[r++]=v; } } } free(q);} 

int mk_topo_needed(const MkGraph *g, const bool *needed, int *order_out, int *out_n){ int *indeg=(int*)calloc(g->n,sizeof(int)); int *q=(int*)malloc(sizeof(int)*g->n); int f=0,r=0,cnt=0; for(int u=0;u<g->n;u++){ if(!needed[u]) continue; for(MkEdgeNode*e=g->v[u].adj;e;e=e->next){ int v=e->to; if(needed[v]) indeg[v]++; } } for(int i=0;i<g->n;i++) if(needed[i] && indeg[i]==0) q[r++]=i; while(f<r){ int u=q[f++]; order_out[cnt++]=u; for(MkEdgeNode*e=g->v[u].adj;e;e=e->next){ int v=e->to; if(!needed[v]) continue; if(--indeg[v]==0) q[r++]=v; } } *out_n=cnt; free(indeg); free(q); return 0; }
