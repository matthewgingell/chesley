#! /usr/bin/env ruby
#
# Run a number of tests against Chesely.
#
# Matthew Gingell
# gingell@adacore.com

for test in File.open("tests/matein4.epd")
  puts("epd #{test}")
end
puts("quit")
