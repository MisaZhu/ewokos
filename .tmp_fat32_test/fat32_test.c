/* host-side functional test for the fat32 library against a real image */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fat32/fat32fs.h>

static FILE* _img = NULL;

static int32_t img_read_sector(int32_t sector, void* buf) {
    if(fseek(_img, (long)sector * FAT32_SECTOR_SIZE, SEEK_SET) != 0)
        return -1;
    return fread(buf, FAT32_SECTOR_SIZE, 1, _img) == 1 ? 0 : -1;
}

static int32_t img_read_sectors(int32_t sector, void* buf, uint32_t count) {
    if(fseek(_img, (long)sector * FAT32_SECTOR_SIZE, SEEK_SET) != 0)
        return -1;
    return fread(buf, FAT32_SECTOR_SIZE, count, _img) == count ? 0 : -1;
}

static int32_t img_write_sector(int32_t sector, const void* buf) {
    if(fseek(_img, (long)sector * FAT32_SECTOR_SIZE, SEEK_SET) != 0)
        return -1;
    if(fwrite(buf, FAT32_SECTOR_SIZE, 1, _img) != 1)
        return -1;
    fflush(_img);
    return 0;
}

static int _failed = 0;
#define CHECK(cond, msg) do { \
    if(cond) printf("PASS: %s\n", msg); \
    else { printf("FAIL: %s\n", msg); _failed++; } \
} while(0)

static void list_dir(fat32_t* fat, fat32_node_t* dir, int depth) {
    fat32_dir_t it;
    fat32_node_t kid;
    if(fat32_diropen(fat, dir, &it) != 0)
        return;
    while(fat32_dirnext(fat, &it, &kid) == 1) {
        printf("%*s%s%s (%u bytes, clus %u)\n", depth * 2, "",
            kid.name, (kid.attr & FAT32_ATTR_DIRECTORY) ? "/" : "", kid.size, kid.start_cluster);
        if(kid.attr & FAT32_ATTR_DIRECTORY)
            list_dir(fat, &kid, depth + 1);
    }
    fat32_dirclose(&it);
}

