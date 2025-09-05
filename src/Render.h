#pragma once

#include <string>

class Render
{
public:
    void render();

    Render(bool &show_window) : show_window(show_window) {}

    bool show_window;
};
