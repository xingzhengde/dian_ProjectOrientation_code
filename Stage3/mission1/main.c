#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_VERTICES 1000  
#define MAX_NAME_LEN 256  
#define MAX_RULES 256
#define MAX_DEPS 128
#define MAX_CMDS 256
#define MAX_NAME 32
#define MAX_LINE 1024

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
	int  rule_count;
} RuleSet;
typedef struct GraphNode {
    int vertex_index;          
    struct GraphNode *next;    
} GraphNode;

typedef struct {
    char name[MAX_NAME_LEN];   
    int in_degree;             
    bool is_target;            
    GraphNode *adj_list;       
} Vertex;

typedef struct {
    Vertex vertices[MAX_VERTICES];  
    int vertex_count;               
    int target_count;              
} DependencyGraph;

void graph_init(DependencyGraph *graph);
int graph_find_or_add_vertex(DependencyGraph *graph, const char *name, bool is_target);
// 添加依赖边
bool graph_add_dependency(DependencyGraph *graph, const char *from, const char *to);
// 构建依赖图
bool build_dependency_graph(const RuleSet *rs, DependencyGraph *graph);
// 打印
void graph_print(const DependencyGraph *graph);
void graph_destroy(DependencyGraph *graph);
void graph_export_dot(const DependencyGraph *graph, const char *filename);

//直接复制的Stage2的解析器，没有通过多文件的方式
static void rstrip_comment(char *s);
static void rtrim_only(char *s);
static bool split_target_deps(const char *line, char *out_target,
                              char deps[][MAX_NAME + 1], int *dep_count);
static bool parse_makefile_min(const char *filename, RuleSet *rs);


int main(int argc, char *argv[]) {
    const char *mk = "Makefile";
    if (argc > 1 && argv[1][0] != '-') {
        mk = argv[1];
    }

    RuleSet *rs = (RuleSet*)malloc(sizeof(RuleSet));
    if (!rs) { fprintf(stderr, "内存分配失败\n"); return 1; }
    memset(rs, 0, sizeof(RuleSet));
    if (!parse_makefile_min(mk, rs)) {
        free(rs);
        return 1;
    }

    // 基于 RuleSet 构建依赖图并输出
    DependencyGraph graph;
    if (!build_dependency_graph(rs, &graph)) {
        fprintf(stderr, "构建依赖图失败\n");
        free(rs);
        return 1;
    }

    graph_print(&graph);
    graph_export_dot(&graph, "dependencies.dot");
    graph_destroy(&graph);
    free(rs);
    return 0;
}

