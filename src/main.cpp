#include "Application/Application.h"

#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    try {
        Application app;
        app.Initialize(argc, argv);
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        // Maybe show a message box?
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "FATAL ERROR: Unknown exception caught." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
