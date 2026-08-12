#include "../include/deploy.h"
#include "../include/build.h"
#include "../include/util.h"
#include "../include/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
    #include <sys/wait.h>
#endif

static int exit_status(int rc) {
#if defined(_WIN32) || !defined(WIFEXITED)
    return rc;
#else
    if (WIFEXITED(rc))   return WEXITSTATUS(rc);
    if (WIFSIGNALED(rc)) return 128 + WTERMSIG(rc);
    return rc;
#endif
}

static int confirm(const char *host, const char *path) {
    char answer[16];

    printf("\nAbout to mirror %s/ to %s:%s\n", g_output_dir, host, path);
    printf("rsync --delete will REMOVE remote files that are not in %s/.\n", g_output_dir);
    printf("Continue? [y/N]: ");
    fflush(stdout);

    if (!fgets(answer, sizeof(answer), stdin)) return 0;
    return answer[0] == 'y' || answer[0] == 'Y';
}

int cmd_deploy(void) {
    const char *host = toml_get(&g_toml, "deploy", "host");
    const char *path = toml_get(&g_toml, "deploy", "path");

    if (!host || !*host || !path || !*path) {
        fprintf(stderr, "Error: [deploy] host and path required in config.toml\n\n");
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "  [deploy]\n");
        fprintf(stderr, "  host = \"user@vps\"\n");
        fprintf(stderr, "  path = \"/var/www/myblog\"\n");
        return 1;
    }

    printf("Building site...\n");
    cmd_build();

    if (!confirm(host, path)) {
        printf("Deploy cancelled.\n");
        return 1;
    }

    /* Config values reach a shell here, so quote them rather than trusting them. */
    char *q_src  = shell_quote(g_output_dir);
    char *q_dest = NULL;
    char  dest[1024];

    snprintf(dest, sizeof(dest), "%s:%s/", host, path);
    q_dest = shell_quote(dest);

    if (!q_src || !q_dest) {
        fprintf(stderr, "Error: out of memory building the rsync command\n");
        free(q_src);
        free(q_dest);
        return 1;
    }

    char cmd[2048];
    int  n = snprintf(cmd, sizeof(cmd), "rsync -avz --delete %s/ %s", q_src, q_dest);
    free(q_src);
    free(q_dest);

    if (n < 0 || n >= (int)sizeof(cmd)) {
        fprintf(stderr, "Error: deploy host/path too long\n");
        return 1;
    }

    printf("\nRunning: %s\n\n", cmd);

    int status = exit_status(system(cmd));
    if (status == 0) {
        printf("\nDeploy complete!\n");
        return 0;
    }

    fprintf(stderr, "\nDeploy failed (rsync exited %d). Check your SSH connection.\n", status);
    return 1;
}
