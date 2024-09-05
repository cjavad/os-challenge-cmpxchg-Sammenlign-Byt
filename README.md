# cmpxchg-Sammenlign&Byt's server implementation

## Server

TODO.

## TCP Protocol

- Read raw bytes from protocol into BE ds, and convert to LE ds we perform further processing on.

- The opposite is done to produce the answer. 


## Hash lookup

Shared nmap'ed memory regions based on a BTreeMap using the start and end ranges (buckets)
run dedicated processed to smartly cache computation without over computing.

In principle implement a database using a bespoke data format for lookup. 
