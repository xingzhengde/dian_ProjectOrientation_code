#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>

#define MAX_RULES 256
#define MAX_DEPS 128
#define MAX_CMDS 256
#define MAX_NAME 32
#define MAX_LINE 1024
#define MAX_VERTS 1024
#define MAX_NAME_LEN 256

typedef struct {
    char target[MAX_NAME+1];
    char deps[MAX_DEPS][MAX_NAME+1];
    int  dep_count;
    char cmds[MAX_CMDS][256];
    int  cmd_count;
    int  line_no;
} Rule;

typedef struct { 
    Rule rules[MAX_RULES]; 
    int rule_count; 
} RuleSet;

static void rstrip_comment(char *s) { 
    for(int i = 0; s[i]; ++i) {
        if(s[i] == '#') { 
            s[i] = '\0'; 
            break; 
        }
    } 
}

static void rtrim_only(char *s) { 
    int n = (int)strlen(s); 
    while(n > 0 && isspace((unsigned char)s[n-1])) {
        s[--n] = '\0'; 
    }
}

static int split_target_deps(const char *line, char *out_target,
                             char deps[][MAX_NAME + 1], int *dep_count) {
    char buf[MAX_LINE];
    strncpy(buf, line, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';

    const char *p = buf;
    while(*p == ' ' || *p == '\t') p++;
    
    char *colon = strchr((char*)p, ':');
    if (!colon) { return 0; }
    *colon = '\0';

    char *target_start = (char*)p;
    char *target_end = colon;
    while (target_end > target_start && isspace((unsigned char)*(target_end-1))) {
        target_end--;
    }
    *target_end = '\0';
    
    if (*target_start == '\0') { return 0; }
    
    strncpy(out_target, target_start, MAX_NAME);
    out_target[MAX_NAME] = '\0';


    char *rest = colon + 1;
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
    int cnt = 0;
    char *saveptr = NULL;
    char *tok = strtok_r(rest, " \t", &saveptr);
    
    while (tok != NULL && cnt < MAX_DEPS) {
        if (*tok != '\0') {
            strncpy(deps[cnt], tok, MAX_NAME);
            deps[cnt][MAX_NAME] = '\0';
            /* dependency captured */
            cnt++;
        }
        tok = strtok_r(NULL, " \t", &saveptr);
    }
    *dep_count = cnt;
    (void)cnt; 
    return 1;
}

static bool parse_makefile_min(const char *filename, RuleSet *rs) {
    FILE *fp = fopen(filename, "r");
    if(!fp) { 
        fprintf(stderr, "错误: 无法打开文件 %s\n", filename); 
        return false; 
    }
    
    rs->rule_count = 0; 
    char line[MAX_LINE]; 
    int line_no = 0;
    
    while(fgets(line, sizeof(line), fp)) {
        line_no++;
        

        line[strcspn(line, "\n")] = '\0';
        line[strcspn(line, "\r")] = '\0';
        
        rstrip_comment(line);
        rtrim_only(line);

        bool only_ws = true;
        for(int i = 0; line[i]; ++i) { 
            if(!isspace((unsigned char)line[i])) { 
                only_ws = false; 
                break; 
            } 
        }
        
        if(line[0] == '\0' || only_ws) { continue; }
        

        if(line[0] == '\t') { continue; }
        
        const char *p = line;
        while(*p == ' ' || *p == '\t') p++;
        
        if(!strchr(p, ':')) { continue; }
        
        if(rs->rule_count >= MAX_RULES) { 
            fprintf(stderr, "Line%d: 规则数量超过限制\n", line_no); 
            continue; 
        }
        
        Rule tmp; 
        memset(&tmp, 0, sizeof(tmp)); 
        tmp.line_no = line_no;
        
        if(!split_target_deps(p, tmp.target, tmp.deps, &tmp.dep_count)) {
            fprintf(stderr, "Line%d: 目标定义格式错误: %s\n", line_no, p);
            continue;
        }
        
        rs->rules[rs->rule_count] = tmp;
        rs->rule_count++;

    }
    
    fclose(fp); 
 
    return true;
}


typedef struct EdgeNode { 
    int to; 
    struct EdgeNode *next; 
} EdgeNode;

typedef struct {
    char name[MAX_NAME_LEN];
    bool is_target;
    EdgeNode *adj;     

} Vtx;

typedef struct {
    Vtx v[MAX_VERTS];
    int n;
} Graph;


static int find_idx(const Graph *g, const char *name) {
    if (!g || !name) return -1;
    
    for(int i = 0; i < g->n; ++i) {
        if(strcmp(g->v[i].name, name) == 0) return i;
    }
    return -1;
}

static int get_idx(Graph *g, const char *name, bool as_target) {
    if (!g || !name) {
        fprintf(stderr, "错误: get_idx 参数为空\n");
        return -1;
    }
    
    int i = find_idx(g, name);
    if(i >= 0) { 
        if(as_target) { g->v[i].is_target = true; }
        return i; 
    }
    
    if(g->n >= MAX_VERTS) {
        fprintf(stderr, "错误: 顶点数量超过限制 %d\n", MAX_VERTS);
        return -1;
    }
    
    i = g->n;
    Vtx *vx = &g->v[i];
    memset(vx, 0, sizeof(*vx));
    strncpy(vx->name, name, MAX_NAME_LEN-1);
    vx->name[MAX_NAME_LEN-1] = '\0';
    vx->is_target = as_target;
    vx->adj = NULL;
    g->n++;
    
    
    return i;
}

static bool link_edge(Graph *g, int from, int to) {
    if (!g || from < 0 || from >= g->n || to < 0 || to >= g->n) {
        fprintf(stderr, "错误: link_edge 参数无效: from=%d, to=%d, n=%d\n", from, to, g->n);
        return false;
    }
    
    // 检查边是否已存在
    for(EdgeNode *e = g->v[from].adj; e != NULL; e = e->next) { if(e->to == to) return true; }
    
    EdgeNode *e = (EdgeNode*)malloc(sizeof(EdgeNode));
    if(!e) {
        fprintf(stderr, "错误: 内存分配失败\n");
        return false;
    }
    
    e->to = to;
    e->next = g->v[from].adj;
    g->v[from].adj = e;
    
    
    return true;
}

static void free_graph(Graph *g) {
    if (!g) return;
    
    for(int i = 0; i < g->n; ++i) {
        EdgeNode *e = g->v[i].adj;
        while(e != NULL) { 
            EdgeNode *next = e->next; 
            free(e); 
            e = next; 
        }
        g->v[i].adj = NULL;
    }
    g->n = 0;
}

static bool build_graph_from_rules(Graph *g, const RuleSet *rs) {
    if (!g || !rs) {
        fprintf(stderr, "错误: build_graph_from_rules 参数为空\n");
        return false;
    }
    
    memset(g, 0, sizeof(*g));
    
    for(int i = 0; i < rs->rule_count; ++i) {
        const Rule *rule = &rs->rules[i];
        
        int target_idx = get_idx(g, rule->target, true);
        if(target_idx < 0) {
            fprintf(stderr, "错误: 无法添加目标顶点 '%s'\n", rule->target);
            continue;
        }
        
        for(int j = 0; j < rule->dep_count; ++j) {
            const char *dep_name = rule->deps[j];
            
            int dep_idx = get_idx(g, dep_name, false);
            if(dep_idx < 0) {
                fprintf(stderr, "错误: 无法添加依赖顶点 '%s'\n", dep_name);
                continue;
            }
            
            if(!link_edge(g, target_idx, dep_idx)) {
                fprintf(stderr, "错误: 添加边失败: %s -> %s\n", rule->target, dep_name);
            }
        }
    }
    

    return true;
}

static int pick_final_target(const Graph *g, const RuleSet *rs, const char *cli_target) {
    if (!g || !rs) {
        fprintf(stderr, "错误: pick_final_target 参数为空\n");
        return -1;
    }
    
    if(cli_target != NULL && cli_target[0] != '\0') {
        int idx = find_idx(g, cli_target);
        if(idx >= 0) {

            return idx;
        }
        fprintf(stderr, "警告: 命令行目标 '%s' 未找到\n", cli_target);
    }
    
    if(rs->rule_count > 0) {
        int idx = find_idx(g, rs->rules[0].target);
        if(idx >= 0) { return idx; }
    }
    
    fprintf(stderr, "错误: 未找到任何目标\n");
    return -1;
}

// 使用BFS标记所有依赖项
static void mark_dependencies(const Graph *g, int target_idx, bool visited[]) {
    if (!g || target_idx < 0 || target_idx >= g->n || !visited) {
        fprintf(stderr, "错误: mark_dependencies 参数无效\n");
        return;
    }
    int *queue = (int*)malloc(sizeof(int)*g->n);
    if(!queue){ fprintf(stderr, "错误: 内存不足 (queue)\n"); return; }
    int front = 0, rear = 0;
    queue[rear++] = target_idx;
    visited[target_idx] = true;
    while (front < rear) {
        int u = queue[front++];
        for (EdgeNode *e = g->v[u].adj; e != NULL; e = e->next) {
            int v = e->to;
            if (v >= 0 && v < g->n && !visited[v]) {
                visited[v] = true;
                queue[rear++] = v;
            }
        }
    }
    free(queue);
}

// Kahn拓扑排序
static int topo_kahn(const Graph *g, const bool needed[], int *order_out, int *out_n) {
    if (!g || !needed || !order_out || !out_n) {
        fprintf(stderr, "错误: topo_kahn 参数为空\n");
        return -1;
    }
    
    if(g->n == 0) {
        *out_n = 0;
        return 0;
    }
    
    int *indeg = (int*)calloc(g->n, sizeof(int));
    int *queue = (int*)malloc(sizeof(int)*g->n);
    if(!indeg || !queue){ fprintf(stderr, "错误: 内存不足 (topo arrays)\n"); free(indeg); free(queue); return -1; }
    int front = 0, rear = 0;
    
    // 计算入度
    for(int u = 0; u < g->n; u++) {
        if(!needed[u]) continue;
        
        for(EdgeNode *e = g->v[u].adj; e != NULL; e = e->next) {
            int v = e->to;
            if(v >= 0 && v < g->n && needed[v]) {
                indeg[v]++;
            }
        }
    }
    
    // 入度为0的顶点入队
    for(int i = 0; i < g->n; i++) {
        if(needed[i] && indeg[i] == 0) {
            queue[rear++] = i;
        }
    }
    
    int cnt = 0;
    while(front < rear && cnt < g->n) {
        int u = queue[front++];
        order_out[cnt++] = u;
        
        for(EdgeNode *e = g->v[u].adj; e != NULL; e = e->next) {
            int v = e->to;
            if(v < 0 || v >= g->n || !needed[v]) continue;
            
            if(--indeg[v] == 0) {
                queue[rear++] = v;
            }
        }
    }
    
    *out_n = cnt;
    
    // 检查是否有环
    if(cnt != g->n) {
        for(int i = 0; i < g->n; i++) {
            if(needed[i] && indeg[i] > 0) {
                fprintf(stderr, "警告: 可能的循环依赖: '%s' (剩余入度=%d)\n", g->v[i].name, indeg[i]);
            }
        }
    }
    /* topo done */
    free(indeg); free(queue);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *mk = "../mission1/Makefile"; 
    const char *final_target = NULL;
    
    for(int i = 1; i < argc; ++i) {
        if(strncmp(argv[i], "--mk=", 5) == 0) {
            mk = argv[i] + 5;
        } else if(argv[i][0] != '-') {
            final_target = argv[i];
        }
    }
    
    RuleSet *rs = (RuleSet*)malloc(sizeof(RuleSet));
    if(!rs){ fprintf(stderr, "错误: 内存不足 (RuleSet)\n"); return 1; }
    memset(rs, 0, sizeof(*rs));
    
    if(!parse_makefile_min(mk, rs)) {
        fprintf(stderr, "解析Makefile失败\n");
        free(rs);
        return 1;
    }
    
    if (rs->rule_count == 0) {
        fprintf(stderr, "错误: Makefile中没有找到任何规则\n");
        free(rs);
        return 1;
    }
    
    Graph g; 
    if (!build_graph_from_rules(&g, rs)) {
        fprintf(stderr, "构建图失败\n");
        free(rs);
        return 1;
    }
    
    if (g.n == 0) {
        fprintf(stderr, "错误: 图中没有顶点\n");
        return 1;
    }
    
    int tgt = pick_final_target(&g, rs, final_target);
    if(tgt < 0 || tgt >= g.n) {
        fprintf(stderr, "错误: 最终目标索引无效: %d\n", tgt);
        free_graph(&g);
        return 1;
    }
    
    /* final target selected */
    
    bool *needed = (bool*)calloc(g.n, sizeof(bool));
    if(!needed){ fprintf(stderr, "错误: 内存不足 (needed)\n"); free_graph(&g); return 1; }
    mark_dependencies(&g, tgt, needed);
    
    int needed_count = 0;
    for(int i = 0; i < g.n; i++) if(needed[i]) needed_count++;
    /* needed count: %d */
    
    if (needed_count == 0) {
        fprintf(stderr, "错误: 没有需要构建的顶点\n");
        free_graph(&g);
        return 1;
    }
    
    int *order = (int*)malloc(sizeof(int)*g.n); int on = 0;
    if(!order){ fprintf(stderr, "错误: 内存不足 (order)\n"); free(needed); free_graph(&g); return 1; }
    
    int rc = topo_kahn(&g, needed, order, &on);
    if(rc != 0) {
        fprintf(stderr, "拓扑排序失败\n");
        free_graph(&g);
        return 1;
    }

    for(int i = on-1; i >=0; --i) {
        int idx = order[i];
        if (idx < 0 || idx >= g.n) { fprintf(stderr, "错误: 无效的顶点索引 %d\n", idx); continue; }
        printf("%s\n", g.v[idx].name);
    }

    free(order);
    free(needed);
    free_graph(&g);
    free(rs);
    /* end */
    return 0;
}