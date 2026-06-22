/* KallistiOS ##version##

   examples/dreamcast/cpp/clock/clock.cc

   Copyright (C) Peter Hatch
   Copyright (C) Megan Pottter
   Copyright (C) 2023, 2026 Falco Girgis

*/

/*
    This example displays the following information, updating in real-time:
        - Current Unix timestamp
        - Current date
        - Current timestamp (nanosecond resolution)

    It uses C++'s std::chrono timing API for all of this, and additionally
    leverages as much modern functioanlity from C++23 as is possible, both
    to provide examples to users and to serve as a sanity test for modern
    C++ features in the toolchain for us KOS devs. :)
*/

#include <kos.h>
#include <dcplib/fnt.h>

#include <cmath>
#include <cstdlib>

#include <array>
#include <algorithm>
#include <functional>
#include <chrono>
#include <print>
#include <format>

namespace {

struct bg_state {
    std::array<float, 3> color = { 0.0f };
    std::array<float, 3> delta = { 0.01f };
    size_t index = 1;

    constexpr static const
    std::array<std::array<float, 3>, 8> presets = {{
        { 0.0f, 0.0f, 0.0f },
        { 0.5f, 0.0f, 0.0f },
        { 0.0f, 0.5f, 0.0f },
        { 0.0f, 0.0f, 0.5f },
        { 0.5f, 0.0f, 0.5f },
        { 0.0f, 0.5f, 0.5f },
        { 0.5f, 0.5f, 0.0f },
        { 0.5f, 0.5f, 0.5f }
    }};
};

void update_bg(bg_state* bg) {
    pvr_set_bg_color(bg->color[0], bg->color[1], bg->color[2]);

    std::ranges::transform(bg->color, bg->delta, bg->color.begin(), std::plus<float>());

    if(std::ranges::equal(bg->color, bg->presets[bg->index],
                          [](float a, float b) { return std::abs(a - b) < 0.01f; }))
    {
        if(++bg->index >= bg->presets.size())
            bg->index = 0;

        std::ranges::transform(bg->presets[bg->index], bg->color, bg->delta.begin(),
                               [](float a, float b) { return (a - b) / (0.5f / 0.01f); });
    }
}

void draw_scene(fntRenderer* text) {
    int y = 50;

    auto timestamp = std::chrono::high_resolution_clock::now();
    auto duration  = timestamp.time_since_epoch();
    auto seconds   = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto unix_secs = std::chrono::system_clock::to_time_t(timestamp);
    auto nanosecs  = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds);

    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);

    text->begin();
    text->setColor(1, 1, 1);
    text->start2f(20, y);
    text->puts("(High Res) Simple DC Clock");
    text->end();
    y += 50;

    text->begin();
    text->setColor(1, 1, 1);
    text->start2f(20, y);
    text->puts(std::format("Unix: {}", unix_secs).c_str());
    text->end();
    y += 50;

    text->begin();
    text->setColor(1, 1, 1);
    text->start2f(20, y);
    text->puts(std::format("{0:%A} {0:%B} {0:%d}, {0:%Y}", timestamp).c_str());
    text->end();
    y += 50;

    text->begin();
    text->setColor(1, 1, 1);
    text->start2f(20, y);
    text->puts(std::format("{:%H:%M:%S}", timestamp).c_str());
    text->end();
    y += 50;

    pvr_scene_finish();
}

bool read_input() {
    if(auto *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER); cont) {
        /* Check for start on the controller */
        auto *state = static_cast<cont_state_t *>(maple_dev_status(cont));

        if(!state) {
            std::println(stderr, "Error getting controller status.");
            return true;
        }

        if(state->start) {
            std::println("Pressed start.");
            return true;
        }
    }

    return false;
}

}

int main(int argc, char **argv) {
    const pvr_init_params_t pvrInit = {
        { PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_0 },
        512 * 1024, 0, 0, 0, 0, 0
    };

    pvr_init(&pvrInit);

    fntRenderer text;
    fntTexFont font("/rd/default.txf");
    bg_state bg;

    text.setFilterMode(PVR_FILTER_NEAREST);
    text.setFont(&font);
    text.setPointSize(30);

    while(!read_input()) {
        update_bg(&bg);
        draw_scene(&text);
    }

    return EXIT_SUCCESS;
}
