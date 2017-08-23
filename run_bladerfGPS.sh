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
./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 36.375024,127.361600,50
#./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 36.372814,127.360602,20 #futsal R
#./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 36.372811,127.359878,20 #futsal L
#./bladegps -t `date -u +'%Y/%m/%d,%H:%M:%S'` $* -i -l 35.453611,126.447222,20
