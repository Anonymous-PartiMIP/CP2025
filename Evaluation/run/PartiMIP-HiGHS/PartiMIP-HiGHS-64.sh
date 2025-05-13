#!/bin/bash

thread="64"

SEND_THREAD_NUM=$((128 / $thread))
tmp_fifofile="/tmp/$$.fifo"
mkfifo "$tmp_fifofile"
exec 6<>"$tmp_fifofile"
rm $tmp_fifofile
for i in $(seq 1 $SEND_THREAD_NUM); do
  echo
done >&6

eval_dir="../../../"

instance="$eval_dir/Benchmark/mps"
benchmark_list="$eval_dir/Benchmark/benchmark.txt"
result="$eval_dir/Result/PartiMIP-HiGHS/thread_$thread"
cutoff="300"
timeout="600"

all_datas=($instance)
for cut in $cutoff; do
  for ((i = 0; i < ${#all_datas[*]}; i++)); do
    instance=${all_datas[$i]}
    res_solver_ins=$result/${cut}
    if [ ! -d "$res_solver_ins" ]; then
      mkdir -p $res_solver_ins
    fi
    for dir_file in $(cat $benchmark_list); do
      file=$dir_file
      echo $file
      touch $res_solver_ins/$file
      read -u 6
      {
        timeout $timeout $eval_dir/Evaluation/pre-compiled/PartiMIP-HiGHS/PartiMIP-HiGHS -i $instance/$file -t $thread -c $cut
        echo >&6
      } >$res_solver_ins/$file 2>>$res_solver_ins/$file &
    done
  done
done

exit 0
