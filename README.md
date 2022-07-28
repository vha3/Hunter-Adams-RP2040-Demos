# Demo Code for the Raspberry Pi Pico
#### [V. Hunter Adams](https://vanhunteradams.com)

## What are these demos?

This is a collection of RP2040 examples created and assembled for ECE 4760 at Cornell. ECE 4760 students are asked to clone this repository and add their own lab assignments and projects. Some links to course materials are provided below.

> - [Course website](https://ece4760.github.io)
> - [Lab 1](https://vanhunteradams.com/Pico/Cricket/Crickets.html): Cricket Chirp Synthesis and Synchronization
> - [Lab 2](https://vanhunteradams.com/Pico/Animal_Movement/Animal_Movement.html): Modeling Decision Making in Animal Groups on the Move
> - [Lab 3](https://vanhunteradams.com/Pico/ReactionWheel/ReactionWheel.html): PID Control of an Inverted Pendulum with a Reaction Wheel

This repository is modeled off of the [pico-examples](https://github.com/raspberrypi/pico-examples) repository from Raspberry Pi. To build the examples in this repository, please install the RP2040 toolchain and SDK as described in the [Getting Started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) document. The two pages linked below provide explicit instructions for Windows and Mac users. For Linux users, setup is easy (see chapters 1 and 2 of the Getting Started document linked previously). 

> - [Building these examples on Windows](https://vanhunteradams.com/Pico/Setup/PicoSetup.html)
> - [Building these examples on Mac](https://vanhunteradams.com/Pico/Setup/PicoSetupMac.html)

If you are an ECE 4760 student, please note that the toolchain is *already installed* on the lab PC's in Phillips 238. You may still find the above links useful for installation on your own computer.

## How do I use them?

If you are an ECE 4760 student, you will clone this repository into the same directory which contains your copy of the [pico sdk](https://github.com/raspberrypi/pico-sdk). 

For Lab 1, you'll add a folder to the [Lab_1](Lab_1) directory. For Lab 2, you'll add a folder to the [Lab_2](Lab_2) directory. And for Lab 3, you'll add a folder to the [Lab_3](Lab_3) directory. Each of those folders contains the demos from which you should start each lab. The [Lab_1_Incremental](Lab_1_Incremental) directory contains a sequence of demos that incrementally builds to the starting demos from which you start Lab 1. This is provided to help you build an understanding of the Lab 1 demo code, and to help me introduce the Lab 1 content in class.

For your final project, you will add a folder to the root directory.