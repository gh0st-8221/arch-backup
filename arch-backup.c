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
        "alacritty", "ghostwm", "rofi", "gtk-4.0", "helix", "dunst", "bottom", "xdg-desktop-portal-termfilechooser", "yazi"
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

    int has_etc_grub = file_exists("/etc/default/grub");
    if (has_etc_grub) {
        char etc_dst[1024];
        snprintf(etc_dst, sizeof(etc_dst), "%s/etc/default", tmp_base);
        create_dirs(etc_dst);
        char grub_dst[1024];
        snprintf(grub_dst, sizeof(grub_dst), "%s/grub", etc_dst);
        char *cp_args[] = {"sudo", "cp", "/etc/default/grub", grub_dst, NULL};
        run_cmd(cp_args);
    }

    const char *grub_theme_src = "/usr/share/grub/themes/catppuccin-mocha-grub-theme";
    if (file_exists(grub_theme_src)) {
        char theme_dst[1024];
        snprintf(theme_dst, sizeof(theme_dst), "%s/usr/share/grub/themes/catppuccin-mocha-grub-theme", tmp_base);
        char parent[1024];
        snprintf(parent, sizeof(parent), "%s", theme_dst);
        char *last_slash = strrchr(parent, '/');
        if (last_slash) *last_slash = 0;
        create_dirs(parent);
        char *cp_args[] = {"sudo", "cp", "-r", (char*)grub_theme_src, theme_dst, NULL};
        run_cmd(cp_args);
    }

    char readme_path[1024];
    snprintf(readme_path, sizeof(readme_path), "%s/README.md", tmp_base);
    FILE *f = fopen(readme_path, "w");
    if (f) {
        fprintf(f, "# GhostWM Dotfiles\n\nAwesome ArchLinux ghostwm helix rice, heavily styled with Catppuccin Mocha everywhere. To install, run:\n\n```bash\ngit clone https://github.com/gh0st-8221/ghostwm-dotfiles\ncd ghostwm-dotfiles\n./install.sh\n```\n\nPackage installation won't work on non-Arch distros, and systemd might fail if you use something cooler like OpenRC or runit.\n");
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

    FILE *fp = popen("pacman -Qqen", "r");
    char pkgs[8192] = "";
    if (fp) {
        char buf[256];
        while (fgets(buf, sizeof(buf), fp)) {
            buf[strcspn(buf, "\n")] = 0;
            strcat(pkgs, buf);
            strcat(pkgs, " ");
        }
        pclose(fp);
    }

    char install_script[16384];
    snprintf(install_script, sizeof(install_script),
        "mkdir -p ~/git\n"
        "git clone %s ~/git/ghostwm-dotfiles\n"
        "git clone https://github.com/gh0st-8221/ghostwm ~/git/ghostwm\n"
        "git clone https://aur.archlinux.org/xdg-desktop-portal-termfilechooser-git.git ~/git/xdg-desktop-portal-termfilechooser-git\n"
        "sudo pacman -Syu --noconfirm %s libdisplay-info libinput seatd mesa libxkbcommon\n\n"
        "cd ~/git/~/git/xdg-desktop-portal-termfilechooser-git\n"
        "makepkg -si\n"
        "cd ~/git/\n"
        "make\n"
        "if [ -d ~/git/ghostwm-dotfiles/usr/share/grub/themes/catppuccin-mocha-grub-theme ]; then\n"
        "    [ -d /usr/share/grub/themes/catppuccin-mocha-grub-theme ] && sudo cp -r /usr/share/grub/themes/catppuccin-mocha-grub-theme /usr/share/grub/themes/catppuccin-mocha-grub-theme.bak\n"
        "    sudo cp -r ~/git/ghostwm-dotfiles/usr/share/grub/themes/catppuccin-mocha-grub-theme /usr/share/grub/themes/\n"
        "fi\n\n"
        "if [ -f ~/git/ghostwm-dotfiles/etc/default/grub ]; then\n"
        "    [ -f /etc/default/grub ] && sudo cp /etc/default/grub /etc/default/grub.bak\n"
        "    sudo cp ~/git/ghostwm-dotfiles/etc/default/grub /etc/default/grub\n"
        "fi\n\n"
        "sudo grub-mkconfig -o /boot/grub/grub.cfg\n\n"
        "mkdir -p ~/.config\n"
        "[ -f ~/.config/xdg-desktop-portal-termfilechooser/config ] && cp ~/.config/xdg-desktop-portal-termfilechooser/config ~/.config/xdg-desktop-portal-termfilechooser/config.bak\n"
        "[ -f ~/.config/yazi/yazi-wrapper.sh ] && cp ~/.config/yazi/yazi-wrapper.sh ~/.config/yazi/yazi-wrapper.sh.bak\n"
        "cp -r ~/git/ghostwm-dotfiles/.config/. ~/.config/\n"
        "chmod +x ~/.config/yazi/yazi-wrapper.sh\n"
        "[ -f ~/.zshrc ] && cp ~/.zshrc ~/.zshrc.bak\n"
        "cp ~/git/ghostwm-dotfiles/.zshrc ~/.zshrc\n"
        "[ -f ~/.zprofile ] && cp ~/.zprofile ~/.zprofile.bak\n"
        "cp ~/git/ghostwm-dotfiles/.zprofile ~/.zprofile\n"
        "if [ -f ~/git/ghostwm-dotfiles/.xinitrc ]; then\n"
        "    [ -f ~/.xinitrc ] && cp ~/.xinitrc ~/.xinitrc.bak\n"
        "    cp ~/git/ghostwm-dotfiles/.xinitrc ~/.xinitrc\n"
        "fi\n\n"
        "chsh -s $(which zsh) $USER\n"
        "sudo chsh -s $(which zsh) root\n\n"
        "if [ -d ~/.zsh/plugins/zsh-autosuggestions ]; then\n"
        "    cp -r ~/.zsh/plugins/zsh-autosuggestions ~/.zsh/plugins/zsh-autosuggestions.bak\n"
        "    rm -rf ~/.zsh/plugins/zsh-autosuggestions\n"
        "fi\n"
        "git clone https://github.com/zsh-users/zsh-autosuggestions ~/.zsh/plugins/zsh-autosuggestions\n\n"
        "if [ -d ~/.zsh/plugins/zsh-syntax-highlighting ]; then\n"
        "    cp -r ~/.zsh/plugins/zsh-syntax-highlighting ~/.zsh/plugins/zsh-syntax-highlighting.bak\n"
        "    rm -rf ~/.zsh/plugins/zsh-syntax-highlighting\n"
        "fi\n"
        "git clone https://github.com/zsh-users/zsh-syntax-highlighting ~/.zsh/plugins/zsh-syntax-highlighting\n\n"
        "systemctl --user enable --now pipewire.service\n"
        "systemctl --user enable --now pipewire-pulse.service\n"
        "systemctl --user enable --now wireplumber.service",
        repo_url, pkgs
    );

    char install_path[1024];
    snprintf(install_path, sizeof(install_path), "%s/install.sh", tmp_base);
    FILE *f_inst = fopen(install_path, "w");
    if (f_inst) {
        fprintf(f_inst, "%s", install_script);
        fclose(f_inst);
        chmod(install_path, 0755);
    }

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
