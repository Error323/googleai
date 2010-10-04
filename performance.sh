#!/bin/sh
#
# Usage: $(basename $0) $(enemydir $1)

# java -jar tools/PlayGame.jar maps/map10.txt 1000 200 log.txt "./MyBot" "./submits/MyBot" &> out.txt

enemies=( $1bot-* )
N=${#enemies[*]}

wins=0
losses=0

process() {
	java -jar tools/PlayGame.jar "$1" 1000 200 log.txt "./MyBot" "./$2" &> out.txt
	echo $1 $2 `grep 'Player' out.txt`
	result=`grep 'Player 1' out.txt`
	if [ ! -n "$result" ]
	then
		(( wins++ ))
	else
		(( losses++ ))
	fi
	rm out.txt
}

for ((i = 0; i < N; i++)); do
	for map in maps/*; do
		process "${map}" "${enemies[i]}"
	done
done

rm $1/*.txt
echo "Won: ${wins} Lost: ${losses}"
