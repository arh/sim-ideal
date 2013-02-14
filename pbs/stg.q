#PBS -N stg
#PBS -e stg.e
#PBS -o stg.o
#PBS -l walltime=12:00:00,nodes=1:ppn=8
#PBS -M haghdoost@gmail.com
#PBS -m a
#PBS -q batch
DIR=/home/dudh/haghdoos/sim-ideal
TRACE=/home/dudh/haghdoos/MSR-Cambridge
cd $DIR
./sim-ideal $TRACE/stg_0.csv pagelru stg_0 2048 2048 &
./sim-ideal $TRACE/stg_0.csv pagemin stg_0 2048 2048 &
./sim-ideal $TRACE/stg_0.csv blockmin stg_0 2048 2048 &
./sim-ideal $TRACE/stg_0.csv owbp stg_0 2048 2048 &
./sim-ideal $TRACE/stg_0.csv owbp stg_0 2048 4096 &
./sim-ideal $TRACE/stg_0.csv owbp stg_0 2048 8192 &
./sim-ideal $TRACE/stg_0.csv owbp stg_0 2048 16384 &
wait
