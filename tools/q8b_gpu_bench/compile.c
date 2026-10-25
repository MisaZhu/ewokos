#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <adreno/adreno_prog.h>
#include "pglcompiler.h"
int main(int argc,char **argv) {
    if (argc!=3) return 1;
    FILE *f=fopen(argv[1],"rb");
    if (!f) return 1;
    fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
    char *text=n>0 && n<1024*1024?calloc(n+1,1):NULL;
    if (!text || fread(text,1,n,f)!=(size_t)n) return 1;
    fclose(f);
    struct pglc_program p={0};
    pglc_set_version(690);
    if (!pglc_compile_compute_raw(text,&p)) { fprintf(stderr,"%s\n",p.log);return 1; }
    free(text);
    struct pglc_stage *s=&p.stage[PGLC_STAGE_COMPUTE];
    int found=0;
    for (unsigned i=0;i<s->num_uniform_bindings;i++) {
        struct pglc_uniform_binding *u=&s->uniform_bindings[i];
        if (!strcmp(u->name,"uFrame") && u->dword_offset==0 && u->num_components==4) found=1;
    }
    if (!found || s->uniform_blob_dwords!=4) return 1;
    unsigned bytes=pglc_program_serialize(&p,NULL,NULL,NULL,0);
    void *blob=malloc(bytes);
    if (!blob || pglc_program_serialize(&p,NULL,NULL,blob,bytes)!=bytes) return 1;
    f=fopen(argv[2],"wb");
    if (!f || fwrite(blob,1,bytes,f)!=bytes || fclose(f)) return 1;
    printf("%s: %u bytes\n",argv[2],bytes);
    free(blob);pglc_program_free(&p);
    return 0;
}
