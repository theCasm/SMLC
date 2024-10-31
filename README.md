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

SMLC is meant to be a compiler for SML. As of now, it can parse the language and it's syntax
error messages are even helpful sometimes. I predict that this project will continue to develop
well until the horrors of code generation swallow it whole.
