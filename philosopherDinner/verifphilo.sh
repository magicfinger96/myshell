#!/bin/bash


test $# -ne 1 && echo "Usage : philo n | $0 n où n est le nombre de philo max"&&exit 1

expr "$1" + 0 >& /dev/null
test $? -gt 1 && echo "$1 n'est pas un nombre valide"&&exit 2

nb=$1
listemange=""

add_mange() {
  gauche=`expr $1 - 1`
  droite=`expr $1 + 1`
  test $gauche -lt 0   && gauche=`expr $nb - 1`
  test $droite -eq $nb && droite=0 
  for i in $listemange
  do
    test $i -eq $gauche && echo "PB: $i et $1 mangent ensemble" && exit 0	
    test $i -eq $droite && echo "PB: $i et $1 mangent ensemble" && exit 0	
    test $i -eq $1      && echo "PB: $1 est déjà en train de manger"  && exit 0	
  done
  listemange="$listemange $1"
}

rm_mange() {
  n=""
  for i in $listemange
  do
     test $i -ne $1 && n="$n $i"	   	  	
  done
  listemange="$n"
}


while read l
do
  num=`echo "$l" | cut -d: -f 1 | cut -de -f 2`
  case "$l" in 
    *" : PENSE")
      rm_mange $num
      ;;
    *" : MANGE")
      add_mange $num
      ;;
  esac
done

