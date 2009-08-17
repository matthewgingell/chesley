#! /usr/bin/env ruby
#
# Run a number of tests against Chesely.
#
# Matthew Gingell
# gingell@adacore.com

for test in File.open("tests/perftsuite.epd")
#for test in File.open("tests/WAC.EPD")
#for test in File.open("fails.epd")
  puts("epd #{test}")
end
puts("quit")
