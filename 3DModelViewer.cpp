#include "engine.h"

int main(int argc, char** argv) {
    // define screen dimensions
    const int SCREEN_WIDTH = 1280;
    const int SCREEN_HEIGHT = 720;

    // initialize Engine
    Engine engine(SCREEN_WIDTH, SCREEN_HEIGHT);

    // begin main loop
    engine.loop();

    // quit
    return 0;
}
