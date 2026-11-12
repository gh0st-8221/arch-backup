#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

void run_cmd(char *const argv[]) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    }
}

void run_cmd_dir(char *const argv[], const char *dir) {
    pid_t pid = fork();
    if (pid == 0) {
        chdir(dir);
        execvp(argv[0], argv);
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    }
}

int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

void create_dirs(const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

void dump_list(const char *cmd, const char *dir, const char *name) {
    char path[1024], line[256];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *in = popen(cmd, "r");
    if (!in) return;
    FILE *out = fopen(path, "w");
    if (out) {
        while (fgets(line, sizeof(line), in)) fputs(line, out);
        fclose(out);
    }
    pclose(in);
}

int main() {
    const char *repo_url = "https://github.com/gh0st-8221/arch-dotfiles.git";
    
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
        else home = "/root";
    }

    char tmp_base[1024];
    snprintf(tmp_base, sizeof(tmp_base), "/tmp/ghostwm-sync");

    if (file_exists(tmp_base)) {
        char *rm_args[] = {"sudo", "rm", "-rf", tmp_base, NULL};
        run_cmd(rm_args);
    }
    create_dirs(tmp_base);

    const char *configs[] = {
        "alacritty", "ghostwm", "rofi", "gtk-4.0", "helix", "dunst", "bottom", "xdg-desktop-portal", "xdg-desktop-portal-termfilechooser", "yazi",
    };

    char config_dst[1024];
    snprintf(config_dst, sizeof(config_dst), "%s/.config", tmp_base);
    create_dirs(config_dst);

    size_t num_configs = sizeof(configs) / sizeof(configs[0]);
    for (size_t i = 0; i < num_configs; i++) {
        char src[1024];
        snprintf(src, sizeof(src), "%s/.config/%s", home, configs[i]);
        if (file_exists(src)) {
            char *cp_args[] = {"sudo", "cp", "-a", src, config_dst, NULL};
            run_cmd(cp_args);
        }
    }

    char qb_theme_path[1024];
    snprintf(qb_theme_path, sizeof(qb_theme_path), "%s/.config/qBittorrent/catppuccin-mocha.qbtheme", home);
    if (file_exists(qb_theme_path)) {
        char qb_dst[1024];
        snprintf(qb_dst, sizeof(qb_dst), "%s/qBittorrent", config_dst);
        create_dirs(qb_dst);
        char *cp_args[] = {"sudo", "cp", "-a", qb_theme_path, qb_dst, NULL};
        run_cmd(cp_args);
    }

    char openrgb_dir[1024];
    snprintf(openrgb_dir, sizeof(openrgb_dir), "%s/.config/OpenRGB", home);
    char openrgb_dst[1024];
    snprintf(openrgb_dst, sizeof(openrgb_dst), "%s/OpenRGB", config_dst);
    const char *openrgb_files[] = {"ghost.orp", "OpenRGB.json"};
    for (int i = 0; i < 2; i++) {
        char file_path[1024];
        snprintf(file_path, sizeof(file_path), "%s/%s", openrgb_dir, openrgb_files[i]);
        if (file_exists(file_path)) {
            create_dirs(openrgb_dst);
            char *cp_args[] = {"sudo", "cp", "-a", file_path, openrgb_dst, NULL};
            run_cmd(cp_args);
        }
    }

    const char *files[] = {".zshrc", ".zprofile", ".xinitrc"};
    for (int i = 0; i < 3; i++) {
        char src[1024];
        snprintf(src, sizeof(src), "%s/%s", home, files[i]);
        if (file_exists(src)) {
            char dst[1024];
            snprintf(dst, sizeof(dst), "%s/%s", tmp_base, files[i]);
            char *cp_args[] = {"sudo", "cp", "-a", src, dst, NULL};
            run_cmd(cp_args);
        }
    }

    const char *chromium_policy_src = "/etc/chromium/policies/managed/catppuccin.json";
    if (file_exists(chromium_policy_src)) {
        char policy_dst_dir[1024];
        snprintf(policy_dst_dir, sizeof(policy_dst_dir), "%s/etc/chromium/policies/managed", tmp_base);
        create_dirs(policy_dst_dir);
        char *cp_args[] = {"sudo", "cp", "-a", (char*)chromium_policy_src, policy_dst_dir, NULL};
        run_cmd(cp_args);
    }

    char readme_path[1024];
    snprintf(readme_path, sizeof(readme_path), "%s/README.md", tmp_base);
    FILE *f = fopen(readme_path, "w");
    if (f) {
        fprintf(f, "# GhostWM Dotfiles\n\nAwesome ArchLinux ghostwm helix rice, heavily styled with Catppuccin Mocha everywhere.\n\nInstall with ghostosinstall: boot the GhostOS installer ISO and pick the install item. Package lists are in packages/.\n");
        fclose(f);
    }

    const char *user = getenv("USER");
    if (!user) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) user = pw->pw_name;
        else user = "ghost";
    }

    char user_group[128];
    snprintf(user_group, sizeof(user_group), "%s:%s", user, user);
    char *chown_args[] = {"sudo", "chown", "-R", user_group, tmp_base, NULL};
    run_cmd(chown_args);

    char *chmod_args[] = {"chmod", "-R", "u+rw", tmp_base, NULL};
    run_cmd(chmod_args);

    char pkg_dir[1024];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/packages", tmp_base);
    create_dirs(pkg_dir);
    dump_list("pacman -Qqen", pkg_dir, "native.txt");
    dump_list("pacman -Qqem", pkg_dir, "foreign.txt");

    char *git_init[] = {"git", "init", "-b", "main", NULL};
    char *git_remote[] = {"git", "remote", "add", "origin", (char*)repo_url, NULL};
    char *git_add[] = {"git", "add", "-A", NULL};
    char *git_commit[] = {"git", "commit", "-m", "update from system", NULL};
    char *git_push[] = {"git", "push", "-u", "origin", "main", "--force", NULL};

    run_cmd_dir(git_init, tmp_base);
    run_cmd_dir(git_remote, tmp_base);
    run_cmd_dir(git_add, tmp_base);
    run_cmd_dir(git_commit, tmp_base);
    run_cmd_dir(git_push, tmp_base);

    char *rm_final[] = {"sudo", "rm", "-rf", tmp_base, NULL};
    run_cmd(rm_final);

    return 0;
}

#pragma GCC diagnostic pop