static void rstrip_comment(char *s) {
    for (int i = 0; s[i]; ++i) {
        if (s[i] == '#') { s[i] = '\0'; break; }
    }
}
static void rtrim_only(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}
static bool split_target_deps(const char *line, char *out_target,
                              char deps[][MAX_NAME + 1], int *dep_count) {
    char buf[MAX_LINE];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    const char *p = buf;
    while (*p == ' ' || *p == '\t') p++;
    char *colon = strchr((char*)p, ':');
    if (!colon) return false;
    *colon = '\0';
    // target = trimmed p
    // 左右去空白
    while (*p && isspace((unsigned char)*p)) p++;
    char *end = (char*)p + strlen(p);
    while (end > p && isspace((unsigned char)end[-1])) *--end = '\0';
    if (*p == '\0') return false;
    strncpy(out_target, p, MAX_NAME);
    out_target[MAX_NAME] = '\0';

    // deps
    char *rest = colon + 1;
    while (*rest && isspace((unsigned char)*rest)) rest++;
    int cnt = 0;
    char *save = NULL;
    for (char *tok = strtok_r(rest, " \t", &save); tok; tok = strtok_r(NULL, " \t", &save)) {
        if (*tok == '\0') continue;
        strncpy(deps[cnt], tok, MAX_NAME);
        deps[cnt][MAX_NAME] = '\0';
        if (++cnt >= MAX_DEPS) break;
    }
    *dep_count = cnt;
    return true;
}
static bool parse_makefile_min(const char *filename, RuleSet *rs) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("fopen Makefile"); return false; }
    rs->rule_count = 0;
    char line[MAX_LINE];
    int line_no = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_no++;
        // 去换行、注释、行尾空白，保留行首以识别命令
        size_t len = strlen(line);
        if (len && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
        rstrip_comment(line);
        rtrim_only(line);
        // 跳过空行与命令行
        bool only_ws = true; for (int i=0; line[i]; ++i) { if (!isspace((unsigned char)line[i])) { only_ws=false; break; } }
        if (line[0] == '\0' || only_ws) continue;
        if (line[0] == '\t') continue; // 忽略命令行

        // 规则行判定：在去左空白的视图上看冒号
        const char *p = line; while (*p==' '||*p=='\t') p++;
        if (!strchr(p, ':')) continue; // 非规则行忽略

        if (rs->rule_count >= MAX_RULES) {
            fprintf(stderr, "Line%d: 规则数量超过限制\n", line_no);
            continue;
        }
        Rule tmp; memset(&tmp, 0, sizeof(tmp)); tmp.line_no = line_no;
        if (!split_target_deps(p, tmp.target, tmp.deps, &tmp.dep_count)) {
            fprintf(stderr, "Line%d: 目标定义格式错误\n", line_no);
            continue;
        }
        rs->rules[rs->rule_count++] = tmp;
    }
    fclose(fp);
    return true;
}
// 初始化图
void graph_init(DependencyGraph *graph) {
    graph->vertex_count = 0;
    graph->target_count = 0;
    for (int i = 0; i < MAX_VERTICES; i++) {
        graph->vertices[i].name[0] = '\0';
        graph->vertices[i].in_degree = 0;
        graph->vertices[i].is_target = false;
        graph->vertices[i].adj_list = NULL;
    }
}

