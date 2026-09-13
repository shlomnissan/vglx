/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

namespace vglx {

/**
 * @brief Base class for all event objects.
 *
 * This class defines the core interface for all event types. Derived event
 * types extend this interface with additional data specific to their category.
 *
 * @ingroup EventsGroup
 */
struct VGLX_EXPORT Event {
    /**
     * @brief Enumerates all event types.
     *
     * Each event subclass identifies its type through @ref Event::GetType,
     * allowing runtime checks to distinguish between different kinds of events.
     */
    enum class Type {
        Keyboard, ///< Event triggered by keyboard input.
        Mouse, ///< Event triggered by mouse input.
        Gamepad, ///< Event triggered by gamepad input.
        Undefined ///< Fallback for undefined or uninitialized event types.
    };

    /**
     * @brief Indicates whether the event has been handled.
     *
     * When set to `true`, this flag prevents the event from propagating
     * further to other nodes in the scene graph. Handlers should set this
     * to `true` if no other node should respond to the event.
     */
    bool handled {false};

    /**
     * @brief Returns the @ref Event::Type "default event" type.
     *
     * Derived classes should override this to return their corresponding
     * event type.
     */
    [[nodiscard]] virtual auto GetType() const -> Event::Type {
        return Event::Type::Undefined;
    }

    virtual ~Event() = default;
};

}