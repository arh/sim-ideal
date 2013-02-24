#!/bin/bash 
#===============================================================================
#
#          FILE:  draw_all.sh
# 
#         USAGE:  ./draw_all.sh (gnuplot data files)
# 
#   DESCRIPTION:  draw all files with gnuplot
# 
#        AUTHOR: Alireza Haghdoost (arh), haghdoost@gmail.com
#       CREATED: 05/07/2011 06:59:45 PM IRDT
#      REVISION:  1.0
#	reference: http://sharats.me/the-ever-useful-and-neat-subprocess-module.html
#===============================================================================

set -o nounset # Treat unset 1s as an errorwdev_0
XLABLE="Distance"
YLABLE="BIRD Histogram (log Scale)"
Y2LABLE="PIRD Histogram"

for i in $@
	do
	
		gnuplot -e " 	set ylabel '$YLABLE';\
				set y2label '$Y2LABLE';\
				set xlabel '$XLABLE';\
				set ytics nomirror tc lt 1;\
				set y2tics nomirror tc lt 2;\
				set yrange [1:*];\
				set y2range [1:*];\
				set xrange [2:*];\
				set logscale y; \
				set logscale y2; \
				set title '$i';\
				set terminal png size 1600,800 enhanced font 'Verdana,20';\
				set output '${i}.png' ;\
				set grid x y;\
				set key top right;\
				set lmargin 11;\
				set rmargin 10;\
				set label '96%' at 2000,1100 tc lt 1 ; \
				set label '84%' at 1500,1100 tc lt 2 ; \
				set arrow from 0,700 to 4096,700 heads back filled lw 2.5 ; \
				set label '94%' at 1100,7 tc lt 1; \
				set label '81%' at 600,7 tc lt 2; \
				set arrow from 0,5 to 2048,5 heads back filled lw 2; \
 show arrow;\
				plot 
				'$i.BIRD' using 1:2 axes x1y1 title 'BIRD' with points lt 1 pt 6 ps 0.3 , 
				'$i.PIRD' using 1:2 axes x1y2 title 'PIRD' with points lt 2 pt 6 ps 0.3 ;	"
done 
#				'$i.BIRD' using 1:2 axes x1y1 title 'BIRD' with l , 
#				'$i.PIRD' using 1:2 axes x1y2 title 'PIRD' with l ;	"

