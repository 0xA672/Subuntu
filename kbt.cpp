#include "package.hpp"
#include "json.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>

using json = nlohmann::json;

const char* embedded_package_json = R"({
  "C++": {
    "apt": ["build-essential", "cmake", "gdb"],
    "yum": ["gcc-c++", "cmake", "gdb"],
    "brew": ["gcc", "cmake"],
    "choco": ["cmake", "ninja", "visualstudio2022buildtools"]
  },
  "Rust": {
    "apt": ["rustc", "cargo"],
    "brew": ["rust"],
    "choco": ["rust", "rustup.install"]
  },
  "Go": {
    "apt": ["golang"],
    "yum": ["golang"],
    "brew": ["go"],
    "choco": ["golang"]
  },
  "Java": {
    "apt": ["openjdk-17-jdk"],
    "yum": ["java-17-openjdk-devel"],
    "brew": ["openjdk"],
    "choco": ["openjdk"]
  },
  "C#": {
    "apt": ["mono-complete"],
    "yum": ["mono-complete"],
    "brew": ["mono"],
    "choco": ["dotnet-sdk"]
  },
  "Python": {
    "apt": ["python3", "python3-pip"],
    "yum": ["python3", "python3-pip"],
    "brew": ["python"],
    "choco": ["python"]
  },
  "Node.js": {
    "apt": ["nodejs", "npm"],
    "yum": ["nodejs", "npm"],
    "brew": ["node"],
    "choco": ["nodejs"]
  },
  "Ruby": {
    "apt": ["ruby", "ruby-dev"],
    "yum": ["ruby", "ruby-devel"],
    "brew": ["ruby"],
    "choco": ["ruby"]
  },
  "PHP": {
    "apt": ["php", "php-cli", "php-mbstring"],
    "yum": ["php", "php-cli"],
    "brew": ["php"],
    "choco": ["php"]
  },
  "Perl": {
    "apt": ["perl"],
    "yum": ["perl"],
    "brew": ["perl"],
    "choco": ["perl"]
  },
  "Swift": {
    "apt": ["swift"],
    "brew": ["swift"],
    "choco": []
  },
  "Kotlin": {
    "apt": ["kotlin"],
    "brew": ["kotlin"],
    "choco": ["kotlin"]
  },
  "Dart": {
    "apt": ["dart"],
    "brew": ["dart"],
    "choco": ["dart"]
  }
})";

std::vector<std::string> getpkg(const std::string& lang, const std::string& pm) {
    try {
        json config = json::parse(embedded_package_json);
        
        std::string lang_lower = lang;
        std::transform(lang_lower.begin(), lang_lower.end(), lang_lower.begin(), ::tolower);
        
        std::string matched_key;
        for (auto& el : config.items()) {
            std::string key_lower = el.key();
            std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(), ::tolower);
            if (key_lower == lang_lower) {
                matched_key = el.key();
                break;
            }
        }
        
        if (matched_key.empty()) {
            std::cerr << "Error: unsupported language '" << lang << "'\n";
            return {};
        }
        
        auto lang_config = config[matched_key];
        if (!lang_config.contains(pm)) {
            std::cerr << "Error: package manager '" << pm << "' does not support language '" << lang << "'\n";
            return {};
        }
        std::vector<std::string> pkgs;
        for (const auto& item : lang_config[pm]) {
            pkgs.push_back(item.get<std::string>());
        }
        return pkgs;
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return {};
    }
}

bool is_package_installed(const std::string& pm, const std::string& pkg) {
    std::string cmd;
    std::string redirect;

    if (pm == "choco" || pm == "winget" || pm == "scoop") {
        redirect = " >nul 2>&1";
    } else {
        redirect = " >/dev/null 2>&1";
    }

    if (pm == "apt") {
        cmd = "dpkg -s " + pkg + redirect;
    } else if (pm == "dnf" || pm == "yum") {
        cmd = "rpm -q " + pkg + redirect;
    } else if (pm == "pacman") {
        cmd = "pacman -Q " + pkg + redirect;
    } else if (pm == "brew") {
        cmd = "brew list " + pkg + redirect;
    } else if (pm == "choco") {
        cmd = "choco list --local-only " + pkg + redirect;
    } else if (pm == "winget") {
        cmd = "winget list --id " + pkg + redirect;
    } else if (pm == "scoop") {
        cmd = "scoop list " + pkg + redirect;
    } else {
        return false;
    }

    int ret = std::system(cmd.c_str());
    return ret == 0;
}

