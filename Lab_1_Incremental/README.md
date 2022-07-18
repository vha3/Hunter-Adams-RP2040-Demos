# Incremental construction of Lab 1 demo code

In [Lab 1](https://vanhunteradams.com/Pico/Cricket/Crickets.html) you are asked to synthesize and synchronize snowy tree cricket chirps. In this lab, as in all labs, you are provided with demo code from which to start your assignment. This repository contains a sequence of example projects which incrementally build to the demo code from which you should start your assignment. The documentation for each of these examples is meant to be read *in order*, starting from the Blinky Demo and moving down.


## Weeks 1 and 2
#### Blinky Demo
- Blinks the Pico's onboard LED by toggling a GPIO port.
- The "Hello world!" of a new microcontroller.
- [Documentation for this example](https://vanhunteradams.com/Pico/Setup/UsingPicoSDK.html)
#### Timer Interrupt DDS Demo
- Blinky demo is augmented to include a timer interrupt, and SPI communication to a DAC.
- Timer interrupt performs Direct Digital Synthesis of a sine wave, which is output through the SPI DAC. 
- Note the differences in the CMakeLists.txt file, and in the includes!
- [More info on DDS](https://vanhunteradams.com/DDS/DDS.html).
- [More info on SPI](https://vanhunteradams.com/Protocols/SPI/SPI.html)
#### Multicore DDS Demo
- Uses two timer interrupts to perform Direct Digital Synthesis of two sine waves, one on each of the two cores of the RP2040. 
- Those sine waves are amplitude-modulated to "beeps".
- [More info on Fixed Point](https://vanhunteradams.com/FixedPoint/FixedPoint.html)
- [Documentation for this example](https://vanhunteradams.com/Pico/Multi/MultiCore.html)
#### Protothreads Demo
- A thorough demonstration of Protothreads.
- [Documentation from Bruce](https://people.ece.cornell.edu/land/courses/ece4760/RP2040/C_SDK_protothreads/index_Protothreads.html)
#### Audio Beep Synthesis - *Starting point for Weeks 1 and 2!*
- Uses Protothreads and two timer interrupts to perform Direct Digital Synthesis of two sine waves, one on each of the two cores of the RP2040. 
- Those sine waves are amplitude-modulated to "beeps".

## Week 3
#### DMA Demo
- Uses a DMA channel to send a sine wave out through an SPI channel to the DAC.
- [Documentation for this example](https://vanhunteradams.com/Pico/DAC/DMA_DAC.html)
#### Audio FFT - *Starting point for Week 3!*
- Uses a DMA channel to gather samples from the ADC, then performs an FFT on the gathered samples and displays to the VGA.
- [More info on FFT's](https://vanhunteradams.com/FFT/FFT.html)
- [Documentation for this example](https://vanhunteradams.com/Pico/VGA/FFT.html)