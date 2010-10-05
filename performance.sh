#!/bin/sh
#
# Usage: $(basename $0) $(enemydir $1)

# java -jar tools/PlayGame.jar maps/map10.txt 1000 200 log.txt "./MyBot" "./submits/MyBot" &> out.txt

enemies=( $1bot* )
N=${#enemies[*]}

wins=0
draws=0
losses=0

process() {
	java -jar tools/PlayGame.jar "$1" 1000 200 log.txt "./MyBot" "./$2" &> out.txt
	echo $1 $2 `grep '!' out.txt`

	win="`grep 'Player 1' out.txt`"
	lost="`grep 'Player 2' out.txt`"
	draw="`grep 'Draw' out.txt`"

	if [ -n "$win" ]
	then
		(( wins++ ))
	elif [ -n "$draw" ]
	then
		(( draws++ ))
	elif [ -n "$lost" ]
	then
		(( losses++ ))
	fi

	rm out.txt
}

for ((i = 0; i < N; i++)); do
	if [ -x "${enemies[i]}" ]
	then
		for map in maps/*; do
			process "${map}" "${enemies[i]}"
		done
	fi 
done

rm $1/*.txt
let total=$wins+$losses+$draws
echo "Won: ${wins}/${total} Lost: ${losses}/${total} Draws: ${draws}/${total}"
