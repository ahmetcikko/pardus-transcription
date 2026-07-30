#include "dictation.h"
#include "keybinding.h"
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <linux/limits.h>
#include <string>
#include <sys/file.h>
#include <unistd.h>

static std::string exe_dir() {
    char buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length <= 0)
        return ".";
    buffer[length] = '\0';
    return std::filesystem::path(buffer).parent_path().string();
}

static void ensure_registered() {
    const char *home = getenv("HOME");
    if (!home)
        return;
    std::string directory = std::string(home) + "/.config/pardus-dikte";
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    std::string flag = directory + "/registered";
    if (std::filesystem::exists(flag))
        return;
    if (keybinding_register())
        std::ofstream(flag) << "1";
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        std::string argument = argv[1];
        if (argument == "--register")
            return keybinding_register() ? 0 : 1;
        if (argument == "--unregister")
            return keybinding_unregister() ? 0 : 1;
    }

    // another instance already holds the lock: exit quietly instead of erroring
    int lock =
        open("/tmp/pardus-dikte.lock", O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (lock < 0 || flock(lock, LOCK_EX | LOCK_NB) != 0)
        return 0;

    QApplication app(argc, argv);
    ensure_registered();
    std::string model = exe_dir() + "/ggml-small-q5_1.bin";
    Dictation dictation(model);
    QQmlApplicationEngine engine;
    (*engine.rootContext()).setContextProperty("backend", &dictation);
    engine.load(QUrl("qrc:/src/main.qml"));
    if (engine.rootObjects().isEmpty())
        dictation.startFallback();
    else
        dictation.start();
    return app.exec();
}
