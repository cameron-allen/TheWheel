#include "pch.h"
#include "core/engine.h"

int main() {
    Core& app = Core::GetInstance();

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
