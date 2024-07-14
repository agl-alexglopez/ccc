#include "str_view/str_view.h"
#include "test.h"

#ifdef __linux__
#    include <linux/limits.h>
#    define FILESYS_MAX_PATH PATH_MAX
#endif
#ifdef __APPLE__
#    include <sys/syslimits.h>
#    define FILESYS_MAX_PATH NAME_MAX
#endif

#include <dirent.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

struct path_bin
{
    str_view path;
    str_view bin;
};

static str_view const test_prefix = SV("test_");
char const *const pass_msg = "⬤";
char const *const fail_msg = "X";
char const *const err_msg = "Test process failed abnormally:";

static int run(str_view);
enum test_result run_test_process(struct path_bin);
static DIR *open_test_dir(str_view);
static bool fill_path(char *, str_view, str_view);

int
main(int argc, char **argv)
{
    if (argc == 1)
    {
        return 0;
    }
    str_view arg_view = sv(argv[1]);
    return run(arg_view);
}

static int
run(str_view const tests_dir)
{
    DIR *dir_ptr = open_test_dir(tests_dir);
    if (!dir_ptr)
    {
        return 1;
    }
    char absolute_path[FILESYS_MAX_PATH];
    size_t tests = 0;
    size_t passed = 0;
    struct dirent const *d;
    while ((d = readdir(dir_ptr)))
    {
        str_view const entry = sv(d->d_name);
        if (!sv_starts_with(entry, test_prefix))
        {
            continue;
        }
        if (!fill_path(absolute_path, tests_dir, entry))
        {
            return 1;
        }
        printf("%s(%s%s\n", CYAN, sv_begin(entry), NONE);
        (void)fflush(stdout);
        enum test_result const res
            = run_test_process((struct path_bin){sv(absolute_path), entry});
        switch (res)
        {
        case ERROR:
            (void)fprintf(stderr, "%s%s%s %s%s\n", RED, err_msg, CYAN,
                          sv_begin(entry), NONE);
            break;
        case PASS:
            (void)fprintf(stdout, "%s%s%s)%s\n", GREEN, pass_msg, CYAN, NONE);
            break;
        case FAIL:
            (void)fprintf(stdout, "%s%s%s)%s\n", RED, fail_msg, CYAN, NONE);
            break;
        }
        passed += 1 - res;
        ++tests;
    }
    passed == tests
        ? printf("%sPASSED %zu/%zu \\(*.*)/%s\n\n", GREEN, passed, tests, NONE)
        : printf("%sPASSED %zu/%zu T_T%s\n\n", RED, passed, tests, NONE);
    return 0;
}

enum test_result
run_test_process(struct path_bin pb)
{
    if (sv_empty(pb.path))
    {
        (void)fprintf(stderr, "No test provided.\n");
        return ERROR;
    }
    pid_t const test_proc = fork();
    if (test_proc == 0)
    {
        execl(sv_begin(pb.path), sv_begin(pb.bin), NULL);
        (void)fprintf(stderr, "Child test process could not start.\n");
        exit(1);
    }
    int status = 0;
    if (waitpid(test_proc, &status, 0) < 0)
    {
        (void)fprintf(stderr, "Error running test: %s\n", sv_begin(pb.bin));
        return ERROR;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == FAIL)
    {
        return FAIL;
    }
    return PASS;
}

static DIR *
open_test_dir(str_view tests_folder)
{
    if (sv_empty(tests_folder) || sv_len(tests_folder) > FILESYS_MAX_PATH)
    {
        (void)fprintf(stderr, "Invalid input to path to test executables %s\n",
                      sv_begin(tests_folder));
        return NULL;
    }
    DIR *dir_ptr = opendir(sv_begin(tests_folder));
    if (!dir_ptr)
    {
        (void)fprintf(stderr, "Could not open directory %s\n",
                      sv_begin(tests_folder));
        return NULL;
    }
    return dir_ptr;
}

static bool
fill_path(char *path_buf, str_view tests_dir, str_view entry)
{
    size_t const dir_bytes = sv_fill(FILESYS_MAX_PATH, path_buf, tests_dir);
    if (FILESYS_MAX_PATH - dir_bytes < sv_size(entry))
    {
        (void)fprintf(stderr, "Relative path exceeds FILESYS_MAX_PATH?\n%s",
                      path_buf);
        return false;
    }
    (void)sv_fill(FILESYS_MAX_PATH - dir_bytes, path_buf + sv_len(tests_dir),
                  entry);
    return true;
}
