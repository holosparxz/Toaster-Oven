# Toaster Oven by Matt Acosta

## File Directory Tree

1. Adc.h - Potentiometer usage
2. Ascii.h - Interface for the Oled
3. BOARD.c - Hardware library
4. BOARD.h - Standard header file to activate the board
5. Buttons.h - Button activation library
6. ToasterOven.c - Toaster Code
7. ToasterOvenSupportLib.a - Executable code for the
functions in Buttons.h and Adc.h
8. Leds.h - Library for using the Leds
9. Oled.h - implements the Oled
10. OledDriver.h - Driver file responsible for implementing the Oled
11. README.md - Lists contents of the Lab and a writeup

## Writeup

I worked by myself for this lab and I did my own work on my own. I listed all my sources just as a precaution just to let the readers know that I did my necessary research for the project. This lab went over finite state machines in order to implement a toaster over on the microcontroller we did this in order to understand how to program reactive systems. When Approaching this lab I started by declaring my macros, static variables, and such so I know what I needed to use. After that I tackled the print statements. I ended up using a lot of switch statements to do most of my code so I just used the switches in order to do what my function needed. The extra credit was the second to last step and I used a simple ternary operator and called it a day. I finally did the Leds.h file last not for any specific reason, I just forgot that we had to edit the file so last minute I had to make those edits in order for the leds to turn on. All and all, I am happy how this lab turned out for me. Everything works well I just wish I started earlier. The content presented to us was very useful for learning finite state machines and I found the material worthwhile.

## Sources Cited

1. [Psuedocode](https://canvas.ucsc.edu/courses/49742/assignments/309343)
2. [The C Programming Language 2nd Edition](https://canvas.ucsc.edu/courses/49742/files/folder/Handouts?preview=4895560)
3. [Bitwise Operations](https://www.geeksforgeeks.org/bitwise-operators-in-c-cpp/)
4. [Structures](https://www.programiz.com/c-programming/c-structures)
5. [How to Bitshift in C](https://www.delftstack.com/howto/c/arithmetic-shift-in-c/)
6. [Finite State Machines](https://web.mit.edu/6.111/www/f2017/handouts/L06.pdf)
7. [Understanding Finite State Machines](https://www.freecodecamp.org/news/state-machines-basics-of-computer-science-d42855debc66/)
