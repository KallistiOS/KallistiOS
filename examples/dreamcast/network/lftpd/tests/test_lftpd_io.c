#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "private/lftpd_io.h"

void test_lftpd_io_canonicalize_path(const char *base, const char *name, const char *expected) {
    char *path = lftpd_io_canonicalize_path(base, name);
    printf("lftpd_io_canonicalize_path(%s, %s) -> %s = %s\n",
            base, name,
            path,
            strcmp(path, expected) == 0 ? "PASS" : "FAIL");
    assert(strcmp(path, expected) == 0);
    free(path);
}

int main() {
    test_lftpd_io_canonicalize_path("/", "name", "/name");
    test_lftpd_io_canonicalize_path("/base", "name", "/base/name");
    test_lftpd_io_canonicalize_path("/base/", "/name", "/name");
    test_lftpd_io_canonicalize_path("/base//", "/name/", "/name");
    test_lftpd_io_canonicalize_path("/base", ".", "/base");
    test_lftpd_io_canonicalize_path("/base/", "..", "/");
    test_lftpd_io_canonicalize_path("/base/base1/base2", "..", "/base/base1");
    test_lftpd_io_canonicalize_path("/base/base1/", "name/name2/..", "/base/base1/name");
    test_lftpd_io_canonicalize_path("/one/./two/../three/four//five/.././././..", "name", "/one/three/name");
    test_lftpd_io_canonicalize_path("/", "/", "/");
    test_lftpd_io_canonicalize_path("/", NULL, "/");
    test_lftpd_io_canonicalize_path(NULL, "/", "/");
    test_lftpd_io_canonicalize_path("/", "", "/");
    test_lftpd_io_canonicalize_path("", "/", "/");
    test_lftpd_io_canonicalize_path("", "", "/");
}
