#include "checkers.h"
#include "str_view.h"

#include <string.h>

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

static enum check_result run(str_view);
static enum check_result run_test_process(struct path_bin);
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
    CHECK_RUN(run(arg_view));
}

CHECK_BEGIN_STATIC_FN(run, str_view const tests_dir)
{
    DIR *dir_ptr = open_test_dir(tests_dir);
    CHECK(dir_ptr != NULL, true);
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
        CHECK(fill_path(absolute_path, tests_dir, entry), true);
        printf("%s(%s%s", CYAN, sv_begin(entry), NONE);
        (void)fflush(stdout);
        enum check_result const res
            = run_test_process((struct path_bin){sv(absolute_path), entry});
        switch (res)
        {
        case ERROR:
            (void)fprintf(stderr, "\n%s%s%s %s %s%s%s)%s\n", RED, err_msg, CYAN,
                          sv_begin(entry), RED, fail_msg, CYAN, NONE);
            break;
        case PASS:
            (void)fprintf(stdout, " %s%s%s)%s\n", GREEN, pass_msg, CYAN, NONE);
            break;
        case FAIL:
            (void)fprintf(stdout, "\n%s%s%s)%s\n", RED, fail_msg, CYAN, NONE);
            break;
        }
        if (res == PASS)
        {
            ++passed;
        }
        ++tests;
    }
    CHECK(passed, tests);
    CHECK_END_FN(closedir(dir_ptr););
}

CHECK_BEGIN_STATIC_FN(run_test_process, struct path_bin pb)
{
    CHECK_ERROR(sv_empty(pb.path), false,
                { (void)fprintf(stderr, "No test provided.\n"); });
    pid_t const test_proc = fork();
    if (test_proc == 0)
    {
        CHECK_ERROR(
            execl(sv_begin(pb.path), sv_begin(pb.bin), NULL) >= 0, true, {
                (void)fprintf(stderr, "Child test process could not start.\n");
            });
    }
    int status = 0;
    CHECK_ERROR(waitpid(test_proc, &status, 0) >= 0, true, {
        (void)fprintf(stderr, "Error running test: %s\n", sv_begin(pb.bin));
    });
    CHECK_ERROR(WIFSIGNALED(status), false, {
        int const sig = WTERMSIG(status);
        char const *const msg = strsignal(sig);
        if (msg)
        {
            (void)fprintf(stderr, "%sProcess terminated with signal %d: %s%s\n",
                          RED, sig, msg, NONE);
        }
        else
        {
            (void)fprintf(
                stderr,
                "%sProcess terminated with signal %d: unknown signal code%s\n",
                RED, sig, NONE);
        }
    });
    CHECK(WIFEXITED(status), true);
    CHECK_STATUS = WEXITSTATUS(status);
    CHECK_END_FN();
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
