#include "ui/App.h"
#include <memory>

int main() {
    auto app = std::make_unique<App>();
    app->run();
    return 0;
}