// 查找顶点索引
int graph_find_vertex(const DependencyGraph *graph, const char *name) {
    for (int i = 0; i < graph->vertex_count; i++) {
        if (strcmp(graph->vertices[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// 查找或添加顶点
int graph_find_or_add_vertex(DependencyGraph *graph, const char *name, bool is_target) {
    int index = graph_find_vertex(graph, name);
    if (index >= 0) {
        if (is_target && !graph->vertices[index].is_target) {
            graph->vertices[index].is_target = is_target;
            graph->target_count++;
        }
        return index;
    }
    
    if (graph->vertex_count >= MAX_VERTICES) {
        return -1;  
    }
    
    index = graph->vertex_count;
    strncpy(graph->vertices[index].name, name, MAX_NAME_LEN - 1);
    graph->vertices[index].name[MAX_NAME_LEN - 1] = '\0';
    graph->vertices[index].in_degree = 0;
    graph->vertices[index].is_target = is_target;
    graph->vertices[index].adj_list = NULL;
    
    graph->vertex_count++;
    if (is_target) {
        graph->target_count++;
    }
    
    return index;
}

// 添加依赖边（from → to，表示from依赖to）
bool graph_add_dependency(DependencyGraph *graph, const char *from, const char *to) {
    int from_index = graph_find_or_add_vertex(graph, from, true);  
    int to_index = graph_find_or_add_vertex(graph, to, false);   
    
    if (from_index < 0 || to_index < 0) {
        return false;  
    }
    //避免重复边
    GraphNode *current = graph->vertices[from_index].adj_list;
    while (current != NULL) {
        if (current->vertex_index == to_index) {
            return true;  
        }
        current = current->next;
    }
    
    GraphNode *new_node = (GraphNode *)malloc(sizeof(GraphNode));
    if (!new_node) {
        return false;  
    }
    
    new_node->vertex_index = to_index;
    new_node->next = graph->vertices[from_index].adj_list;
    graph->vertices[from_index].adj_list = new_node;
    
    // 入度
    graph->vertices[to_index].in_degree++;
    
    return true;
}

bool build_dependency_graph(const RuleSet *rs, DependencyGraph *graph) {
    graph_init(graph);
    
    // 添加所有目标和它们的直接依赖
    for (int i = 0; i < rs->rule_count; i++) {
        const Rule *rule = &rs->rules[i];
        int target_index = graph_find_or_add_vertex(graph, rule->target, true);
        
        if (target_index < 0) {
            return false;  
        }
        
        // 添加目标的所有依赖
        for (int j = 0; j < rule->dep_count; j++) {
            if (!graph_add_dependency(graph, rule->target, rule->deps[j])) {
                return false; 
            }
        }
    }
    
    //检查依赖是否也是目标，改is_target
    for (int i = 0; i < graph->vertex_count; i++) {
        if (!graph->vertices[i].is_target) {
            if (graph_find_vertex(graph, graph->vertices[i].name) >= 0) {
                for (int j = 0; j < rs->rule_count; j++) {
                    if (strcmp(rs->rules[j].target, graph->vertices[i].name) == 0) {
                        graph->vertices[i].is_target = true;
                        graph->target_count++;
                        break;
                    }
                }
            }
        }
    }
    
    return true;
}

// 打印依赖图（文本格式）
void graph_print(const DependencyGraph *graph) {
    printf("依赖关系图（共%d个顶点，%d个目标）:\n", 
           graph->vertex_count, graph->target_count);
    printf("========================================\n");
    
    for (int i = 0; i < graph->vertex_count; i++) {
        const Vertex *v = &graph->vertices[i];
        
        printf("%s [入度:%d] %s\n", 
               v->name, 
               v->in_degree,
               v->is_target ? "(目标)" : "(文件)");
        
        // 打印依赖关系
        GraphNode *current = v->adj_list;
        if (current != NULL) {
            printf("  → 依赖: ");
            while (current != NULL) {
                printf("%s", graph->vertices[current->vertex_index].name);
                current = current->next;
                if (current != NULL) {
                    printf(", ");
                }
            }
            printf("\n");
        }
        printf("\n");
    }
}

// 生成Graphviz格式的图
void graph_export_dot(const DependencyGraph *graph, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("无法创建Graphviz文件");
        return;
    }
    
    fprintf(file, "digraph MakefileDependencies {\n");
    fprintf(file, "  rankdir=TB;\n");
    fprintf(file, "  node [shape=box, style=filled];\n\n");
    
    // 定义节点样式
    for (int i = 0; i < graph->vertex_count; i++) {
        const Vertex *v = &graph->vertices[i];
        if (v->is_target) {
            fprintf(file, "  \"%s\" [fillcolor=lightblue];\n", v->name);
        } else {
            fprintf(file, "  \"%s\" [fillcolor=lightgreen];\n", v->name);
        }
    }
    
    fprintf(file, "\n");
    
    // 添加边
    for (int i = 0; i < graph->vertex_count; i++) {
        const Vertex *v = &graph->vertices[i];
        GraphNode *current = v->adj_list;
        
        while (current != NULL) {
            fprintf(file, "  \"%s\" -> \"%s\";\n", 
                    v->name, 
                    graph->vertices[current->vertex_index].name);
            current = current->next;
        }
    }
    
    fprintf(file, "}\n");
    fclose(file);
    printf("依赖图已导出到: %s (使用Graphviz可视化)\n", filename);
}

void graph_destroy(DependencyGraph *graph) {
    for (int i = 0; i < graph->vertex_count; i++) {
        GraphNode *current = graph->vertices[i].adj_list;
        while (current != NULL) {
            GraphNode *temp = current;
            current = current->next;
            free(temp);
        }
        graph->vertices[i].adj_list = NULL;
    }
    graph->vertex_count = 0;
    graph->target_count = 0;
}
