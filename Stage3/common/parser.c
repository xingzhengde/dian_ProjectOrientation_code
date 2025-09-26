#include "make_types.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

static void strip_comment(char *s){ for(int i=0;s[i];++i){ if(s[i]=='#'){ s[i]='\0'; break; } } }
static void rtrim(char *s){ int n=(int)strlen(s); while(n>0 && (unsigned char)s[n-1] <= ' '){ s[--n]='\0'; } }

static int parse_header(const char *line, char *out_target, char deps[][MK_MAX_NAME+1], int *dep_count){
    char buf[MK_MAX_LINE]; strncpy(buf,line,sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
    char *colon = strchr(buf, ':'); if(!colon) return 0; *colon='\0';
    char *rest = colon+1; while(*rest==' '||*rest=='\t') rest++;
    char *t=buf; while(*t==' '||*t=='\t') t++; char *end=t+strlen(t); while(end>t && isspace((unsigned char)end[-1])) *--end='\0';
    if(*t=='\0') return 0;
    strncpy(out_target,t,MK_MAX_NAME); out_target[MK_MAX_NAME]='\0';
    int cnt=0; char *tok=strtok(rest," \t"); while(tok && cnt < MK_MAX_DEPS){ strncpy(deps[cnt], tok, MK_MAX_NAME); deps[cnt][MK_MAX_NAME]='\0'; cnt++; tok=strtok(NULL," \t"); }
    *dep_count=cnt; return 1;
}

bool mk_parse_makefile(const char *filename, MkRuleSet *rs){
    FILE *fp=fopen(filename,"r"); if(!fp){ fprintf(stderr,"错误: 打开 %s 失败: %s\n", filename, strerror(errno)); return false; }
    rs->rule_count=0; char line[MK_MAX_LINE]; int line_no=0; MkRule *last=NULL;
    while(fgets(line,sizeof(line),fp)){
        line_no++; line[strcspn(line,"\n")]='\0'; line[strcspn(line,"\r")]='\0';
        strip_comment(line); rtrim(line); if(line[0]=='\0') continue;
        if(line[0]=='\t'){
            if(!last){ fprintf(stderr,"Line%d: 无规则的命令行\n", line_no); continue; }
            if(last->cmd_count >= MK_MAX_CMDS){ fprintf(stderr,"Line%d: 命令过多\n", line_no); continue; }
            char *cmd=line+1; while(*cmd=='\t'||*cmd==' ') cmd++;
            strncpy(last->cmds[last->cmd_count], cmd, MK_MAX_CMD_LEN-1);
            last->cmds[last->cmd_count][MK_MAX_CMD_LEN-1]='\0';
            last->cmd_count++; continue; }
        if(!strchr(line,':')) continue;
        if(rs->rule_count >= MK_MAX_RULES){ fprintf(stderr,"Line%d: 规则过多\n", line_no); continue; }
        MkRule tmp; memset(&tmp,0,sizeof(tmp)); tmp.line_no=line_no;
        if(!parse_header(line,tmp.target,tmp.deps,&tmp.dep_count)){ fprintf(stderr,"Line%d: 规则头解析失败\n", line_no); continue; }
        rs->rules[rs->rule_count]=tmp; last=&rs->rules[rs->rule_count]; rs->rule_count++;
    }
    fclose(fp); return true;
}

const MkRule* mk_find_rule(const MkRuleSet *rs, const char *target){ for(int i=0;i<rs->rule_count;i++) if(strcmp(rs->rules[i].target,target)==0) return &rs->rules[i]; return NULL; }
