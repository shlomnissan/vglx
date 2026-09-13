/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/events/event.hpp"

namespace vglx {

enum class GamepadButton;
enum class GamepadAxis;

/**
 * @brief Represents a gamepad input event.
 *
 * A gamepad event is dispatched when a gamepad is connected or disconnected,
 * a button is pressed or released, or an axis changes. It extends the base
 * @ref Event with data specific to gamepad input: the @ref GamepadEvent::Type
 * "interaction type", the @ref GamepadEvent::gamepad "gamepad id", and the
 * @ref GamepadEvent::button "button" or @ref GamepadEvent::axis "axis" involved.
 *
 * Gamepad events are dispatched through the @ref Scene hierarchy where nodes
 * can override the @ref Node::OnGamepadEvent handler and optionally mark the
 * event as @ref Event::handled "handled". When handled is set to `true` the
 * event stops propagating to other nodes.
 *
 * @code
 * class MyNode : public vglx::Node {
 * public:
 *   auto OnGamepadEvent(vglx::GamepadEvent* event) -> void override {
 *     if (event->type == vglx::GamepadEvent::Type::ButtonPressed) {
 *       if (event->button == vglx::GamepadButton::RightFaceDown) {
 *         // Do something...
 *         event->handled = true; // stop propagation
 *       }
 *     }
 *   }
 * };
 * @endcode
 *
 * @ingroup EventsGroup
 */
struct VGLX_EXPORT GamepadEvent : public Event {
    /**
     * @brief Enumerates all gamepad event types.
     *
     * Distinguishes between connection changes, button transitions, and
     * axis changes.
     */
    enum class Type {
        Connected, ///< Gamepad was connected.
        Disconnected, ///< Gamepad was disconnected.
        ButtonPressed, ///< Button transitioned to the down state.
        ButtonReleased, ///< Button transitioned to the up state.
        AxisMoved ///< Axis value changed.
    };

    /// @brief The interaction @ref GamepadEvent::Type "type" for this event.
    GamepadEvent::Type type;

    /**
     * @brief Identifier of the gamepad that generated the event.
     *
     * Stable for as long as the gamepad stays connected.
     */
    int gamepad;

    /**
     * @brief Gamepad button associated with the event, if any.
     *
     * Refer to the source code for enum details.
     */
    GamepadButton button;

    /**
     * @brief Gamepad axis associated with the event, if any.
     *
     * Refer to the source code for enum details.
     */
    GamepadAxis axis;

    /**
     * @brief Current value of the @ref GamepadEvent::axis "axis".
     *
     * Sticks range from -1 to 1, with positive Y pointing down, and read 0
     * inside a small deadzone. Triggers range from 0 (released) to 1.
     */
    float value;

    /**
     * @brief Identifies this event as @ref Event::Type "Event::Type::Gamepad".
     */
    auto GetType() const -> Event::Type override {
        return Event::Type::Gamepad;
    }
};

enum class GamepadButton {
    None,
    RightFaceDown, ///< Xbox: A, PlayStation: Cross, Nintendo: B.
    RightFaceRight, ///< Xbox: B, PlayStation: Circle, Nintendo: A.
    RightFaceLeft, ///< Xbox: X, PlayStation: Square, Nintendo: Y.
    RightFaceUp, ///< Xbox: Y, PlayStation: Triangle, Nintendo: X.
    LeftBumper,
    RightBumper,
    Back,
    Start,
    Guide,
    LeftThumb,
    RightThumb,
    DpadUp,
    DpadRight,
    DpadDown,
    DpadLeft
};

enum class GamepadAxis {
    None,
    LeftX,
    LeftY,
    RightX,
    RightY,
    LeftTrigger,
    RightTrigger
};

}
