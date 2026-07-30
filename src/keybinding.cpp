#include "keybinding.h"
#include <cstdio>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

static const char *SCHEMA = "org.gnome.settings-daemon.plugins.media-keys";
static const char *KEY_PATH =
    "/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/"
    "pardus-dikte/";
static const char *BINDING = "<Super><Shift>d";

static std::string run(const std::string &command) {
    std::string out;
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe)
        return out;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe))
        out += buffer;
    pclose(pipe);
    return out;
}

static std::string trim(const std::string &value) {
    size_t begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return "";
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

static std::string self_path() {
    char buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length <= 0)
        return "";
    buffer[length] = '\0';
    return std::string(buffer);
}

static std::string list_key() {
    return trim(
        run(std::string("gsettings get ") + SCHEMA + " custom-keybindings"));
}

bool keybinding_register() {
    std::string command = self_path();
    if (command.empty())
        return false;
    std::string list = list_key();
    if (list.find(KEY_PATH) == std::string::npos) {
        std::string updated;
        // gsettings prints an empty typed array as "@as []", not "[]"
        if (list == "@as []" || list == "[]" || list.empty())
            updated = std::string("['") + KEY_PATH + "']";
        else
            updated = std::string("['") + KEY_PATH + "', " + list.substr(1);
        run(std::string("gsettings set ") + SCHEMA + " custom-keybindings \"" +
            updated + "\"");
    }
    std::string base = std::string("gsettings set ") + SCHEMA +
                       ".custom-keybinding:" + KEY_PATH + " ";
    run(base + "name \"'Pardus Dikte'\"");
    run(base + "command \"'" + command + "'\"");
    run(base + "binding \"'" + BINDING + "'\"");
    return true;
}

bool keybinding_unregister() {
    std::string list = list_key();
    std::string token = std::string("'") + KEY_PATH + "'";
    size_t position = list.find(token);
    if (position != std::string::npos) {
        size_t end = position + token.size();
        if (list.compare(end, 2, ", ") == 0)
            end += 2;
        else if (position >= 2 && list.compare(position - 2, 2, ", ") == 0)
            position -= 2;
        std::string updated = list.substr(0, position) + list.substr(end);
        if (updated == "[]")
            updated = "@as []";
        run(std::string("gsettings set ") + SCHEMA + " custom-keybindings \"" +
            updated + "\"");
    }
    run(std::string("gsettings reset-recursively ") + SCHEMA +
        ".custom-keybinding:" + KEY_PATH);
    return true;
}
