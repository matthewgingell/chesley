#! /usr/bin/env ruby
#
# Run a number of tests against Chesely.
#
# Matthew Gingell
# gingell@adacore.com

for test in File.open("tests/ECM.EPD")
  puts("epd #{test}")
end
puts("quit")
