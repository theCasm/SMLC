# SML

SML is a language designed to model SM213 assembly but without the parts that suck.
SML is not designed to be 'good' or 'effective' or 'nice to program with' - it
focuses on the much more important objective of being "at least its not assembly".  

Because it's intended to mimic SM213, SML is completely typeless - everything is a 4 byte word.
Types were a mistake, anyway - liberate yourself of these silly ideas. Add to booleans! Divide function pointers!
If you're feeling really saucy, you can even compare floats for equality!  

Some unenlightened folk will claim that typelessness limits compile-time error detection. They say
typed variables help you infer purpose.  

Do not believe their lies.  

Cast off these chains of conformity and experience programming as it was meant to be. The first
step on your journey of freedom is to learn SML they way languages are meant to be learned:
read the EBNF formal specification scattered in comments throughout the compilers parser.

# SMLC

SMLC is a compiler for SML. With cutting edge technology like memory accesses everytime you want to check a variable's value, 
it is truly fit for such a glorious language.

## Usage:
1. Download the code
2. In a terminal in the codes folder, run `make`. You might want to edit the Makefile if you don't use clang as your compiler.

Now you can run SMLC! The recommended way to do this is to write your SML in a file first.  
Then, run `./build/smlc < ./path/to/file.txt` (replacing ./path/to/file.txt with the actual path to your file). This will output the assembly to the screen. To write the output to a file, use
`./build/smlc < ./path/to/file.txt > ./path/to/output.s`, which will write the output to `./path/to/output.s`.  

For example, to compile the test program `./testPrograms/valid/testFullProgram.txt` (which is the SML equivilant of a solution to Assignment 6 Q5) and save the output as q3.s, you would run  
`./build/smlc < ./testPrograms/valid/testFullProgram.txt > q3.s`  
Feel free to open up q3.s and add a test case! Its a lot easier than writing all the assembly by hand.  
