#!/bin/bash

year=`date -u +%Y`
day=`date -u +%j`

#brdc_file="brdc${day}0.17n"
#brdc_file_year="brdc${year}_${day}0.17n"

#if [ ! -e "$brdc_file_year" ]; then
#	wget ftp://cddis.gsfc.nasa.gov/gnss/data/daily/${year}/brdc/${brdc_file}.Z -O ${brdc_file_year}.Z
	
#	if [ ! -e "${brdc_file_year}.Z" ]; then
#		echo "Can't download BRDC file"
#		exit
#	fi
#	uncompress ${brdc_file_year}.Z
#fi

#./bladegps -e $brdc_file_year -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i
#./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 36.374987,127.361584,60
#./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 36.391384,127.397872,60 #munji
#./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 37.369444,127.567778,60 #yeoju2
./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 37.3689,127.568333,60 #yeoju
#./bladegps $* -i -l 37.3689,127.568333,60 #yeoju
#./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 36.397800,127.402501,60
#./bladegps -e brdc1700.16n $* -i -l 36.374987,127.361584,60
#./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 36.372814,127.360602,20 #futsal R
#./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 36.372811,127.359878,20 #futsal L