void list(const std::string& pm) {
    try {
        json config = json::parse(embedded_package_json);
        std::cout << "Supported languages and installation status (using " << pm << "):\n";
        for (auto& el : config.items()) {
            std::string lang = el.key();
            auto lang_config = el.value();
            if (!lang_config.contains(pm)) {
                std::cout << "  " << lang << " : Not supported by " << pm << "\n";
                continue;
            }
            auto pkgs = lang_config[pm];
            if (pkgs.empty()) {
                std::cout << "  " << lang << " : Not supported by " << pm << "\n";
                continue;
            }
            std::string main_pkg = pkgs[0].get<std::string>();
            bool installed = is_package_installed(pm, main_pkg);
            if (installed) {
                std::cout << "\033[32m  " << lang << " : Installed\033[0m\n";
            } else {
                std::cout << "  " << lang << " : Not installed\n";
            }
        }
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }
}

bool installpkg(const std::string& pm, const std::vector<std::string>& pkgs) {
    std::string cmd_prefix;
    if (pm == "apt") {
        cmd_prefix = "sudo apt update && sudo apt install -y";
    } else if (pm == "dnf" || pm == "yum") {
        cmd_prefix = "sudo " + pm + " install -y";
    } else if (pm == "pacman") {
        cmd_prefix = "sudo pacman -S --noconfirm";
    } else if (pm == "brew") {
        cmd_prefix = "brew install";
    } else if (pm == "choco") {
        cmd_prefix = "choco install -y";
    } else if (pm == "winget") {
        cmd_prefix = "winget install --id";
    } else if (pm == "scoop") {
        cmd_prefix = "scoop install";
    } else {
        std::cerr << "Unsupported package manager: " << pm << std::endl;
        return false;
    }

    bool all_success = true;
    for (const auto& pkg : pkgs) {
        std::string full_cmd = cmd_prefix + " " + pkg;
        int ret = std::system(full_cmd.c_str());
        if (ret != 0) {
            std::cerr << "Failed to install " << pkg << " (exit code: " << ret << ")" << std::endl;
            all_success = false;
        }
    }
    return all_success;
}

void print_usage(const char* progname) {
    std::cout << "Usage:\n"
              << "  " << progname << " install <language>   Install a language\n"
              << "  " << progname << " list                 List all supported languages and their installation status\n"
              << "Supported languages: C++, Rust, Go, Java, C#, Python, Node.js, Ruby, PHP, Perl, Swift, Kotlin, Dart\n";
}

void help(const char* progname) {
    std::cout << "Usage:\n"
              << "  " << progname << " install <language>   Install development packages for a programming language\n"
              << "  " << progname << " list                 List all supported languages and their installation status\n"
              << "\nSupported languages:\n"
              << "  C++, Rust, Go, Java, C#, Python, Node.js, Ruby, PHP, Perl, Swift, Kotlin, Dart\n"
              << "\nPackage managers automatically detected based on your system:\n"
              << "  Linux: apt, dnf, yum, pacman\n"
              << "  macOS: brew\n"
              << "  Windows: choco, winget, scoop\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        help(argv[0]);
        return 0;
    }

    if (cmd == "list") {
        std::string pm = package();
        if (pm.empty()) {
            std::cerr << "Error: No supported package manager found.\n"
                      << "Please install one of: apt, dnf, yum, pacman, brew, choco, winget, scoop.\n";
            return 1;
        }
        list(pm);
        return 0;
    }

    if (cmd == "install") {
        if (argc < 3) {
            std::cerr << "Error: missing language argument.\n";
            print_usage(argv[0]);
            return 1;
        }

        std::string lang = argv[2];
        std::string pm = package();
        if (pm.empty()) {
            std::cerr << "Error: No supported package manager found.\n"
                      << "Please install one of: apt, dnf, yum, pacman, brew, choco, winget, scoop.\n";
            return 1;
        }

        std::vector<std::string> packages = getpkg(lang, pm);
        if (packages.empty()) {
            return 1;
        }

        bool success = installpkg(pm, packages);
        if (success) {
            std::cout << "Installation completed successfully." << std::endl;
            return 0;
        } else {
            std::cerr << "Installation completed with errors." << std::endl;
            return 1;
        }
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    print_usage(argv[0]);
    return 1;
}
