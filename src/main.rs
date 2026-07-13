use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::env;

fn main() -> std::io::Result<()> {
    let repo_url = "https://github.com/gh0st-8221/driftwm-dotfiles.git";
    let tmp_base = env::temp_dir().join("driftwm-sync");
    
    if tmp_base.exists() { fs::remove_dir_all(&tmp_base)?; }
    fs::create_dir_all(&tmp_base)?;

    let home = env::var("HOME").unwrap();
    let home_path = Path::new(&home);
    
    let configs = vec!["alacritty", "driftwm", "fuzzel", "gtk-4.0", "helix", "ironbar", "HyperHDR"];
    let config_dst = tmp_base.join(".config");
    fs::create_dir_all(&config_dst)?;

    for folder in configs {
        let src = home_path.join(".config").join(folder);
        if src.exists() { copy_dir_all(&src, config_dst.join(folder))?; }
    }

    let qb_theme_path = home_path.join(".config/qBittorrent/catppuccin-mocha.qbtheme");
    if qb_theme_path.exists() {
        let qb_dst = config_dst.join("qBittorrent");
        fs::create_dir_all(&qb_dst)?;
        fs::copy(&qb_theme_path, qb_dst.join("catppuccin-mocha.qbtheme"))?;
    }
    
    for file in vec![".zshrc", ".zprofile"] {
        let src = home_path.join(file);
        if src.exists() { fs::copy(&src, tmp_base.join(file))?; }
    }

    let pkg_output = Command::new("pacman").args(["-Qqe"]).output().expect("failed to get packages");
    let pkgs = String::from_utf8_lossy(&pkg_output.stdout);
    
    let install_script = format!(
        "git clone {} ~/driftwm-dotfiles\n\
        sudo pacman -S --noconfirm {} libdisplay-info libinput seatd mesa libxkbcommon\n\n\
        cp -r ~/driftwm-dotfiles/.config/* ~/.config/\n\
        cp ~/driftwm-dotfiles/.zshrc ~/.zshrc\n\
        cp ~/driftwm-dotfiles/.zprofile ~/.zprofile\n\n\
        chsh -s $(which zsh) $USER\n\
        sudo chsh -s $(which zsh) root\n\n\
        git clone https://github.com/zsh-users/zsh-autosuggestions ~/.zsh/plugins/zsh-autosuggestions\n\
        git clone https://github.com/zsh-users/zsh-syntax-highlighting ~/.zsh/plugins/zsh-syntax-highlighting\n\n\
        systemctl --user enable --now pipewire.service\n\
        systemctl --user enable --now pipewire-pulse.service\n\
        systemctl --user enable --now wireplumber.service", 
        repo_url, pkgs.replace('\n', " ")
    );
    
    fs::write(tmp_base.join("install.sh"), install_script)?;

    run_git(&["init", "-b", "main"], &tmp_base);
    run_git(&["remote", "add", "origin", repo_url], &tmp_base);
    run_git(&["add", "."], &tmp_base);
    run_git(&["commit", "-m", "update from system"], &tmp_base);
    run_git(&["push", "-u", "origin", "main", "--force"], &tmp_base);

    fs::remove_dir_all(&tmp_base)?;
    Ok(())
}

fn run_git(args: &[&str], dir: &PathBuf) {
    Command::new("git").args(args).current_dir(dir).status().ok();
}

fn copy_dir_all(src: impl AsRef<Path>, dst: impl AsRef<Path>) -> std::io::Result<()> {
    fs::create_dir_all(&dst)?;
    for entry in fs::read_dir(src)? {
        let entry = entry?;
        if entry.file_type()?.is_dir() {
            copy_dir_all(entry.path(), dst.as_ref().join(entry.file_name()))?;
        } else {
            fs::copy(entry.path(), dst.as_ref().join(entry.file_name()))?;
        }
    }
    Ok(())
}
