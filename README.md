Who, What, Where, Why?
======================

* Chesley the Chess Engine! is a computer chess program written in C++
  supporting both Windows and POSIX. Chesley is compatible with the
  xboard GUI.

* Chesley takes its name from Chesley B. "Sully" Sullenberger, the
  pilot who successfully landed an airplane on the Hudson river around
  the time I was getting started.

* To install, see the Makefile in this directory. To build Chesley you
  will need a C++ compiler. You will need at the very least to set the
  variable GXX to point to the compiler you want to use.

  Chesley will perform *dramatically* better if your compiler and
  target support 64-bit executable. If possible, the engine should be
  compiled with -m64. However if that doesn't work you will need to
  remove that flag.

* I'd love to hear from anyone with comments, so please feel free to
  end me some mail at matthewgingell@gmail.com
