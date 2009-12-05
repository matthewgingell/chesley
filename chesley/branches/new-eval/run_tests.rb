#! /usr/bin/env ruby
#
# Run a number of tests against Chesely.
#
# Matthew Gingell
# gingell@adacore.com

for test in File.open("tests/WAC.EPD");
#for test in File.open("tactics/wac.fails.1s")
  puts("epd #{test}")
end
puts("quit")
