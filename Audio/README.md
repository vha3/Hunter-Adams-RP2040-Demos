# Audio synthesis and processing examples of incremental complexity

Lab 1 for [ECE 4760](https://ece4760.github.io) changes from year to year, but it always involves audio synthesis and/or audio digital signal processing. This repository contains a sequence of example audio projects which incrementally add complexity. The documentation for these examples is meant to be read *in order*, starting form the Timer Interrupt DDS Demo and moving down. This repository exists both to help you gain an understanding of the demo code provided for Lab 1, and also to help me introduce the Lab 1 content in class.

You might also find [Lectures 1-7](https://www.youtube.com/playlist?list=PLDqMkB5cbBA5oDg8VXM110GKc-CmvUqEZ) useful for these demos.


<!-- #### Blinky Demo
- Blinks the Pico's onboard LED by toggling a GPIO port.
- The "Hello world!" of a new microcontroller.
- [**Documentation for this example**](https://vanhunteradams.com/Pico/Setup/UsingPicoSDK.html) -->
#### Timer Interrupt DDS Demo
- This example includes a timer interrupt, and [SPI communication](https://vanhunteradams.com/Protocols/SPI/SPI.html) to a DAC.
- The timer interrupt performs [Direct Digital Synthesis](https://vanhunteradams.com/DDS/DDS.html) of a sine wave, which is output through the [SPI DAC](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/20002249B.pdf). 

- [**Documentation for this example**](https://vanhunteradams.com/Pico/TimerIRQ/SPI_DDS.html)
#### Multicore DDS Demo
- Uses two timer interrupts to perform Direct Digital Synthesis of two sine waves, one on each of the two cores of the RP2040. 
- Those sine waves are amplitude-modulated to "beeps" using [Fixed Point arithmetic](https://vanhunteradams.com/FixedPoint/FixedPoint.html)
- [**Documentation for this example**](https://vanhunteradams.com/Pico/Multi/MultiCore.html)
#### Protothreads Demo
- A thorough demonstration of Protothreads, a lightweight threading library which will be used in *every lab*.
- [**Documentation from Bruce**](https://people.ece.cornell.edu/land/courses/ece4760/RP2040/C_SDK_protothreads/index_Protothreads.html)
#### Audio Beep Synthesis Single Core <--- *Starting point for Lab 1*
- Uses a timer interrupt to perform Direct Digital Synthsis of a sine wave out of a single core
- This sine wave is amplitude-modulated into a "beep"
- Incorporates Protothreads
#### Audio Beep Synthesis Multicore
- Uses Protothreads and two timer interrupts to perform Direct Digital Synthesis of two sine waves, one on each of the two cores of the RP2040. 
- Those sine waves are amplitude-modulated to "beeps".
#### DMA Demo
- Uses a DMA channel to send a sine wave out through an SPI channel to the DAC.
- [**Documentation for this example**](https://vanhunteradams.com/Pico/DAC/DMA_DAC.html)
#### Audio FFT
- Uses a DMA channel to gather samples from the ADC, then performs an [FFT](https://vanhunteradams.com/FFT/FFT.html) on the gathered samples and displays to the VGA.
- [**Documentation for this example**](https://vanhunteradams.com/Pico/VGA/FFT.html)