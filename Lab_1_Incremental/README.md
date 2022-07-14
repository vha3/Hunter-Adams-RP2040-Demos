# Incremental construction of Lab 1 demo code


## Weeks 1 and 2
#### Blinky Demo
- Blinks the Pico's onboard LED by toggling a GPIO port.
- The "Hello world!" of a new microcontroller.
- **Timer Interrupt Demo**: Blinks the onboard LED inside a timer interrupt.
- **Timer Interrupt DDS Demo**: Uses a timer interrupt to perform Direct Digital Synthesis of a sine wave, which is output through an SPI DAC. In this example, note the differences in the CMakeLists.txt file.
- **Multicore DDS Demo**: Uses two timer interrupts to perform Direct Digital Synthesis of two sine waves, one on each of the two cores of the RP2040. Those sine waves are amplitude-modulated to "beeps".
- **Protothreads Demo**: A thorough demonstration of Protothreads.
- **Audio Beep Synthesis**: Uses Protothreads and two timer interrupts to perform Direct Digital Synthesis of two sine waves, one on each of the two cores of the RP2040. Those sine waves are amplitude-modulated to "beeps".

## Week 3
- **DMA Demo**: Uses a DMA channel to send a sine wave out through an SPI channel to the DAC.
- **Audio FFT**: Uses a DMA channel to gather samples from the ADC, then performs an FFT on the gathered samples and displays to the VGA.