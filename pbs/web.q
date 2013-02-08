#PBS -N web
#PBS -e web.e
#PBS -o web.o
#PBS -l walltime=72:00:00,nodes=1:ppn=1
#PBS -M haghdoost@gmail.com
#PBS -m a
#PBS -q oc
DIR=/home/dudh/haghdoos/sim-ideal
TRACE=/home/dudh/haghdoos/MSR-Cambridge
cd $DIR
./sim-ideal $TRACE/web_0.csv pagelru web_0 2048 2048
./sim-ideal $TRACE/web_0.csv pagemin web_0 2048 2048
./sim-ideal $TRACE/web_0.csv blockmin web_0 2048 2048
./sim-ideal $TRACE/web_0.csv owbp web_0 2048 2048
./sim-ideal $TRACE/web_0.csv pagemin web_0 2048 4096
./sim-ideal $TRACE/web_0.csv blockmin web_0 2048 4096
./sim-ideal $TRACE/web_0.csv owbp web_0 2048 4096

