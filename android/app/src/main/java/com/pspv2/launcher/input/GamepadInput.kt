package com.pspv2.launcher.input

import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent

/**
 * Normalises input from Bluetooth HID gamepads (the "Serafim" controllers and any
 * other standard Android gamepad) plus keyboard fallback into the small set of XMB
 * navigation actions used by the UI. Replaces SFML's sf::Joystick polling from the
 * desktop build, which is unreliable on Android.
 */
enum class GamepadAction {
    UP, DOWN, LEFT, RIGHT, CONFIRM, CANCEL, MENU, NONE
}

object GamepadInput {

    /** True for D-pad / button sources we care about. */
    fun isFromGamepad(deviceId: Int): Boolean {
        val device = InputDevice.getDevice(deviceId) ?: return false
        val sources = device.sources
        return (sources and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD) ||
            (sources and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK) ||
            (sources and InputDevice.SOURCE_DPAD == InputDevice.SOURCE_DPAD)
    }

    /** Maps a key-down KeyEvent (gamepad buttons, D-pad, or keyboard) to an action. */
    fun fromKeyEvent(event: KeyEvent): GamepadAction {
        if (event.action != KeyEvent.ACTION_DOWN || event.repeatCount != 0) return GamepadAction.NONE
        return when (event.keyCode) {
            KeyEvent.KEYCODE_DPAD_UP -> GamepadAction.UP
            KeyEvent.KEYCODE_DPAD_DOWN -> GamepadAction.DOWN
            KeyEvent.KEYCODE_DPAD_LEFT -> GamepadAction.LEFT
            KeyEvent.KEYCODE_DPAD_RIGHT -> GamepadAction.RIGHT

            // PSP/PlayStation cross (and gamepad A) = confirm.
            KeyEvent.KEYCODE_BUTTON_A,
            KeyEvent.KEYCODE_ENTER,
            KeyEvent.KEYCODE_DPAD_CENTER,
            KeyEvent.KEYCODE_SPACE -> GamepadAction.CONFIRM

            // PSP/PlayStation circle (and gamepad B) = cancel/back.
            KeyEvent.KEYCODE_BUTTON_B,
            KeyEvent.KEYCODE_BACK,
            KeyEvent.KEYCODE_ESCAPE -> GamepadAction.CANCEL

            KeyEvent.KEYCODE_BUTTON_START,
            KeyEvent.KEYCODE_BUTTON_SELECT,
            KeyEvent.KEYCODE_MENU -> GamepadAction.MENU

            else -> GamepadAction.NONE
        }
    }

    /**
     * Maps analog stick / hat motion to a discrete action. Caller is responsible for
     * de-bouncing (only emit when crossing the dead-zone, then reset on return to centre).
     */
    fun fromMotionEvent(event: MotionEvent): GamepadAction {
        val deadZone = 0.5f

        // D-pad reported as a hat axis on many controllers.
        val hatX = event.getAxisValue(MotionEvent.AXIS_HAT_X)
        val hatY = event.getAxisValue(MotionEvent.AXIS_HAT_Y)
        if (hatX <= -deadZone) return GamepadAction.LEFT
        if (hatX >= deadZone) return GamepadAction.RIGHT
        if (hatY <= -deadZone) return GamepadAction.UP
        if (hatY >= deadZone) return GamepadAction.DOWN

        val x = event.getAxisValue(MotionEvent.AXIS_X)
        val y = event.getAxisValue(MotionEvent.AXIS_Y)
        return when {
            x <= -deadZone -> GamepadAction.LEFT
            x >= deadZone -> GamepadAction.RIGHT
            y <= -deadZone -> GamepadAction.UP
            y >= deadZone -> GamepadAction.DOWN
            else -> GamepadAction.NONE
        }
    }

    /** True once all relevant axes are back inside the dead-zone (for de-bouncing). */
    fun isCentered(event: MotionEvent): Boolean {
        val deadZone = 0.5f
        val axes = listOf(
            MotionEvent.AXIS_X, MotionEvent.AXIS_Y,
            MotionEvent.AXIS_HAT_X, MotionEvent.AXIS_HAT_Y
        )
        return axes.all { kotlin.math.abs(event.getAxisValue(it)) < deadZone }
    }
}
