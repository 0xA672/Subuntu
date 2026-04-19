#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$script_dir"

LOG_FILE="/tmp/subuntu_install_$(date +%Y%m%d_%H%M%S).log"
exec > >(tee -a "$LOG_FILE") 2>&1

log()   { echo "[$(date +'%Y-%m-%d %H:%M:%S')] $*"; }
error() { log "ERROR: $*" >&2; }

log "Starting system provisioning..."

log "Updating package index..."
if ! sudo apt update; then
    error "Failed to update package index"
    exit 1
fi

log "Installing base utilities..."
if ! sudo apt install -y software-properties-common apt-transport-https curl wget; then
    error "Failed to install base utilities"
    exit 1
fi

log "Processing dependency manifest..."
if [ ! -f "./dep.txt" ]; then
    error "Dependency manifest 'dep.txt' not found in $script_dir"
    exit 1
fi

failed_packages=()

clean_pkg_line() {
    tr -d '\r' | sed -e 's/#.*//' -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//'
}

while IFS= read -r line; do
    pkg=$(echo "$line" | clean_pkg_line)
    [ -z "$pkg" ] && continue

    log "Installing: $pkg"
    if ! sudo apt install -y "$pkg"; then
        log "Attempting to fix broken packages..."
        sudo dpkg --configure -a || true
        sudo apt-get --fix-broken install -y || true
        if ! sudo apt install -y "$pkg"; then
            error "Failed to install $pkg after recovery"
            failed_packages+=("$pkg")
        fi
    fi
done < "./dep.txt"

if ! command -v rustc &> /dev/null; then
    log "Installing Rust via rustup..."
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
    log "Rust installed. To activate in current shell, run: source \$HOME/.cargo/env"
fi

if [ ${#failed_packages[@]} -gt 0 ]; then
    log "WARNING: The following packages failed to install:"
    for p in "${failed_packages[@]}"; do
        echo "  - $p" | tee >(cat >&2)
    done
fi

log "Finalizing package configuration..."
sudo dpkg --configure -a || log "Note: dpkg configuration check completed with warnings"
sudo apt autoremove -y
sudo apt clean

log "System provisioning completed."
log "Log file: $LOG_FILE"
log "Please reload your shell with 'source ~/.bashrc' or restart your terminal."

upgradable=$(apt-get -s upgrade 2>/dev/null | grep -c '^Inst' || echo 0)
broken=$(dpkg-query -W -f='${Status}\n' 2>/dev/null | grep -cE 'not-installed|config-files|half-installed' || echo 0)
disk_usage=$(df -h / | awk 'NR==2 {print $5}')

log "Current system status:"
log "  - Packages with available upgrades: $upgradable"
log "  - Packages in broken/incomplete state: $broken"
log "  - Root filesystem usage: $disk_usage"
