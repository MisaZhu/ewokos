#ifndef CAP_POLICY_H
#define CAP_POLICY_H

/*
 * Capability policy engine: parses /etc/cap.json and hands the rules to
 * the kernel. See cap_policy.c for the design notes.
 *  - cap_policy_load():    parse the config once.
 *  - cap_policy_install(): push every rule into the kernel's exec-time
 *    policy; from then on the kernel grants the caps by itself whenever a
 *    matching image execs, so process-start order no longer matters.
 */

#define CAP_CONF_FILE "/etc/cap.json"

void cap_policy_load(const char* fname);
void cap_policy_install(void);

#endif
