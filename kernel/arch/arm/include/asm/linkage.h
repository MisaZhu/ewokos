#ifndef __LINKAGE_ARM_H_INCLUDE__
#define __LINKAGE_ARM_H_INCLUDE__

#define __ALIGN .align 0
#define __ALIGN_STR ".align 0"

#define ENDPROC(name) \
  .type name, %function; \
  END(name)

#endif /* __LINKAGE_ARM_H_INCLUDE__ */
