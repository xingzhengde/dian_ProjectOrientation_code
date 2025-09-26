#ifndef MAKE_TYPES_H
#define MAKE_TYPES_H
#include <stdbool.h>
#include <time.h>

#define MK_MAX_RULES 256
#define MK_MAX_DEPS 128
#define MK_MAX_CMDS 32
#define MK_MAX_CMD_LEN 256
#define MK_MAX_NAME 128
#define MK_MAX_LINE 1024
#define MK_MAX_VERTS 1024

typedef struct {
    char target[MK_MAX_NAME+1];
    char deps[MK_MAX_DEPS][MK_MAX_NAME+1];
    int  dep_count;
    char cmds[MK_MAX_CMDS][MK_MAX_CMD_LEN];
    int  cmd_count;
    int  line_no;
} MkRule;

typedef struct {
    MkRule rules[MK_MAX_RULES];
    int rule_count;
} MkRuleSet;

typedef struct MkEdgeNode { int to; struct MkEdgeNode *next; } MkEdgeNode;

typedef struct { char name[MK_MAX_NAME+1]; bool is_target; MkEdgeNode *adj; } MkVtx;

typedef struct { MkVtx v[MK_MAX_VERTS]; int n; } MkGraph;

// Parse API
bool mk_parse_makefile(const char *filename, MkRuleSet *rs);

// Graph build
bool mk_build_graph(MkGraph *g, const MkRuleSet *rs);
void mk_free_graph(MkGraph *g);

// Dependency marking + topo
void mk_mark_needed(const MkGraph *g, int tgt, bool *vis);
int  mk_topo_needed(const MkGraph *g, const bool *needed, int *order_out, int *out_n);
int  mk_find_target_index(const MkGraph *g, const char *name);

// Rebuild logic
int  mk_should_rebuild(const MkRule *rule);
int  mk_exec_rule(const MkRule *rule);

// Rule lookup
const MkRule* mk_find_rule(const MkRuleSet *rs, const char *target);

#endif // MAKE_TYPES_H
