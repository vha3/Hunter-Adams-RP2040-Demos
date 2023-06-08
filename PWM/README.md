# Pulse-Width Modulation Demonstrations

#### AM Radio Beacon
- A PWM channel at the carrier frequency is modulated on/off at the desired audio frequency. Tuning a nearby AM radio to the appropriate channel makes the generated "beeps" audible
- [**Documentation available here**](https://vanhunteradams.com/Pico/AM_Radio/AM.html)

#### AM Radio Voice
- A microphone is attached to an ADC input. A DMA channel samples the ADC at 10kHz, and uses the ADC sample to set the duty cycle for a PWM channel running at the carrier frequency. This modulates the first term in the Taylor Expansion for the square wave PWM output, which is AM demodulated by a nearby radio tuned to the appropriate channel.
- This allows for AM transmission of voice with no CPU interaction
- [**Documentation available here**](https://vanhunteradams.com/Pico/AM_Radio/AM.html)

#### PWM Demo <--- *Starting point for Lab 3*
- A basic PWM demonstration that involves Protothreads
- RP2040 generates a PWM output, the user can specify the duty cycle of that PWM output via a serial interface