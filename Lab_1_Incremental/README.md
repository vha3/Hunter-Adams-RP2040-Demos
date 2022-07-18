# Incremental construction of Lab 1 demo code


## Weeks 1 and 2
#### Blinky Demo
- Blinks the Pico's onboard LED by toggling a GPIO port.
- The "Hello world!" of a new microcontroller.
- [Documentation for this example](https://vanhunteradams.com/Pico/Setup/UsingPicoSDK.html)
#### Timer Interrupt DDS Demo
- Uses a timer interrupt to perform Direct Digital Synthesis of a sine wave, which is output through an SPI DAC. 
- Note the differences in the CMakeLists.txt file.
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
#### Audio Beep Synthesis
- Uses Protothreads and two timer interrupts to perform Direct Digital Synthesis of two sine waves, one on each of the two cores of the RP2040. 
- Those sine waves are amplitude-modulated to "beeps".
- Starting point for Weeks 1 and 2 checkoffs

## Week 3
#### DMA Demo
- Uses a DMA channel to send a sine wave out through an SPI channel to the DAC.
#### Audio FFT
- Uses a DMA channel to gather samples from the ADC, then performs an FFT on the gathered samples and displays to the VGA.
- The starting point for the Week 3 checkoff