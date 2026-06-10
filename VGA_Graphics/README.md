# PIO-Based VGA Driver for RP2040
#### [V. Hunter Adams](https://vanhunteradams.com)

This repository contains a collection of VGA examples for the Raspberry Pi Pico. 

## Documentation
[**Documentation for the VGA driver is available here.**](https://vanhunteradams.com/Pico/VGA/VGA.html)

#### Animation Demo <--- *Starting point for Lab 2*
- A basic animation demonstration, which incorporates multicore, protothreads, and double-buffering
- Two balls bounce around in a box, the user can change the color of the balls via a serial interface

#### Barnsley Fern
- Computes and renders the [Barnsley Fern](https://en.wikipedia.org/wiki/Barnsley_fern)
- [Video of Barnsley Fern](https://www.youtube.com/watch?v=XR2Ptu-vrDo&list=PLDqMkB5cbBA4W8_FkjXW4WdzXWH0-Xyny&index=3)

#### Game of Life
- Computes and animates [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)
- [Video of Game of Life](https://www.youtube.com/watch?v=SpzD_NGPbp4&list=PLDqMkB5cbBA4W8_FkjXW4WdzXWH0-Xyny&index=4&t=23s)

#### Mandelbrot Set
- Uses both cores of the RP2040 to compute/render the [Mandelbrot Set](https://en.wikipedia.org/wiki/Mandelbrot_set).
- Half of the screen is computed using floating point, and half using fixed point. Visually demonstrates speedup from fixed point.
- [Video of Mandelbrot Set](https://www.youtube.com/watch?v=ySxg6M0f0eo&list=PLDqMkB5cbBA52vmAp0_8pW_GcbBtdBghU&index=9)

#### Fonts and Colors (from Bruce)
- This demo shows the six defined fonts which range from a small 5x7 pixel font to a 16x32 monster. It shows the color range and the defined color names, then falls into an animation loop which just moves graphics primitives acorss the screen. As the shapes move across pixel (320,120) their color is read back from the draw buffer and displayed.
- *Probably the most useful starting point for general graphics projects*
- [Bruce's Documentation](https://people.ece.cornell.edu/land/courses/ece4760/pi_pico/vga16_v3/index_vga16_v3.html)

#### VGA Graphics Primitives (from Bruce)
- This test attempted to fill the screen with lots of moving pixels to check for flicker and double buffer errors. It also demonstrates the triangle primitive and the polyline primitive. The yellow text in the lower left corner is drawn at random phase on the second core. This verifys that the new text drawing routines are re-entrant.
- [Bruce's Documentation](https://people.ece.cornell.edu/land/courses/ece4760/pi_pico/vga16_v3/index_vga16_v3.html)