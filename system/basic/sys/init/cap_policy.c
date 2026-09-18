#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <procinfo.h>
#include <ewoksys/cap.h>
#include <ewoksys/klog.h>
#include <tinyjson/tinyjson.h>

#include "cap_policy.h"

/* ====================================================================
 * Capability policy (/etc/cap.json).
 *
 * The rules are enforced by the KERNEL, not by process scanning: after
 * parsing, cap_policy_install() pushes every entry into the kernel's
 * exec-time policy (SYS_CAP_POLICY_ADD) and the kernel grants a rule's
 * caps by itself whenever a matching image execs (see kernel cap.c).
 * Process-start order is therefore irrelevant and restarts re-acquire
 * exactly the declared caps. init holds CAP_ROOT (kernel-created),
 * which is what authorizes SYS_CAP_POLICY_ADD.
 * ================================================================== */

#define CAP_RULE_MAX       32
#define CAP_RULE_ENTRY_MAX 16

typedef struct {
    uint32_t type;   /* CAP_FRAME / CAP_IRQ / CAP_DMA */
    uint32_t rights; /* CAP_R|CAP_W|CAP_X|CAP_GRANT */
    uint64_t a;      /* frame: paddr, irq: irq number, dma: block id */
    uint64_t b;      /* frame: size (unused for the others) */
} cap_rule_entry_t;

typedef struct {
    char cmd[PROC_INFO_MAX_CMD_LEN]; /* exe path, matched against the
                                        first token of procinfo.cmd */
    uint32_t num;
    cap_rule_entry_t entries[CAP_RULE_ENTRY_MAX];
} cap_rule_t;

static cap_rule_t _cap_rules[CAP_RULE_MAX];
static uint32_t _cap_rule_num = 0;

static uint32_t cap_parse_rights(json_var_t* v) {
    if(v == NULL)
        return 0;
    if(v->type == JSON_V_INT)
        return ((uint32_t)json_var_get_int(v)) & (CAP_R | CAP_W | CAP_X | CAP_GRANT);
    const char* s = json_var_get_str(v);
    if(s == NULL)
        return 0;
    uint32_t r = 0;
    for(; *s != 0; s++) {
        switch(*s) {
        case 'r': r |= CAP_R;     break;
        case 'w': r |= CAP_W;     break;
        case 'x': r |= CAP_X;     break;
        case 'g': r |= CAP_GRANT; break;
        default:                  break;
        }
    }
    return r;
}

static uint64_t cap_json_u64(json_var_t* obj, const char* name) {
    json_var_t* v = json_get_obj_member(obj, name);
    if(v == NULL)
        return 0;
    if(v->type == JSON_V_STRING) {
        const char* s = json_var_get_str(v);
        return (s != NULL) ? strtoull(s, NULL, 0) : 0;
    }
    if(v->type == JSON_V_INT)
        return (uint32_t)json_var_get_int(v);
    return 0;
}

/* Parse one {"type":...} object into rule entries; an "irq" array expands
   to one entry per irq number. Returns the entry count or -1 on bad input. */
static int32_t cap_parse_entry(json_var_t* v, cap_rule_entry_t* out, uint32_t out_max) {
    const char* type = json_get_str(v, "type");
    if(type == NULL)
        return -1;
    uint32_t rights = cap_parse_rights(json_get_obj_member(v, "rights"));
    if(rights == 0)
        return -1;

    if(strcmp(type, "root") == 0) {
        if(out_max < 1)
            return -1;
        out[0].type = CAP_ROOT;
        out[0].rights = rights;
        out[0].a = 0;
        out[0].b = 0;
        return 1;
    }
    if(strcmp(type, "frame") == 0) {
        if(out_max < 1)
            return -1;
        out[0].type = CAP_FRAME;
        out[0].rights = rights;
        out[0].a = cap_json_u64(v, "paddr");
        out[0].b = cap_json_u64(v, "size");
        return (out[0].b != 0) ? 1 : -1;
    }
    if(strcmp(type, "irq") == 0) {
        json_var_t* irqs = json_get_obj_member(v, "irq");
        if(irqs == NULL)
            return -1;
        if(!irqs->json_is_array) {
            if(out_max < 1)
                return -1;
            out[0].type = CAP_IRQ;
            out[0].rights = rights;
            out[0].a = (uint32_t)json_var_get_int(irqs);
            out[0].b = 0;
            return 1;
        }
        uint32_t n = json_var_array_size(irqs);
        if(n > out_max)
            n = out_max;
        for(uint32_t i = 0; i < n; i++) {
            out[i].type = CAP_IRQ;
            out[i].rights = rights;
            out[i].a = (uint32_t)json_var_get_int(json_var_array_get_var(irqs, i));
            out[i].b = 0;
        }
        return (int32_t)n;
    }
    if(strcmp(type, "dma") == 0) {
        if(out_max < 1)
            return -1;
        out[0].type = CAP_DMA;
        out[0].rights = rights;
        out[0].a = cap_json_u64(v, "block");
        out[0].b = 0;
        return 1;
    }
    return -1;
}

