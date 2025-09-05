#include <iostream>
#include <list>
#include <map>
#include <ranges>
#include <string>

#include "httpclient/httpclient.h"
#include "imgui.h"

#include "Constants.h"
#include "Render.h"

namespace
{
}

void Render::render()
{
    if (!show_window)
        return;

    if (ImGui::Begin("GW2TP", &show_window))
    {

    }

    ImGui::End();
}
