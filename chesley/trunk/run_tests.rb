#! /usr/bin/env ruby
# -*- coding: iso-8859-1 -*-
#
# Run a number of tests against Chesely. 
#
# Matthew Gingell
# gingell@adacore.com

puts "Running move generation correctness tests."

c = IO.popen("./chesley", "r+")
for test in File.open("tests/perftsuite.epd")
  c.puts("epd #{test}")
end
c.puts("quit")
for result in c
  puts(result)
end
