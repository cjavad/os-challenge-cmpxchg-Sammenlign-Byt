#!/bin/sh

PATHTOCOMMON=/home/vagrant/os-challenge-common
SERVER=127.0.0.1
PORT=8080


# CUSTOM EDIT ME.
function custom() {
  echo "Custom"
SEED=1234
TOTAL=100000
START=0
DIFFICULTY=10000
REP_PROB_PERCENT=0
DELAY_US=0
PRIO_LAMBDA=0
}

# FINAL DO NOT EDIT ME.
function final() {
  echo "Final"
SEED=5041
TOTAL=500
START=0
DIFFICULTY=30000000
REP_PROB_PERCENT=20
DELAY_US=600000
PRIO_LAMBDA=1.5
}

# MIDWAY DO NOT EDIT ME.
function midway() {
  echo "Midway"
SEED=3435245
TOTAL=100
START=0
DIFFICULTY=30000000
REP_PROB_PERCENT=20
DELAY_US=600000
PRIO_LAMBDA=1.5
}


# INITIAL DO NOT EDIT ME.
function initial() {
  echo "Initial"
SEED=1234
TOTAL=20
START=1
DIFFICULTY=10
REP_PROB_PERCENT=0
DELAY_US=100000
PRIO_LAMBDA=0
}

set -a

# Get first argument
if [ "$1" = "custom" ]; then
    custom
elif [ "$1" = "final" ]; then
    final
elif [ "$1" = "midway" ]; then
    midway
else
    initial
fi

set +a

./client $SERVER $PORT $SEED $TOTAL $START $DIFFICULTY $REP_PROB_PERCENT $DELAY_US $PRIO_LAMBDA
