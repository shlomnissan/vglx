/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include <gtest/gtest.h>
#include <test_helpers.hpp>

#include <vglx/canvas/canvas.hpp>
#include <vglx/canvas/node2d.hpp>
#include <vglx/events/keyboard_event.hpp>

#include <vector>

namespace {

// Records the order in which nodes receive updates and events.
class Recorder : public vglx::Node2D {
public:
    Recorder(std::vector<int>* log, int id, bool handles = false)
        : log_(log), id_(id), handles_(handles) {}

    auto OnUpdate(float) -> void override {
        log_->push_back(id_);
    }

    auto OnKeyboardEvent(vglx::KeyboardEvent* event) -> void override {
        log_->push_back(id_);
        if (handles_) event->handled = true;
    }

private:
    std::vector<int>* log_;
    int id_;
    bool handles_;
};

class RecordingCanvas : public vglx::Canvas {
public:
    explicit RecordingCanvas(std::vector<int>* log) : log_(log) {}

    auto OnUpdate(float) -> void override {
        log_->push_back(0);
    }

    auto OnKeyboardEvent(vglx::KeyboardEvent*) -> void override {
        log_->push_back(0);
    }

private:
    std::vector<int>* log_;
};

}

#pragma region Resize

TEST(Canvas, ResizeBuildsProjection) {
    auto canvas = vglx::Canvas::Create();
    canvas->Resize(200, 100);

    EXPECT_VEC2_EQ(canvas->GetSize(), {200.0f, 100.0f});
    EXPECT_MAT3_EQ(canvas->projection_matrix, {
        0.01f, 0.0f, -1.0f,
        0.0f, -0.02f, 1.0f,
        0.0f, 0.0f, 1.0f
    });
}

TEST(Canvas, ResizeMapsCornersToClipSpace) {
    auto canvas = vglx::Canvas::Create();
    canvas->Resize(200, 100);

    const auto& p = canvas->projection_matrix;

    EXPECT_VEC3_EQ(p * vglx::Vector3 {0.0f, 0.0f, 1.0f}, {-1.0f, 1.0f, 1.0f});
    EXPECT_VEC3_EQ(p * vglx::Vector3 {200.0f, 100.0f, 1.0f}, {1.0f, -1.0f, 1.0f});
    EXPECT_VEC3_EQ(p * vglx::Vector3 {100.0f, 50.0f, 1.0f}, {0.0f, 0.0f, 1.0f});
}

TEST(Canvas, ResizeWithEmptyDimensionKeepsProjection) {
    auto canvas = vglx::Canvas::Create();
    canvas->Resize(200, 100);
    canvas->Resize(200, 0);

    EXPECT_VEC2_EQ(canvas->GetSize(), {200.0f, 0.0f});
    EXPECT_MAT3_EQ(canvas->projection_matrix, {
        0.01f, 0.0f, -1.0f,
        0.0f, -0.02f, 1.0f,
        0.0f, 0.0f, 1.0f
    });
}

#pragma endregion

#pragma region Advance

TEST(Canvas, AdvanceVisitsCanvasThenDepthFirst) {
    auto log = std::vector<int> {};
    auto canvas = RecordingCanvas {&log};
    auto a = canvas.Add(std::make_unique<Recorder>(&log, 1));
    a->Add(std::make_unique<Recorder>(&log, 3));
    canvas.Add(std::make_unique<Recorder>(&log, 2));

    canvas.Advance(0.016f);

    EXPECT_EQ(log, (std::vector<int> {0, 1, 3, 2}));
}

#pragma endregion

#pragma region HandleEvent

TEST(Canvas, HandleEventVisitsChildrenBeforeParents) {
    auto log = std::vector<int> {};
    auto canvas = RecordingCanvas {&log};
    auto a = canvas.Add(std::make_unique<Recorder>(&log, 1));
    a->Add(std::make_unique<Recorder>(&log, 3));
    canvas.Add(std::make_unique<Recorder>(&log, 2));

    auto event = vglx::KeyboardEvent {};
    canvas.HandleEvent(&event);

    EXPECT_EQ(log, (std::vector<int> {3, 1, 2, 0}));
}

TEST(Canvas, HandleEventStopsWhenHandled) {
    auto log = std::vector<int> {};
    auto canvas = RecordingCanvas {&log};
    auto a = canvas.Add(std::make_unique<Recorder>(&log, 1));
    a->Add(std::make_unique<Recorder>(&log, 3, true));
    canvas.Add(std::make_unique<Recorder>(&log, 2));

    auto event = vglx::KeyboardEvent {};
    canvas.HandleEvent(&event);

    EXPECT_TRUE(event.handled);
    EXPECT_EQ(log, (std::vector<int> {3}));
}

#pragma endregion
