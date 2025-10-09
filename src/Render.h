#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <future>
#include <d3d11.h>

class Render
{
public:
    void render(ID3D11Device *pd3dDevice);

    Render(bool &show_window) : show_window(show_window){}

    bool show_window;
};
