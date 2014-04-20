require "math"
require "os"

-- create global functions

-- call randomization function
math.randomseed(os.time())

-- dice utilities
function d(side, number)
	local outcome = 1 + math.floor(math.random()*side)
	
	if number <= 0 then
		return 0
	end

	if number == 1 then
		return outcome
	end
	
	return outcome + d(side, number-1)
end

function best_d(side, number, best)
	local rolls = {}

	-- get 'number' dice throw 
	for ii = 1, number, 1 do
		rolls[ii] = d(side, 1)
	end

	-- sort the results
	table.sort(rolls)

	-- sum the first best
	local res = 0
	for ii = (number-best)+1, number, 1 do
		res = res + rolls[ii]
	end

	return res
end