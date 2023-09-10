# Blocks (Prototype)

This project is a short-lived proof-of-concept to test whether it would be possible to develop similar functionality to Sandia Laboratories' [Snippet](https://www.osti.gov/servlets/purl/1762995) tool without a high degree of complexity (or resource costs).

The Qt framework was used to develop almost everything here (in the spirit of Sandia Labs' version) however the drawbacks of that framework and its licensing - combined with the effects of data loss, unsatisfactory code structure, and failed cross-platform efforts - have led to this repository being archived.

Blocks currently won't compile using any other OS than Qt on MacOS, and lacks a substantial amount of the functionality that would be needed to be compliant with Snippet's standard. Nontheless, it will likely serve as a useful reference point for future versions of source code auditing tools that might make use of different technology than the Qt framework or C++ (i.e, plugins for other applications such as Ghidra or Visual Studio Code, or web-based tools).