int main(int argc, char** argv) {
    if(argc < 2) {
        printf("usage: %s <image> [phase2]\n", argv[0]);
        return 1;
    }
    _img = fopen(argv[1], "r+b");
    if(_img == NULL) {
        printf("cannot open %s\n", argv[1]);
        return 1;
    }

    fat32_t fat;
    CHECK(fat32_init_ex(&fat, img_read_sector, img_read_sectors, img_write_sector) == 0, "fat32_init");

    printf("--- tree ---\n");
    fat32_node_t root;
    fat32_root_node(&fat, &root);
    list_dir(&fat, &root, 0);
    printf("------------\n");

    /* read checks */
    int32_t size = 0;
    char* data = fat32_readfile(&fat, "/hello.txt", &size);
    CHECK(data != NULL && size == 12 && memcmp(data, "hello fat32\n", 12) == 0, "read /hello.txt");
    free(data);

    data = fat32_readfile(&fat, "/subdir/nested/deep.txt", &size);
    CHECK(data != NULL && size == 20 && memcmp(data, "nested content here\n", 20) == 0, "read nested path");
    free(data);

    data = fat32_readfile(&fat, "/bigfile.bin", &size);
    int ok = data != NULL && size == 100000;
    if(ok) {
        for(int i = 0; i < 100000; i++) {
            if(data[i] != 'A') { ok = 0; break; }
        }
    }
    CHECK(ok, "read 100000-byte multi-cluster file");
    free(data);

    data = fat32_readfile(&fat, "/A Long File Name With Spaces.text", &size);
    CHECK(data != NULL && size == 15, "read LFN with spaces");
    free(data);

    data = fat32_readfile(&fat, "/\xe4\xb8\xad\xe6\x96\x87\xe6\x96\x87\xe4\xbb\xb6\xe5\x90\x8d.txt", &size);
    CHECK(data != NULL && size == 8, "read utf8(chinese) LFN");
    free(data);

    fat32_node_t node;
    CHECK(fat32_node_by_fname(&fat, "/MIXEDCASE.TXT", &node) == 0, "case-insensitive lookup");
    CHECK(fat32_node_by_fname(&fat, "/no_such_file", &node) != 0, "missing file rejected");

    /* offset read on the big file */
    CHECK(fat32_node_by_fname(&fat, "/bigfile.bin", &node) == 0, "lookup bigfile");
    char part[100];
    CHECK(fat32_read(&fat, &node, part, 100, 99950) == 50, "tail read clamps at EOF");

    if(argc > 2 && strcmp(argv[2], "phase2") == 0) {
        /* write checks: create nested dirs/files like the ext2 regression */
        CHECK(fat32_node_by_fname(&fat, "/", &root) == 0, "root lookup");

        fat32_node_t d1, d2, f1;
        CHECK(fat32_create_dir(&fat, &root, "newdir", &d1) == 0, "create /newdir");
        CHECK(fat32_create_dir(&fat, &d1, "level2", &d2) == 0, "create /newdir/level2");
        CHECK(fat32_create_file(&fat, &d2, "created inside nested dir.txt", &f1) == 0, "create nested LFN file");

        const char* payload = "written by fat32 lib";
        CHECK(fat32_write(&fat, &f1, payload, 20, 0) == 20, "write file data");
        CHECK(fat32_update_node(&fat, &f1) == 0, "persist dirent metadata");

        /* overwrite + extend across cluster boundary */
        fat32_node_t big;
        CHECK(fat32_create_file(&fat, &root, "big2.bin", &big) == 0, "create big2.bin");
        char* wbuf = malloc(70000);
        for(int i = 0; i < 70000; i++) wbuf[i] = (char)(i & 0xff);
        CHECK(fat32_write(&fat, &big, wbuf, 70000, 0) == 70000, "write 70000 bytes");
        CHECK(fat32_update_node(&fat, &big) == 0, "persist big2 metadata");

        char* rbuf = malloc(70000);
        CHECK(fat32_node_by_fname(&fat, "/big2.bin", &big) == 0 &&
            big.size == 70000 &&
            fat32_read(&fat, &big, rbuf, 70000, 0) == 70000 &&
            memcmp(wbuf, rbuf, 70000) == 0, "read-back 70000 bytes verbatim");
        free(wbuf);
        free(rbuf);

        /* sparse-extend write at offset */
        fat32_node_t sp;
        CHECK(fat32_create_file(&fat, &root, "sparse.bin", &sp) == 0, "create sparse.bin");
        CHECK(fat32_write(&fat, &sp, "XY", 2, 5000) == 2, "write at offset 5000");
        CHECK(fat32_update_node(&fat, &sp) == 0, "persist sparse metadata");
        char two[2];
        CHECK(fat32_node_by_fname(&fat, "/sparse.bin", &sp) == 0 && sp.size == 5002 &&
            fat32_read(&fat, &sp, two, 2, 5000) == 2 && two[0] == 'X' && two[1] == 'Y', "sparse read-back");

        /* truncate */
        fat32_node_t h;
        CHECK(fat32_create_file(&fat, &root, "trunc.txt", &h) == 0, "create trunc.txt");
        CHECK(fat32_write(&fat, &h, "0123456789", 10, 0) == 10, "fill trunc.txt");
        CHECK(fat32_update_node(&fat, &h) == 0, "persist trunc.txt");
        CHECK(fat32_truncate(&fat, &h) == 0, "truncate");
        CHECK(fat32_node_by_fname(&fat, "/trunc.txt", &h) == 0 && h.size == 0 && h.start_cluster == 0, "truncated on disk");

        /* unlink / rmdir semantics */
        fat32_node_t rm;
        CHECK(fat32_create_file(&fat, &root, "removeme.txt", &rm) == 0, "create removeme.txt");
        CHECK(fat32_unlink(&fat, "/removeme.txt") == 0, "unlink file");
        CHECK(fat32_node_by_fname(&fat, "/removeme.txt", &rm) != 0, "unlinked file gone");
        CHECK(fat32_rmdir(&fat, "/newdir") != 0, "rmdir refuses non-empty dir");
        fat32_node_t e1;
        CHECK(fat32_create_dir(&fat, &root, "emptydir", &e1) == 0, "create emptydir");
        CHECK(fat32_rmdir(&fat, "/emptydir") == 0, "rmdir empty dir");
        CHECK(fat32_node_by_fname(&fat, "/emptydir", &e1) != 0, "removed dir gone");

        /* duplicated create must fail */
        fat32_node_t dup;
        CHECK(fat32_create_file(&fat, &root, "hello.txt", &dup) != 0, "duplicate create rejected");

        /* many files in one dir forces directory cluster extension */
        fat32_node_t md;
        CHECK(fat32_create_dir(&fat, &root, "manydir", &md) == 0, "create manydir");
        int created = 0;
        for(int i = 0; i < 40; i++) {
            char nm[64];
            fat32_node_t t;
            snprintf(nm, sizeof(nm), "some quite long file name %02d.dat", i);
            if(fat32_create_file(&fat, &md, nm, &t) == 0)
                created++;
        }
        CHECK(created == 40, "40 LFN files in one dir (dir cluster extension)");

        printf("--- tree after writes ---\n");
        fat32_root_node(&fat, &root);
        list_dir(&fat, &root, 0);
    }

    fat32_quit(&fat);
    fclose(_img);
    printf(_failed == 0 ? "ALL-PASS\n" : "FAILED=%d\n", _failed);
    return _failed;
}
