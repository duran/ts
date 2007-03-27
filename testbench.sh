#!/bin/bash

# Some simple tasks
./ts -K
./ts ls
./ts -n ls > /dev/null
./ts -f ls
./ts -nf ls > /dev/null
./ts ls
./ts cat
./ts -w

LINES=`./ts -l | grep finished | wc -l`
if [ $LINES -ne 6 ]; then
  echo "Error in simple tasks."
  exit 1
fi

./ts -K

# Check errorlevel 1
./ts -f ls
if [ $? -ne 0 ]; then
  echo "Error in errorlevel 1."
  exit 1
fi
# Check errorlevel 2
./ts -f patata
if [ $? -eq 0 ]; then
  echo "Error in errorlevel 2."
  exit 1
fi
# Check errorlevel 3
./ts patata
if [ $? -ne 0 ]; then
  echo "Error in errorlevel 3."
  exit 1
fi
# Check errorlevel 4
./ts ls
./ts -w
if [ $? -ne 0 ]; then
  echo "Error in errorlevel 4."
  exit 1
fi
# Check errorlevel 5
./ts patata
./ts -w
if [ $? -eq 0 ]; then
  echo "Error in errorlevel 5."
  exit 1
fi

./ts -K
