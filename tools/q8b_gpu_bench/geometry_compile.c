#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pglcompiler.h>
#include <geom_raster_cs.h>
#include <adreno/adreno_prog.h>
#include "geometry.h"
#include "adreno_geom_source.h"

static int offset(struct pglc_program *p, const char *name, unsigned base) {
    struct pglc_stage *s=&p->stage[PGLC_STAGE_COMPUTE];
    for (unsigned i=0;i<s->num_uniform_bindings;i++)
        if (!strcmp(s->uniform_bindings[i].name,name))
            return base+s->uniform_bindings[i].dword_offset;
    return -1;
}
static void save(const char *dir,const char *name,const char *ext,void *p,size_t n) {
    char path[512]; snprintf(path,sizeof(path),"%s/geom-%s.%s",dir,name,ext);
    FILE *f=fopen(path,"wb");
    if (!f || fwrite(p,1,n,f)!=n || fclose(f)) exit(1);
}
static void replace(char *s,const char *from,const char *to) {
    char *p=strstr(s,from);
    if (!p) exit(1);
    memmove(p+strlen(to),p+strlen(from),strlen(p+strlen(from))+1);
    memcpy(p,to,strlen(to));
}
int main(int argc,char **argv) {
    if (argc!=2) return 1;
    pglc_set_version(690);
    for (unsigned variant=0;variant<4;variant++) {
        unsigned kind=(variant&1)+1, cached=variant==2 || variant==3;
        const char *names[]={"color","texture","color-cached","texture-cached",
                             "color-16x8","texture-16x8","color-8x8","texture-16x16"};
        struct pglc_program r={0},x={0};
        const char *raster=kind==1?geom_raster_cs_glsl:geom_raster_tex_cs_glsl;
        char *cs=NULL;
        if(variant>=2) {
            const char *body=kind==1?q8b_geom_color_pixel:q8b_geom_texture_pixel;
            cs=malloc(strlen(body)+64);
            sprintf(cs,"#version 310 es\n#define Q8B_TRI_CACHE %u\n%s",cached,strchr(body,'\n')+1);
            raster=cs;
        }
        if (!pglc_compile_compute_raw(raster,&r) ||
            !pglc_compile_compute_raw(kind==1?geom_xform_color_cs_glsl:geom_xform_tex_cs_glsl,&x)) {
            fprintf(stderr,"%s\n%s\n",r.log,x.log); return 1;
        }
        struct pglc_stage *rs=&r.stage[PGLC_STAGE_COMPUTE];
        struct pglc_blob_opts opt={.geom_kind=kind,.geom_tpw=rs->local_size[0]*rs->local_size[1]};
        unsigned bytes=pglc_program_serialize(&r,&x,&opt,NULL,0);
        void *blob=malloc(bytes);
        if (!bytes || !blob || pglc_program_serialize(&r,&x,&opt,blob,bytes)!=bytes) return 1;
        if(variant>=2) ((adreno_prog_blob_t *)blob)->version=ADRENO_PROG_PIXEL_VERSION;
        unsigned split=x.stage[PGLC_STAGE_COMPUTE].uniform_blob_dwords;
        struct geom_meta m={
            .uniforms=split+r.stage[PGLC_STAGE_COMPUTE].uniform_blob_dwords,
            .mvp=offset(&x,"uMVP",0),.viewport_x=offset(&x,"uViewportWH",0),
            .vertices=offset(&x,"uVertCount",0),.viewport_r=offset(&r,"uViewportWH",split),
            .stride=offset(&r,"uFbStride",split),.first=offset(&r,"uTriFirst",split),
            .count=offset(&r,"uTriCount",split),.blend=offset(&r,"uBlendMode",split),
            .depth_write=offset(&r,"uDepthWrite",split),.cull=offset(&r,"uCullSign",split),
            .tex_w=offset(&r,"uTexW",split),.tex_h=offset(&r,"uTexH",split)
        };
        if (m.uniforms>256 || m.mvp<0 || m.viewport_x<0 || m.vertices<0 ||
            m.viewport_r<0 || m.stride<0 || m.first<0 || m.count<0) return 1;
        const char *name=names[variant];
        save(argv[1],name,"bin",blob,bytes); save(argv[1],name,"meta",&m,sizeof(m));
        if (variant==0) {
            /* Legal unused pushed constants make each dispatch fit separately
             * but overflow the combined 4096-word IB. Exercise real hardware
             * fallback without changing the shader's live constant registers. */
            adreno_prog_blob_t *h=blob;
            for (unsigned i=0;i<2;i++) {
                adreno_prog_kernel_t *k=&h->kern[i];
                if(k->num_ubo_ranges>=ADRENO_PROG_MAX_UBO_RANGES || k->constlen>=512) return 1;
                adreno_prog_ubo_range_t *r=&k->ubo_range[k->num_ubo_ranges++];
                r->dst_vec4=k->constlen; r->src_byte=4096;
                r->size_bytes=(512-k->constlen)*16; k->constlen=512;
            }
            save(argv[1],"large","bin",blob,bytes); save(argv[1],"large","meta",&m,sizeof(m));
            /* Restore the ordinary program, then require an unbound raster
             * SSBO. The entire draw must decline before clear or transform. */
            if(pglc_program_serialize(&r,&x,&opt,blob,bytes)!=bytes) return 1;
            h->kern[0].ssbo_binding[0]=7;
            save(argv[1],"invalid","bin",blob,bytes);
        }
        printf("geometry %s: %u bytes, uniforms=%u split=%u\n",name,bytes,m.uniforms,split);
        free(cs); free(blob); pglc_program_free(&r); pglc_program_free(&x);
    }
    return 0;
}
