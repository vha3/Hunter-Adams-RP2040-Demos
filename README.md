# Demo Code for the Raspberry Pi Pico (1 and 2)
#### [V. Hunter Adams](https://vanhunteradams.com)

## What are these demos?

This is a collection of RP2040/RP2350 examples created and assembled for ECE 4760 at Cornell. These provide starting points for student assignments and projects. Some links to course materials are provided below. All demos should work with both the Pi Pico (RP2040) and the Pi Pico 2 (RP2350). For demos that use VGA, just be sure that double-buffering is *disabled* if you're using the RP2040 (there isn't enough RAM to double-buffer). You enable/disable double-buffering via [the macros](https://github.com/vha3/Hunter-Adams-RP2040-Demos/blob/917b5bf8adba8e312f70fd9bf9608993aadb5fea/VGA_Graphics/VGA_Fonts_and_Colors/VGA/vga16_graphics_v3.c#L85) in the vga16_graphics_v3.c files.

> - [Course website](https://ece4760.github.io)
> - [Lab 1](https://vanhunteradams.com/Pico/Birds/Birdsong.html): Synthesizing birdsong
> - [Lab 2](https://vanhunteradams.com/Pico/Galton/Galton.html): Digital Galton board
> - [Lab 3](https://vanhunteradams.com/Pico/Helicopter/Helicopter.html): PID Control of a 1D helicopter
> - [Alternative Lab 1](https://vanhunteradams.com/Pico/Cricket/Crickets.html): Synthesizing and synchronizing cricket chirps
> - [Alternative Lab 2](https://vanhunteradams.com/Pico/Animal_Movement/Boids_Lab.html): Animating murmurations of starlings
> - [Alternative Lab 3](https://vanhunteradams.com/Pico/ReactionWheel/ReactionWheel.html): PID control of an inverted pendulum with a reaction wheel.

As of 6/30/2025, this repository has been refactored such that **every demo is a standalone project.** As such, you can download/clone this directory and import any project using the Raspberry Pi Pico VSCode extension and it will work. For detailed instructions, please [see here](https://vanhunteradams.com/Pico/CourseMaterials/Building_Demos.html). 


You may also be interested in some meta-information about this class:

> - [What makes a good laboratory exercise?](https://vanhunteradams.com/Pico/CourseMaterials/Lab_Criteria.html)
> - [Hunter's teaching philosophy](https://vanhunteradams.com/Professional/Teaching.pdf)
> - [Hunter's love letter to embedded systems](https://www.youtube.com/watch?v=-TFsfcIx04Q)
