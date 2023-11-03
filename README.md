tiny-js-fusion360
=======

This project is an embedable environment for Fusion 360 Post Processors based on Tiny-JS.

The interpreter has no dependencies other than normal C++ libraries. 

I make absolutely no guarantees that this is compliant to JavaScript/EcmaScript standard, 
nor does it work with every Fusion 360 Post Processor ( although that is a goal, so 
please submit a bug if it doesn't).

Currently TinyJS supports:

* Variables, Arrays, Structures
* JSON parsing and output
* Functions
* Calling C/C++ code from JavaScript
* Objects with Inheritance (not fully implemented)
