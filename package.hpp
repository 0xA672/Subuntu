#include <string>
#include <cstdlib>

std::string package() {
#ifdef __linux__
    if (system("which apt >/dev/null 2>&1") == 0) return "apt";
    if (system("which dnf >/dev/null 2>&1") == 0) return "dnf";
    if (system("which yum >/dev/null 2>&1") == 0) return "yum";
    if (system("which pacman >/dev/null 2>&1") == 0) return "pacman";
    else return ""; 
#elif __APPLE__
    if (system("which brew >/dev/null 2>&1") == 0) return "brew";
    else return ""; 
#elif _WIN32
    if (system("where choco >nul 2>nul") == 0) return "choco";
    if (system("where winget >nul 2>nul") == 0) return "winget";
    if (system("where scoop >nul 2>nul") == 0) return "scoop";
    else return ""; 
#endif
    return "";
}