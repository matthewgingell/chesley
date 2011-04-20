#! /usr/bin/env ruby
#
# Run a number of tests against Chesely.
#
# Matthew Gingell
# gingell@adacore.com

#for test in File.open("tests/undermine.epd");
#for test in File.open("tests/WAC.EPD");
#for test in File.open("fails");
#for test in File.open("tactics/wac.fails.1s")
#for test in File.open("tests/STS/Knight_Outposts.epd")
#for test in File.open("tests/STS/Undermine.epd")
#for test in File.open("tests/STS/Open_Files_and_Diagonals.epd")
for test in File.open("tests/STS/Square_Vacancy.epd")
  puts("epd #{test}")
end
puts("quit")
