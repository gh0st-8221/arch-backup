use std::env;
use std::fs;
use std::io::{Error, ErrorKind};
use std::path::{Path, PathBuf};
use std::process::Command;

fn main() -> std::io::Result<()> {
    let repo_url = "https://github.com/gh0st-8221/driftwm-dotfiles.git";
    let tmp_base = env::temp_dir().join("driftwm-sync");

    if tmp_base.exists() {
        Command::new("sudo")
            .args(["rm", "-rf", tmp_base.to_str().unwrap()])
            .status()?;
    }
    fs::create_dir_all(&tmp_base)?;

    let home = env::var("HOME").map_err(|_| Error::new(ErrorKind::NotFound, "HOME environment variable not set"))?;
    let home_path = Path::new(&home);

    let configs = [
        "alacritty", "driftwm", "fuzzel", "gtk-4.0", "helix", "ironbar", "mako", "bottom",
    ];
    let config_dst = tmp_base.join(".config");
    fs::create_dir_all(&config_dst)?;

    for folder in configs {
        let src = home_path.join(".config").join(folder);
        if src.exists() {
            Command::new("sudo")
                .args(["cp", "-a", src.to_str().unwrap(), config_dst.to_str().unwrap()])
                .status()?;
        }
    }

    let qb_theme_path = home_path.join(".config/qBittorrent/catppuccin-mocha.qbtheme");
    if qb_theme_path.exists() {
        let qb_dst = config_dst.join("qBittorrent");
        fs::create_dir_all(&qb_dst)?;
        Command::new("sudo")
            .args(["cp", "-a", qb_theme_path.to_str().unwrap(), qb_dst.to_str().unwrap()])
            .status()?;
    }

    let openrgb_dir = home_path.join(".config/OpenRGB");
    let openrgb_dst = config_dst.join("OpenRGB");
    for file_name in ["ghost.orp", "OpenRGB.json"] {
        let file_path = openrgb_dir.join(file_name);
        if file_path.exists() {
            fs::create_dir_all(&openrgb_dst)?;
            Command::new("sudo")
                .args(["cp", "-a", file_path.to_str().unwrap(), openrgb_dst.to_str().unwrap()])
                .status()?;
        }
    }

    for file in [".zshrc", ".zprofile"] {
        let src = home_path.join(file);
        if src.exists() {
            Command::new("sudo")
                .args(["cp", "-a", src.to_str().unwrap(), tmp_base.join(file).to_str().unwrap()])
                .status()?;
        }
    }

    let has_etc_grub = Command::new("sudo")
        .args(["test", "-f", "/etc/default/grub"])
        .status()
        .map(|s| s.success())
        .unwrap_or(false);

    if has_etc_grub {
        let etc_dst = tmp_base.join("etc/default");
        fs::create_dir_all(&etc_dst)?;
        Command::new("sudo")
            .args(["cp", "/etc/default/grub", etc_dst.join("grub").to_str().unwrap()])
            .status()?;
    }

    let grub_theme_src = Path::new("/usr/share/grub/themes/catppuccin-mocha-grub-theme");
    if grub_theme_src.exists() {
        let theme_dst = tmp_base.join("usr/share/grub/themes/catppuccin-mocha-grub-theme");
        if let Some(parent) = theme_dst.parent() {
            fs::create_dir_all(parent)?;
        }
        Command::new("sudo")
            .args(["cp", "-r", grub_theme_src.to_str().unwrap(), theme_dst.to_str().unwrap()])
            .status()?;
    }

    let readme_content = "# DriftWM Dotfiles\n\n\
        Awesome ArchLinux driftwm ironbar helix rice, heavily styled with Catppuccin Mocha everywhere. To install, run:\n\n\
        ```bash\n\
        git clone https://github.com/gh0st-8221/driftwm-dotfiles\n\
        cd driftwm-dotfiles\n\
        chmod +x ./install.sh\n\
        ./install.sh\n\
        ```\n\n\
        Package installation won't work on non-Arch distros, and systemd might fail if you use something cooler like OpenRC or runit.\n";
    fs::write(tmp_base.join("README.md"), readme_content)?;

    let user = env::var("USER").unwrap_or_else(|_| "ghost".to_string());
    Command::new("sudo")
        .args(["chown", "-R", &format!("{}:{}", user, user), tmp_base.to_str().unwrap()])
        .status()?;

    Command::new("chmod")
        .args(["-R", "u+rw", tmp_base.to_str().unwrap()])
        .status()?;

    let pkg_output = Command::new("pacman")
        .args(["-Qqen"])
        .output()?;
    let pkgs = String::from_utf8_lossy(&pkg_output.stdout);

    let install_script = format!(
        "mkdir -p ~/git\n\
        git clone {} ~/git/driftwm-dotfiles\n\
        git clone https://github.com/malbiruk/driftwm ~/git/driftwm\n\n\
        sudo pacman -Syu --noconfirm {} libdisplay-info libinput seatd mesa libxkbcommon\n\n\
        cd ~/git/driftwm\n\
        make build\n\
        sudo make install\n\n\
        if [ -d ~/git/driftwm-dotfiles/usr/share/grub/themes/catppuccin-mocha-grub-theme ]; then\n\
            sudo cp -r ~/git/driftwm-dotfiles/usr/share/grub/themes/catppuccin-mocha-grub-theme /usr/share/grub/themes/\n\
        fi\n\n\
        if [ -f ~/git/driftwm-dotfiles/etc/default/grub ]; then\n\
            sudo cp ~/git/driftwm-dotfiles/etc/default/grub /etc/default/grub\n\
        fi\n\n\
        sudo grub-mkconfig -o /boot/grub/grub.cfg\n\n\
        mkdir -p ~/.config\n\
        cp -r ~/git/driftwm-dotfiles/.config/. ~/.config/\n\
        cp ~/git/driftwm-dotfiles/.zshrc ~/.zshrc\n\
        cp ~/git/driftwm-dotfiles/.zprofile ~/.zprofile\n\n\
        chsh -s $(which zsh) $USER\n\
        sudo chsh -s $(which zsh) root\n\n\
        git clone https://github.com/zsh-users/zsh-autosuggestions ~/.zsh/plugins/zsh-autosuggestions\n\
        git clone https://github.com/zsh-users/zsh-syntax-highlighting ~/.zsh/plugins/zsh-syntax-highlighting\n\n\
        systemctl --user enable --now pipewire.service\n\
        systemctl --user enable --now pipewire-pulse.service\n\
        systemctl --user enable --now wireplumber.service",
        repo_url,
        pkgs.replace('\n', " ")
    );

    fs::write(tmp_base.join("install.sh"), install_script)?;

    run_git(&["init", "-b", "main"], &tmp_base);
    run_git(&["remote", "add", "origin", repo_url], &tmp_base);
    run_git(&["add", "-A"], &tmp_base);
    run_git(&["commit", "-m", "update from system"], &tmp_base);
    run_git(&["push", "-u", "origin", "main", "--force"], &tmp_base);

    Command::new("sudo")
        .args(["rm", "-rf", tmp_base.to_str().unwrap()])
        .status()?;

    Ok(())
}

fn run_git(args: &[&str], dir: &PathBuf) {
    Command::new("git").args(args).current_dir(dir).status().ok();
}