void cap_policy_load(const char* fname) {
    json_var_t* root = json_parse_file(fname);
    if(root == NULL) {
        klog("init: no cap policy '%s'\n", fname);
        return;
    }

    json_var_t* rules = json_get_obj_member(root, "rules");
    uint32_t rn = (rules != NULL) ? json_var_array_size(rules) : 0;
    for(uint32_t i = 0; i < rn && _cap_rule_num < CAP_RULE_MAX; i++) {
        json_var_t* rv = json_var_array_get_var(rules, i);
        const char* cmd = json_get_str(rv, "cmd");
        json_var_t* caps = json_get_obj_member(rv, "caps");
        if(cmd == NULL || cmd[0] == 0 || caps == NULL)
            continue;

        cap_rule_t* rule = &_cap_rules[_cap_rule_num];
        rule->num = 0;
        uint32_t cn = json_var_array_size(caps);
        for(uint32_t c = 0; c < cn; c++) {
            json_var_t* cv = json_var_array_get_var(caps, c);
            int32_t got = cap_parse_entry(cv,
                    &rule->entries[rule->num], CAP_RULE_ENTRY_MAX - rule->num);
            if(got > 0)
                rule->num += (uint32_t)got;
            if(rule->num >= CAP_RULE_ENTRY_MAX)
                break;
        }
        if(rule->num == 0)
            continue;
        strncpy(rule->cmd, cmd, PROC_INFO_MAX_CMD_LEN - 1);
        rule->cmd[PROC_INFO_MAX_CMD_LEN - 1] = 0;
        _cap_rule_num++;
    }
    json_var_unref(root);
    klog("init: cap policy '%s': %u rule(s)\n", fname, _cap_rule_num);
}

static void cap_rights_str(uint32_t rights, char* out) {
    uint32_t i = 0;
    if(rights & CAP_R)     out[i++] = 'r';
    if(rights & CAP_W)     out[i++] = 'w';
    if(rights & CAP_X)     out[i++] = 'x';
    if(rights & CAP_GRANT) out[i++] = 'g';
    out[i] = 0;
}

static void cap_log_entry(const char* cmd, const cap_rule_entry_t* ce) {
    char rights[5];
    cap_rights_str(ce->rights, rights);
    switch(ce->type) {
    case CAP_ROOT:
        klog("init: cap %s: root %s\n", cmd, rights);
        break;
    case CAP_FRAME:
        klog("init: cap %s: frame 0x%llx+0x%llx %s\n", cmd,
                (unsigned long long)ce->a, (unsigned long long)ce->b, rights);
        break;
    case CAP_IRQ:
        klog("init: cap %s: irq %u %s\n", cmd, (uint32_t)ce->a, rights);
        break;
    case CAP_DMA:
        klog("init: cap %s: dma %d %s\n", cmd, (int32_t)ce->a, rights);
        break;
    default:
        klog("init: cap %s: type %u 0x%llx %s\n", cmd,
                ce->type, (unsigned long long)ce->a, rights);
        break;
    }
}

/*
 * Push every parsed rule into the kernel's exec-time policy. From then on
 * the kernel itself grants a rule's caps whenever a matching image execs,
 * so no process-start timing is involved.
 */
void cap_policy_install(void) {
    uint32_t entries = 0;
    for(uint32_t r = 0; r < _cap_rule_num; r++) {
        cap_rule_t* rule = &_cap_rules[r];
        for(uint32_t e = 0; e < rule->num; e++) {
            cap_rule_entry_t* ce = &rule->entries[e];
            if(cap_policy_add(rule->cmd, ce->type, ce->rights, ce->a, ce->b) != 0) {
                klog("init: cap policy install failed: '%s' entry %u\n", rule->cmd, e);
                continue;
            }
            cap_log_entry(rule->cmd, ce);
            entries++;
        }
    }
    if(entries > 0)
        klog("init: cap policy installed (%u entries)\n", entries);
}
