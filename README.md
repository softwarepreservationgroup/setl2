# setl2

SETL2 programming language by W. Kirk Snyder, with contributions by Salvatore Paxia, Jack Schwartz, and Giuseppe Di Mauro (see [AUTHORS](AUTHORS)).  It is licensed under the MIT License -- see [LICENSE](LICENSE). 

In May 2020, Salvatore Paxia responded to my enquiry about SETL2 with a snapshot of the source that he'd gotten running on a 64-bit Ubuntu distribution; it also builds on x86_64 Macintosh and armv7l Raspbian. Kirk Snyder gave us permission to distribute the code. See [INSTALL](INSTALL) for building the program.

In his Ph.D. thesis "SETL for Internet Data Processing" [4], David Bacon observed:

>  In the late 1980s at NYU, meanwhile, there was a project aimed at overhauling the CIMS SETL language and implementation. The new version of the language was tentatively named SETL2. Kirk Snyder, a graduate student at that time, became dissatisfied with what appeared to be ceaseless discussion supported by little action, and covertly designed and implemented his own system called SETL2 [1] in about a year. It simplified and modified various aspects of SETL syntax and semantics while it removed the usual apocrypha (macros, backtracking, the data representation sublanguage, and SETL modules), and introduced Ada-based “packages” and a portable file format for separate compilation on DOS, Macintosh, Unix, and other platforms. Snyder subsequently added lambda expressions (first-class functions with closures) and support for object-oriented programming, including multiple inheritance [2], though these extensions were not entirely without semantic problems in the context of a nominally value-oriented language. Recently, [Salvatore] Paxia has made improvements to the interoperability of SETL2 through his “native” package declarations [3] which allow more direct calls out to routines written in C than were previously possible.

[1] W. Kirk Snyder. The SETL2 Programming Language. Technical Report 490, Courant Institute of Mathematical Sciences, New York University, 1989; updated September 9, 1990. [pdf](doc/report.PDF)

[2] W. Kirk Snyder. The SETL2 Programming Language: Update On Current Developments. Courant Institute of Mathematical Sciences, New York University, September 7, 1990. [pdf](doc/update.PDF)

[3] Salvatore Paxia. A string matching native package for the SETL2 language, March 1999.

[4] David Bacon. SETL for Internet Data Processing. Ph.D. thesis, Computer Science, New York University, January, 2000. [pdf at cs.nyu.edu](https://cs.nyu.edu/media/publications/bacon_david.pdf)

This introduction is also available:

* Robert Hummel. A Gentle Introduction to the SETL2 Programming Language. Revised version of Robert B. K. Dewar: The SETL Programming Language, New York University, 1979. [pdf](doc/intro.PDF)

Also, Jack Schwartz was in the process of revising _Programming in SETL_ to be consistent with the SETL2 language; his draft is available at [settheory.org](http://www.settheory.com).


Paul McJones  
Mountain View, California  
July 28, 2020
