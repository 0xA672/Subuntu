#!/bin/bash

set -e
set -o pipefail

script_dir="$(dirname "$(readlink -f "$0")")"
cd "$script_dir"

LOG_FILE="/tmp/subuntu_install_$(date +%Y%m%d_%H%M%S).log"
exec > >(tee -a "$LOG_FILE") 2>&1

echo "Updating package index..."
if ! sudo apt update; then
    echo "ERROR: Failed to update package index" >&2
    exit 1
fi

echo "Installing software-properties-common and transport utilities..."
if ! sudo apt install -y software-properties-common apt-transport-https curl wget; then
    echo "ERROR: Failed to install base utilities" >&2
    exit 1
fi

echo "Executing custom package deployment..."
if [ -f "./dep.txt" ]; then
    failed_packages=()
    while IFS= read -r package; do
        if [ -n "$package" ] && [[ ! "$package" =~ ^# ]]; then
            echo "Deploying package: $package"
            if ! sudo apt install -y "$package"; then
                echo "Warning: Initial installation failed for $package, attempting recovery..."
                sudo dpkg --configure -a || echo "Warning: dpkg configure had issues"
                sudo apt --fix-broken install -y || echo "Warning: fix-broken had issues"
                if ! sudo apt install -y "$package"; then
                    echo "Critical: Failed to install $package after recovery attempts" >&2
                    failed_packages+=("$package")
                    continue
                fi
            fi
        fi
    done < "./dep.txt"
    
    if [ ${#failed_packages[@]} -gt 0 ]; then
        echo "WARNING: The following packages failed to install:" >&2
        printf '%s\n' "${failed_packages[@]}" >&2
    fi
else
    echo "Error: Dependency manifest 'dep.txt' not found in script directory."
    echo "Expected location: $script_dir/dep.txt"
    echo "Please ensure the dependency file exists in the same directory as this script."
    exit 1
fi

echo "Addressing WSL-specific package configuration issues..."
sudo dpkg --configure -a || echo "Note: Some dpkg configuration issues encountered"

echo "Performing system maintenance tasks..."
sudo apt autoremove -y
sudo apt clean

echo "System provisioning completed successfully."
echo "Log file saved to: $LOG_FILE"
echo "Reload your shell configuration with 'source ~/.bashrc' or restart your terminal session to apply all changes."

echo "Current system status:"
echo "- Package upgrades available: $(apt list --upgradable 2>&1 | wc -l) packages"
echo "- Broken packages: $(dpkg -l 2>/dev/null | grep ^.[RHUFW] | wc -l)"
echo "- Disk space usage: $(df -h / | tail -1 | awk '{print $5}')"