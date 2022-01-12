#include "common/xfile.h"
#include "common/ilog2.h"
#include "hope/hope.h"

void test_xfile_hash(void);
void test_xfile_tmp(void);
void test_ilog2(void);
void test_xfile_set_ext(void);
void test_xfile_basename(void);
void test_xfile_strip_ext(void);

int main(void)
{
    test_xfile_hash();
    test_xfile_tmp();
    test_ilog2();
    test_xfile_set_ext();
    test_xfile_basename();
    test_xfile_strip_ext();
    return hope_status();
}

void test_xfile_hash(void)
{
    FILE *fp = fopen(TMPDIR "/file.txt", "wb");
    NOTNULL(fp);
    EQ(fwrite("12345", 5, 1, fp), 1);
    fclose(fp);

    fp = fopen(TMPDIR "/file.txt", "rb");
    NOTNULL(fp);
    uint64_t hash = 0;
    EQ(xfile_hash(fp, &hash), DONE);
    EQ(hash, 14335752410685132726ULL);

    fclose(fp);
    remove(TMPDIR "/file.txt");
}

void test_xfile_tmp(void)
{
    struct xfile_tmp file = {0};
    EQ(xfile_tmp_open(&file), DONE);
    EQ(fwrite("12345", 5, 1, file.fp), 1);
    EQ(fflush(file.fp), 0);
    rewind(file.fp);

    uint64_t hash = 0;
    EQ(xfile_hash(file.fp, &hash), DONE);
    EQ(hash, 14335752410685132726ULL);

    xfile_tmp_del(&file);
}

void test_ilog2(void)
{
    EQ(ilog2(1 << 0), 0);
    EQ(ilog2(1 << 1), 1);
    EQ(ilog2(1 << 2), 2);
    EQ(ilog2(1 << 3), 3);
    EQ(ilog2(1 << 4), 4);
    EQ(ilog2(1 << 5), 5);
    EQ(ilog2(1 << 5), 5);
    EQ(ilog2(1 << 6), 6);
    EQ(ilog2(1 << 7), 7);
    EQ(ilog2(1 << 8), 8);
    EQ(ilog2(1 << 9), 9);
    EQ(ilog2(1 << 10), 10);
    EQ(ilog2(1 << 11), 11);
    EQ(ilog2(1 << 12), 12);
    EQ(ilog2(1 << 13), 13);
    EQ(ilog2(1 << 14), 14);
    EQ(ilog2(1 << 15), 15);
    EQ(ilog2(1 << 16), 16);
    EQ(ilog2(1 << 17), 17);
    EQ(ilog2(1 << 18), 18);
    EQ(ilog2(1 << 19), 19);
    EQ(ilog2(1 << 20), 20);
    EQ(ilog2(1 << 21), 21);
    EQ(ilog2(1 << 22), 22);
    EQ(ilog2(1 << 23), 23);
    EQ(ilog2(1 << 24), 24);
    EQ(ilog2(1 << 25), 25);
    EQ(ilog2(1 << 26), 26);
    EQ(ilog2(1 << 27), 27);
    EQ(ilog2(1 << 28), 28);
    EQ(ilog2(1 << 29), 29);
    EQ(ilog2(1 << 30), 30);
    EQ(ilog2(1ul << 31), 31);
    EQ(ilog2(1ull << 32), 32);
    EQ(ilog2(1ull << 33), 33);
    EQ(ilog2(1ull << 34), 34);
    EQ(ilog2(1ull << 35), 35);
    EQ(ilog2(1ull << 36), 36);
    EQ(ilog2(1ull << 37), 37);
    EQ(ilog2(1ull << 38), 38);
    EQ(ilog2(1ull << 39), 39);
    EQ(ilog2(1ull << 40), 40);
    EQ(ilog2(1ull << 41), 41);
    EQ(ilog2(1ull << 42), 42);
    EQ(ilog2(1ull << 43), 43);
    EQ(ilog2(1ull << 44), 44);
    EQ(ilog2(1ull << 45), 45);
    EQ(ilog2(1ull << 46), 46);
    EQ(ilog2(1ull << 47), 47);
    EQ(ilog2(1ull << 48), 48);
    EQ(ilog2(1ull << 49), 49);
    EQ(ilog2(1ull << 50), 50);
    EQ(ilog2(1ull << 51), 51);
    EQ(ilog2(1ull << 52), 52);
    EQ(ilog2(1ull << 53), 53);
    EQ(ilog2(1ull << 54), 54);
    EQ(ilog2(1ull << 55), 55);
    EQ(ilog2(1ull << 56), 56);
    EQ(ilog2(1ull << 57), 57);
    EQ(ilog2(1ull << 58), 58);
    EQ(ilog2(1ull << 59), 59);
    EQ(ilog2(1ull << 60), 60);
    EQ(ilog2(1ull << 61), 61);
    EQ(ilog2(1ull << 62), 62);
    EQ(ilog2(1ull << 63), 63);
}

void test_xfile_set_ext(void)
{
    char path[4 + 1] = "file";
    char path0[5 + 1 + 4] = "file0";
    char path1[5 + 1 + 4] = "file1.txt";
    char path2[5 + 1 + 2 * 4] = "file2.txt.pdf";

    EQ(xfile_set_ext(sizeof path, path, ".pdf"), ENOMEM);
    EQ(path, "file");

    EQ(xfile_set_ext(sizeof path0, path0, ".pdf"), DONE);
    EQ(path0, "file0.pdf");

    EQ(xfile_set_ext(sizeof path1, path1, ".doc"), DONE);
    EQ(path1, "file1.doc");

    EQ(xfile_set_ext(sizeof path2, path2, ".o"), DONE);
    EQ(path2, "file2.txt.o");
}

void test_xfile_basename(void)
{
    char filename[32] = {0};
    xfile_basename(filename, "/path/to/basename");
    EQ(filename, "basename");
}

void test_xfile_strip_ext(void)
{
    char path0[] = "file";
    char path1[] = "file.txt";
    char path2[] = "file.txt.doc";
    xfile_strip_ext(path0);
    xfile_strip_ext(path1);
    xfile_strip_ext(path2);

    EQ(path0, "file");
    EQ(path1, "file");
    EQ(path2, "file.txt");
}
