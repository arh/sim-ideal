#PBS -N wdev
#PBS -e wdev.e
#PBS -o wdev.o
#PBS -l walltime=72:00:00,nodes=1:ppn=1
#PBS -M haghdoost@gmail.com
#PBS -m a
#PBS -q oc
DIR=/home/dudh/haghdoos/sim-ideal
TRACE=/home/dudh/haghdoos/MSR-Cambridge
cd $DIR
#./sim-ideal $TRACE/wdev_0.csv pagelru wdev_0 2048 2048
#./sim-ideal $TRACE/wdev_0.csv pagemin wdev_0 2048 2048
#./sim-ideal $TRACE/wdev_0.csv blockmin wdev_0 2048 2048
#./sim-ideal $TRACE/wdev_0.csv owbp wdev_0 2048 2048
#./sim-ideal $TRACE/wdev_0.csv owbp wdev_0 2048 4096
./sim-ideal $TRACE/wdev_0.csv owbp wdev_0 2048 8192
./sim-ideal $TRACE/wdev_0.csv owbp wdev_0 2048 16384

