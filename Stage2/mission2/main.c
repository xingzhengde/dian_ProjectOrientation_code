// minimake: 解析 + 静态检查 + 运行指定目标（最小实现）
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

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

static void trim(char *s) {
	int n = (int)strlen(s);
	while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
	int i = 0; 
	while (isspace((unsigned char)s[i])) i++;
	if (i) memmove(s, s + i, n - i + 1);
}

static void rstrip_comment(char *s) {
	for (int i = 0; s[i]; ++i) {
		if (s[i] == '#') { s[i] = '\0'; break; }
	}
}

static int find_rule_index(const RuleSet *rs, const char *name) {
	for (int i = 0; i < rs->rule_count; ++i)
		if (strcmp(rs->rules[i].target, name) == 0) return i;
	return -1;
}

static bool split_target_deps(const char *line, char *out_target,
							  char deps[][MAX_NAME + 1], int *dep_count) {
	char line_copy[MAX_LINE];
	strncpy(line_copy, line, sizeof(line_copy) - 1);
	line_copy[sizeof(line_copy) - 1] = '\0';
	char *colon = strchr(line_copy, ':');
	if (!colon) return false;
	*colon = '\0';
	trim(line_copy);
	if (line_copy[0] == '\0' || strlen(line_copy) > MAX_NAME) 
		return false;
	strncpy(out_target, line_copy, MAX_NAME);
	out_target[MAX_NAME] = '\0';
	char *rest = colon + 1;
	trim(rest);
	int cnt = 0;
	char *tok = strtok(rest, " \t");
	while (tok && cnt < MAX_DEPS) {
		if (strlen(tok) <= MAX_NAME) {
			strncpy(deps[cnt], tok, MAX_NAME);
			deps[cnt][MAX_NAME] = '\0';
			cnt++;
		}
		tok = strtok(NULL, " \t");
	}
	*dep_count = cnt;
	return true;
}

static bool parse_makefile(const char *filename, RuleSet *rs, int *error) {
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "无法打开文件: %s\n", filename);
		return false;
	}
	rs->rule_count = 0;
	int last_rule = -1;
	char buf[MAX_LINE];
	int line_no = 0;
	while (fgets(buf, sizeof(buf), fp)) {
		line_no++;
		buf[strcspn(buf, "\n")] = '\0';
		rstrip_comment(buf);
		// 仅去尾部空白，保留行首 TAB 以识别命令
		int blen = (int)strlen(buf);
		while (blen > 0 && isspace((unsigned char)buf[blen - 1])) buf[--blen] = '\0';
		bool only_ws = true;
		for (int k = 0; buf[k]; ++k) {
			if (!isspace((unsigned char)buf[k])) { only_ws = false; break; }
		}
		if (buf[0] == '\0' || only_ws) continue;

		if (buf[0] == '\t') { // 命令行
			if (last_rule < 0) {
				fprintf(stderr, "Line%d: Command found before rule\n", line_no);
				*error = 1;
				continue;
			}
			Rule *r = &rs->rules[last_rule];
			if (r->cmd_count < MAX_CMDS) {
				const char *cmd = buf + 1; // 去掉一个 TAB
				while (*cmd == '\t' || *cmd == ' ') cmd++;
				if (strlen(cmd) > 0) {
					strncpy(r->cmds[r->cmd_count], cmd, sizeof(r->cmds[0]) - 1);
					r->cmds[r->cmd_count][sizeof(r->cmds[0]) - 1] = '\0';
					r->cmd_count++;
				}
			}
			continue;
		}

		// 规则行在去左空白的视图上识别
		const char *rule_line = buf;
		while (*rule_line == ' ' || *rule_line == '\t') rule_line++;
		if (strchr(rule_line, ':')) {
			if (rs->rule_count >= MAX_RULES) {
				fprintf(stderr, "Line%d: 规则数量超过限制\n", line_no);
				*error = 1;
				continue;
			}
			Rule tmp; memset(&tmp, 0, sizeof(tmp)); tmp.line_no = line_no;
			if (!split_target_deps(rule_line, tmp.target, tmp.deps, &tmp.dep_count)) {
				fprintf(stderr, "Line%d: 目标定义格式错误\n", line_no);
				*error = 1;
				continue;
			}
			if (find_rule_index(rs, tmp.target) >= 0) {
				fprintf(stderr, "Line%d: 目标重复定义 '%s'\n", line_no, tmp.target);
				*error = 1;
				last_rule = -1;
				continue;
			}
			rs->rules[rs->rule_count] = tmp;
			last_rule = rs->rule_count;
			rs->rule_count++;
		} else {
			fprintf(stderr, "Line%d: 无效的行格式\n", line_no);
			*error = 1;
		}
	}
	fclose(fp);
	return true;
}

static void check_dependencies(const RuleSet *rs, int *error) {
	for (int i = 0; i < rs->rule_count; ++i) {
		const Rule *r = &rs->rules[i];
		for (int j = 0; j < r->dep_count; ++j) {
			const char *dep = r->deps[j];
			if (dep[0] == '\0') continue;
			if (find_rule_index(rs, dep) >= 0) continue;
			if (access(dep, F_OK) == 0) continue;
			fprintf(stderr, "Line%d: 无效的依赖 '%s'\n", r->line_no, dep);
			*error = 1;
		}
	}
}

static int run_target(const RuleSet *rs, const char *target, int *error) {
	int idx = -1;
	if (target && target[0]) idx = find_rule_index(rs, target);
	else if (rs->rule_count > 0) idx = 0;
	if (idx < 0) {
		fprintf(stderr, "目标不存在 '%s'\n", target ? target : "");
		*error = 1;
		return 1;
	}
	const Rule *r = &rs->rules[idx];
	for (int i = 0; i < r->cmd_count; ++i) {
		const char *cmd = r->cmds[i];
		printf("%s\n", cmd);
		int ret = system(cmd);
		if (ret == -1) { perror("system"); *error = 1; return 1; }
		if (WIFEXITED(ret)) {
			int code = WEXITSTATUS(ret);
			if (code != 0) { fprintf(stderr, "命令失败，退出码 %d: %s\n", code, cmd); *error = 1; return code; }
		} else if (WIFSIGNALED(ret)) {
			fprintf(stderr, "命令被信号 %d 终止: %s\n", WTERMSIG(ret), cmd);
			*error = 1; return 1;
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {
	const char *file = "Makefile";
	const char *target = NULL; // minimake [target]

	RuleSet *rs = (RuleSet *)malloc(sizeof(RuleSet));
	if (!rs) { fprintf(stderr, "内存分配失败\n"); return 1; }
	memset(rs, 0, sizeof(RuleSet));
	int error = 0;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			printf("用法: %s [target]\n", argv[0]);
			printf("默认读取 ./Makefile；若省略 target，则执行第一个规则。\n");
			free(rs); return 0;
		} else {
			target = argv[i];
			break;
		}
	}

	if (!parse_makefile(file, rs, &error)) { free(rs); return 1; }
	check_dependencies(rs, &error);
	if (!error) run_target(rs, target, &error);
	free(rs);
	return error ? 1 : 0;
}

