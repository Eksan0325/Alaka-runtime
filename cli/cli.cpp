/**
 * @brief Alaka-runtime CLI Tool
 * @author Eksan Ablikim
 * @note Command:
 * version                      drop Alaka-runtime version
 * run [path] [--native]        start Alaka-runtime system and control all components
 * stop                         stop Alaka-runtime system and all components
 */

#include <string>
#include <vector>

#include <sys/stat.h>
#include <limits.h>

#include "system.hpp"

std::string fworkspace(const std::string& start) {
    std::string path = start.empty() ? "." : start;
    std::string cur = path;
    for (int idx = 0; idx < 8; ++idx) {
        struct stat st{};
        bool is_dir = stat((cur + "/components").c_str(), &st) == 0;
        bool is_exi = stat((cur + "/config/system.yaml").c_str(), &st) == 0 && S_ISDIR(st.st_mode);
        if (is_dir || is_exi) return cur;
        auto s = cur.rfind('/');
        if (s == std::string::npos || s == 0) break;
        cur.resize(s);
    }
    return {};
}

int cmd_run(const std::vector<std::string>& args) {
    std::string path(".");
    for (const std::string& arg : args) { 
        if (arg == "--native") continue; 
        if (arg.rfind("--", 0) != 0) path = arg;
    }
    const auto workspace = fworkspace(path);
    std::string root = workspace.empty() ? "." : workspace + "/target";
    char resolved[PATH_MAX] = {};
    
    if (::realpath(root.c_str(), resolved)) root = resolved;
    const std::string cfg = root + "/config/system.yaml";
    const std::string mgr = root + "/bin/alaka_launcher";

    if (!alaka::filesystem::exists(cfg)) return -1;
    if (!alaka::filesystem::exists(mgr)) return -1;
    const int pid = alaka::helper::find_pid_by_name("alaka_launcher");  
    if (pid > 0) return -1;

    ::setenv("ALAKA_ROOT", root.c_str(), 1);
    std::string ld = "LD_LIBRARY_PATH=" + root + "/lib/midware:" + root + "/lib/3rdparty:" +
                     (std::getenv("LD_LIBRARY_PATH") ? std::getenv("LD_LIBRARY_PATH") : "");
    ::putenv(const_cast<char*>(ld.c_str()));
    ::chdir(root.c_str());
    ::execl(mgr.c_str(), "alaka_launcher", cfg.c_str(), static_cast<char*>(nullptr));
    return 0;
}

int main(int argc, char** argv) {
    const std::string cmd = argv[1];
    std::vector<std::string> args(argv + 2, argv + argc);
    if (cmd == "run") return cmd_run(args);
    return -1;
}