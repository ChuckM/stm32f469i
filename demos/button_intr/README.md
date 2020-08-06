Interupt Driven Button Example
------------------------------

Interrupt driven button manager. This code uses the EXTI subsystem to watch
the button for you. If the button is pressed, an interrupt service routine
is called. A volatile global variable `button_pressed` is used to track
activations of the button.

Because the ISR captures button press and button release transitions,
it can do interesting things like measure how long the button has been
pressed. If you were using code like this in a keyboard it could give
you "key down" / "key up" events.

The "wakeup" button (it is blue in color)  on the STM32F469I-DISCO is
connected to GPIO pin `PA0`. This code sets up PA0 to activate interrupt
EXTI0 when the button changes state. The ISR notes (in global variables)
that the button was pressed and for how long.

The main loop monitors the global variable `button_pressed` and turns
on the user LEDs in succession, just like the simple polled example does.

### Bouncing Buttons

This was the first attempt at capturing button state, it doesn't
work well.

```c
void
exti0_isr(void)
{
	/*
	 * If GPIO0 is "high" button is pressed so this was a
	 * a transition from not pressed to pressed (rising edge)
	 */
	if (gpio_get(GPIOA, GPIO0)) {
		button_press++; 		/* button press */
		button_down = mtime();	/* copy systick value */
	} else {
		press_time = mtime() - button_down; /* mS of press time */
		button_down = 0;
	}
	EXTI_PR = 1;
}
```

You can look at the code that works for the solution but perhaps you can
guess the problem?

This code has no way to protect against a "short" press, a glitch or
a bounce if you will. As a result it can sometimes see 2 or 3 button presses
when the user feels as if they pressed once.
