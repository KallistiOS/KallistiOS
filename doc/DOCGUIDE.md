# Documentation Style Guide

## A Guide For Writing and Reading KOS' Documentation

### Documentation Structure

KallistiOS uses Doxygen to generate documentation from the project's header 
files and the following .dox files:

- attribution.dox 
- audio.dox
- debugging.dox
- filesystem.dox
- math.dox
- networking.dox
- peripherals.dox
- system.dox
- threading.dox
- timing.dox
- video.dox

All of them can be found in the doc folder.

#### Topics

Doxygen's documentation is organized in a tree-like structure constructed around
Topics and Files. In KallistiOS, the previously mentioned list of files describes
the top level Topics used to group the corresponding subtopics. As an example,
the audio Topic hosts the Driver, Sound Effects and Streaming subtopics.
Topics can link to Files and other topics as well. 

#### Files

Files represent (optional) units of documentation that exist under Topics. Doxygen
typically uses them to document specific source or header files. In KallistiOS,
Files are used to list macros, function signatures, function lists and the
headers used in the file.

---

### Documentation Level of Detail

Different levels of detail are provided in KallistiOS' documentation as a way to
help the reader quickly find their way through the documentation and the OS.
To achieve these two goals, documentation should be written with more
detail as it gets closer to the File level.

Topic level documentation should aim to inform the reader of what they will find
inside it. A top level Topic description must state the API's purpose and
hardware it might relate to.

Subtopics are the place to start providing more detailed insights into the API
by describing its capabilities and limitations, providing function descriptions 
and alerting the reader to common pitfalls and known issues. Subtopics that
aren't parents to other subtopics should also provide a basic guide into using 
the API, with a focus on any setup required to use it and its key
functions.

At the File level, the documentation should focus on describing the inner
workings of a module as well as more advanced API use cases and data structures.

#### Linking to Other Pages

When expanding the documentation, it is good practice to provide links to
important functions or macros mentioned. If a Topic has any type of relationship
with resources that belong to another Topic, Doxygen functionalities should be
used to ensure that the page points to the mentioned resources. 
This will help readers reach important pages that might be of their interest.

---

### Where to Write What

Documentation should be kept in the header files of KallistiOS, while the .dox
files should be used to define groups. As modern IDEs are capable of parsing
Doxygen style documentation, keeping the majority of the documentation inside
header files can prove useful for users as they use KallistiOS' APIs.

In cases where a module has a top level header and then headers specific to
other parts of the module, documentation should be kept on the specific header
and then use Doxygen commands to present it on the top level header file, if
needed. This allows for documentation to be kept in the corresponding header,
maintaining the ability to use Doxygen's function specific functionalities
without requiring a reorganising of function definitions inside the headers. 

---

### Style Guide

When contributing to the docs (either with new documentation or updates to 
existing docs), please make sure you follow this style guide so that everything 
stays uniform across pages. 

It is possible that chunks of code or documentation written in some header files
belong on a different page than the one Doxygen is putting them on, but also contains
information that, by virtue of providing IDEs information about the code, must not 
be moved. In these cases, Doxygen's `\addtogroup` allows writers to move documentation
blocks around, without changing the header file where it is written 

#### Topic hierarchy

Top level Topics are defined in their own text file in the `./doc/` folder; its in these files
that their specific subtopics must be declared so that they can be referenced in the header files later.

##### Functions

When mentioned in the documentation, functions __must__ link back to their signatures. 

##### See Also

Relevant macros or pages should be included in this section, so the reader can quickly move to pages that
can be of interest. Structs are also good candidates to this section, as it often isn't possible to cover
all of the thought that went into the members, alignments, padding and packing.