reset
datafile = 'time.txt'
firstrow = system('head -1 '.datafile)
set xlabel word(firstrow, 1)
set ylabel 'time'
set term png enhanced font 'Verdana,10'
set output 'runtime.png'

N = system("awk 'NR==1{print NF}' time.txt")

plot for [i=2:N] datafile u 0:i w linespoints title word(firstrow,i)
