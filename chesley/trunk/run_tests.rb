#! /usr/bin/env ruby
# -*- coding: iso-8859-1 -*-
#
# Run a number of tests against Chesely.
#
# Matthew Gingell
# gingell@adacore.com



###################################
#           Perft tests           #
###################################

# puts "Running move generation correctness tests."

# c = IO.popen("./chesley", "r+")
# for test in File.open("tests/perftsuite.epd")
#   c.puts("epd #{test}")
# end

###################################
#      Zobrist hashing tests      #
###################################

# puts "Running hash correctness tests."
# c.puts("test_hashing")

puts "Running best move tests"
puts

c = IO.popen("sudo nice -20 ./chesley", "r+")
for test in File.open("tests/WAC.EPD")
  c.puts("epd #{test}")
end

# Quit and exit.

c.puts("quit")
for result in c
  puts(result)
end
puts("done.")

