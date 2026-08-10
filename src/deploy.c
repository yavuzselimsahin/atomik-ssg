#include "../include/deploy.h"
#include "../include/build.h"
#include "../toml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern TomlDoc g_toml;

void cmd_deploy(void) {
    const char *host = toml_get(&g_toml, "deploy", "host");
    const char *path = toml_get(&g_toml, "deploy", "path");

    if (!host || !path) {
        fprintf(stderr, "Error: [deploy] host and path required in config.toml\n\n");
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "  [deploy]\n");
        fprintf(stderr, "  host = \"user@vps\"\n");
        fprintf(stderr, "  path = \"/var/www/myblog\"\n");
        return;
    }

    /* Önce build al */
    printf("Building site...\n");
    cmd_build();

    /* rsync komutu oluştur */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "rsync -avz --delete public/ %s:%s/",
        host, path);

    printf("\nDeploying to %s:%s\n", host, path);
    printf("Running: %s\n\n", cmd);

    int result = system(cmd);
    if (result == 0)
        printf("\nDeploy complete!\n");
    else
        fprintf(stderr, "\nDeploy failed. Check your SSH connection.\n");
}